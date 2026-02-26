#pragma execution_character_set("utf-8")
#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
using namespace DirectX;
#include "shader.h"
#include "debug_ostream.h"
#include "game.h"
#include "field.h"
#include "texture.h"
#include "keyboard.h"
#include "scene.h"
#include "camera.h"
#include "sprite.h"
#include "UI.h"
#include "UI_PauseMenu.h"
#include "UI_Tutorial.h"
#include "ghost.h"
#include "furniture.h"
#include "fade.h"
#include "busters.h"
#include "Tutorial_Object.h"
#include "debugdraw.h"
#include "sound.h"
#include "minimap.h"
#include "mouse.h"
#include "light.h"
#include "model.h"
#include <windows.h>
#include <string>
#include <cmath>

static AmbientLight* g_pAmbientLight = nullptr;
static SoundData* g_pBGM = nullptr;

static int g_NextFloorID = -1;

// =================================================================
// フロア降下アニメーションのステートマシン
// =================================================================
enum FLOOR_EXIT_ANIM_STATE
{
	FLOOR_EXIT_NONE = 0,    // 通常状態
	FLOOR_EXIT_FADEOUT,     // フェードアウト中
	FLOOR_EXIT_OVERVIEW,    // 俯瞰カメラ + バスターズ走行中
	FLOOR_EXIT_FADEIN,      // フェードイン中
};

static FLOOR_EXIT_ANIM_STATE g_FloorExitState = FLOOR_EXIT_NONE;
static int  g_FloorExitTimer    = 0;
static bool g_FloorTransferDone = false; // フロア移行処理を実行済みか
static XMFLOAT3 g_OverviewCameraPos = { 0.0f, 0.0f, 0.0f };
static XMFLOAT3 g_StairsPosForAnim  = { 0.0f, 0.0f, 0.0f }; // アニメ開始時の階段座標（元フロア）

// 俯瞰カメラをバスターズの真上に設定する
static void SetupOverviewCamera(void)
{
	// バスターズが存在すればその位置に同期、なければゴーストにフォールバック
	XMFLOAT3 focusPos = { 0.0f, 0.0f, 0.0f };
	Busters* buster = GetBusters();
	if (buster)
	{
		focusPos = buster->GetPos();
	}
	else
	{
		Ghost* ghost = GetGhost();
		if (ghost) focusPos = ghost->GetPos();
	}

	// カメラをY+20の真上に置き、注視点をXZ同座標・Y=0（地面）に向ける
	XMFLOAT3 camPos  = { focusPos.x, focusPos.y + 20.0f, focusPos.z };
	XMFLOAT3 atPos   = { focusPos.x, focusPos.y,          focusPos.z };
	g_OverviewCameraPos = camPos;

	Camera* cam = GetCamera();
	if (cam) cam->UpdateView(camPos, atPos);
}

static bool g_FloorExitAnimRequested = false; // ghost.cppからの要求フラグ

void Game_RequestFloorExitAnim(void)
{
	g_FloorExitAnimRequested = true;
}

bool Game_IsFloorExitAnimActive(void)
{
	return g_FloorExitState != FLOOR_EXIT_NONE;
}

bool* Game_GetEnbanTouchedPtr(void)
{
	return TutorialObject_GetEnbanTouchedPtr();
}

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pAmbientLight = new AmbientLight(XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));

	Camera_Initialize();
	Ghost_Initialize(pDevice, pContext);
	Field_Initialize(pDevice, pContext);
	UI_Initialize();
	Furniture_Initialize();
	Busters_Initialize();
	Minimap_Initialize();
	DebugDraw_Initialize();

	TutorialObject_Initialize();

	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) PlaySound(g_pBGM, true);

	UI_PauseMenu_Initialize(pDevice, pContext);
	UI_Tutorial_Initialize(pDevice, pContext);
}

void Game_Update(void)
{
	FADESTAT fadeState = GetFadeState();

	// -------------------------------------------------------
	// フロア降下アニメーションのステートマシン
	// -------------------------------------------------------
	if (g_FloorExitState != FLOOR_EXIT_NONE)
	{
		switch (g_FloorExitState)
		{
		case FLOOR_EXIT_FADEOUT:
			// フェードアウト中はバスターズを更新して走らせ続ける
			Busters_Update();
			Field_Update();
			Furniture_Update();
			// 俯瞰カメラを維持
			{
				XMFLOAT3 focusPos = { 0.0f, 0.0f, 0.0f };
				Busters* buster = GetBusters();
				if (buster) focusPos = buster->GetPos();
				else { Ghost* g = GetGhost(); if (g) focusPos = g->GetPos(); }
				XMFLOAT3 camPos = { focusPos.x, focusPos.y + 20.0f, focusPos.z };
				Camera* cam = GetCamera();
				if (cam) cam->UpdateView(camPos, { focusPos.x, focusPos.y, focusPos.z });
				Shader_SetCameraPos(camPos);
			}
			// フェードアウト完了（真っ暗）になったらフロア移行してフェードイン
			if (fadeState == FADE_MAX)
			{
				if (!g_FloorTransferDone)
				{
					Busters_DoFloorTransition();
					g_FloorTransferDone = true;
				}
				SetupOverviewCamera();
				g_FloorExitState = FLOOR_EXIT_OVERVIEW;
				g_FloorExitTimer = 0;
				Fade_StartIn();
			}
			return;

		case FLOOR_EXIT_OVERVIEW:
		// バスターズが階段に到着するまで、俯瞰カメラで見守る
			g_FloorExitTimer++;

			// 俯瞰カメラをバスターズのX/Zに追従させながら維持
			{
				XMFLOAT3 focusPos = { 0.0f, 0.0f, 0.0f };
				Busters* buster = GetBusters();
				if (buster)
				{
					focusPos = buster->GetPos();
				}
				else
				{
					Ghost* ghost = GetGhost();
					if (ghost) focusPos = ghost->GetPos();
				}
				XMFLOAT3 camPos = { focusPos.x, focusPos.y + 20.0f, focusPos.z };
				XMFLOAT3 atPos  = { focusPos.x, focusPos.y,          focusPos.z };
				Camera* cam = GetCamera();
				if (cam) cam->UpdateView(camPos, atPos);
				Shader_SetCameraPos(camPos);
			}

			// バスターズ更新（走行アニメーション）
			Busters_Update();
			Field_Update();
			Furniture_Update();

			// 到着 or タイムアウト（最大8秒）かつフェード中でなければ次のフェードアウトへ
			if ((Busters_IsFloorExitAnimDone() || g_FloorExitTimer > 8 * FPS)
				&& fadeState == FADE_NONE)
			{
				StartFade(SCENE_NONE); // プレイヤー視点に戻すためフェードアウト
				g_FloorExitState = FLOOR_EXIT_FADEIN;
			}
			return; // 通常更新をスキップ

		case FLOOR_EXIT_FADEIN:
			// フェードアウト完了後、プレイヤー視点に戻してフェードイン
			if (fadeState == FADE_MAX)
			{
				// プレイヤー（ゴースト）の視点にカメラを戻す
				Ghost* ghost = GetGhost();
				if (ghost)
				{
					Camera_SetTargetPos(ghost->GetPos());
					// pitch/yaw をリセットして前後方向を向かせる
					Camera* cam = GetCamera();
					if (cam)
					{
						XMFLOAT3 gp = ghost->GetPos();
						XMFLOAT3 atPos = { gp.x, gp.y + CAMERA_OFFSET_Y, gp.z };
						XMFLOAT3 camPos = { gp.x, gp.y + CAMERA_OFFSET_Y + 6.0f, gp.z - 0.01f };
						cam->UpdateView(camPos, atPos);
					}
				}
				g_FloorExitState = FLOOR_EXIT_NONE;
				g_FloorExitTimer = 0;
				g_FloorTransferDone = false;
				g_FloorExitAnimRequested = false;
				Fade_StartIn();
			}
			return; // 通常更新をスキップ

		default:
			break;
		}
		return;
	}

	// フロア降下アニメーション要求を受け付ける
	if (g_FloorExitAnimRequested && g_FloorExitState == FLOOR_EXIT_NONE)
	{
		g_FloorExitAnimRequested = false;
		g_FloorExitState = FLOOR_EXIT_FADEOUT;
		g_FloorTransferDone = false;

		// 元フロアの階段位置を保存し、バスターズを今すぐ走らせる
		g_StairsPosForAnim = Field_GetStairsUpWorldPos(Field_GetCurrentFloor());
		Busters_StartFloorExitAnim(g_StairsPosForAnim);

		StartFade(SCENE_NONE); // まずフェードアウト
		return;
	}

	// -------------------------------------------------------
	// 通常フェード処理
	// -------------------------------------------------------
	if (fadeState == FADE_MAX)
	{
		if (g_NextFloorID != -1) {
			LoadMapData(g_NextFloorID);
			GetGhost()->SetPos(XMFLOAT3(0, 0, 0));
			g_NextFloorID = -1;
			Fade_StartIn();
		}
		return;
	}
	if (fadeState != FADE_NONE) return;

	UI_PauseMenu_Update();
	if (UI_PauseMenu_IsPaused())
	{
		return;
	}

	if (Field_GetCurrentFloor() == 2)
	{
		UI_Tutorial_Update();
		TutorialObject_Update();


		if (UI_Tutorial_IsWaiting())
		{
			Camera_Update();
			Shader_SetCameraPos(GetCamera()->GetPos());
			Field_Update();
			Ghost_Update();
			Furniture_Update();
			return;
		}

		if (UI_Tutorial_IsActive())
		{
			return;
		}
	}

	Camera_Update();
	Shader_SetCameraPos(GetCamera()->GetPos());
	Field_Update();
	UI_Update();
	Furniture_Update();
	Ghost_Update();
#if !STOP_TIMER_BUSTER
	Busters_Update();
#endif

	DebugDraw_Update();
}

void Game_Draw(void)
{
	SetDepthTest(true);

	Shader_SetAmbientLight(g_pAmbientLight);
	Shader_ClearPointLights();
	Ghost_SetLight();
	Busters_SetLight();
	Furniture_SetLight();

	Field_Draw();
	Busters_Draw();

	if (Field_GetCurrentFloor() == 2)
	{
		TutorialObject_Draw();
	}

	Furniture_Draw();
	Ghost_Draw();
	DebugDraw_Draw();

	SetDepthTest(false);

	Sprite_BeginDraw2D();
	UI_Draw();
	Minimap_Draw();

	if (Field_GetCurrentFloor() == 2)
	{
		UI_Tutorial_Draw();
	}
	UI_PauseMenu_Draw();

	Sprite_EndDraw2D();
}

void Game_Finalize(void)
{
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}

	if (g_pAmbientLight) {
		delete g_pAmbientLight;
		g_pAmbientLight = nullptr;
	}

	UI_PauseMenu_Finalize();
	UI_Tutorial_Finalize();

	Camera_Finalize();
	Ghost_Finalize();
	Field_Finalize();
	UI_Finalize();
	Furniture_Finalize();
	Minimap_Finalize();
	Busters_Finalize();
	DebugDraw_Finalize();

	TutorialObject_Finalize();
}