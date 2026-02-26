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
	FLOOR_EXIT_NONE = 0,         // 通常状態
	FLOOR_EXIT_FADEOUT,          // フェードアウト中（未使用）
	FLOOR_EXIT_OVERVIEW,         // 俯瞰カメラ + バスターズ走行中
	FLOOR_EXIT_CAM_LERP,         // 俯瞰→プレイヤー視点へカメラ補間
	FLOOR_EXIT_PLAYER_WALK,      // プレイヤー操作に戻し階段を待つ
	FLOOR_EXIT_FADEIN,           // フェードイン中（フロア移行フェード）
};

static FLOOR_EXIT_ANIM_STATE g_FloorExitState = FLOOR_EXIT_NONE;
static int  g_FloorExitTimer      = 0;
static bool g_FloorTransferDone   = false;
static bool g_FloorExitAnimRequested = false;
static int  g_OverviewFrameCount  = 0;
static XMFLOAT3 g_OverviewCameraPos = { 0.0f, 0.0f, 0.0f };
static XMFLOAT3 g_StairsPosForAnim  = { 0.0f, 0.0f, 0.0f };
static int      g_FloorBeforeExit   = -1;
static XMFLOAT3 g_LerpStartCamPos   = { 0.0f, 0.0f, 0.0f };
static XMFLOAT3 g_LerpStartAtPos    = { 0.0f, 0.0f, 0.0f };
static float    g_CamLerpT          = 0.0f;
static const float CAM_LERP_SPEED   = 0.012f;

// バスターズの真上に俯瞰カメラをセット
static void SetupOverviewCamera(void)
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

	XMFLOAT3 camPos  = { focusPos.x, focusPos.y + 20.0f, focusPos.z };
	XMFLOAT3 atPos   = { focusPos.x, focusPos.y,         focusPos.z + 0.01f };
	g_OverviewCameraPos = camPos;

	Camera* cam = GetCamera();
	if (cam) cam->UpdateView(camPos, atPos);
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
		fadeState = GetFadeState();

		switch (g_FloorExitState)
		{
		case FLOOR_EXIT_OVERVIEW:
			// 俯瞰カメラでバスターズを追いかけ、到着後に削除してカメラ補間へ
			{
				Busters_Update();
				XMFLOAT3 focusPos = { 0.0f, 0.0f, 0.0f };
				Busters* buster = GetBusters();
				if (buster) focusPos = buster->GetPos();
				else { Ghost* ghost = GetGhost(); if (ghost) focusPos = ghost->GetPos(); }
				XMFLOAT3 camPos = { focusPos.x, focusPos.y + 20.0f, focusPos.z };
				XMFLOAT3 atPos  = { focusPos.x, focusPos.y,         focusPos.z + 0.01f };
				Camera* cam = GetCamera();
				if (cam) cam->UpdateView(camPos, atPos);
				Shader_SetCameraPos(camPos);
				g_OverviewFrameCount++;
				if (g_OverviewFrameCount > 1 && Busters_IsFloorExitAnimDone())
				{
					// バスターズが階段に到着したら削除
					Busters_DeleteCurrentFloor();
					g_LerpStartCamPos = camPos;
					g_LerpStartAtPos  = atPos;
					g_CamLerpT        = 0.0f;
					g_OverviewFrameCount = 0;
					g_FloorExitState  = FLOOR_EXIT_CAM_LERP;
				}
			}
			return;

		case FLOOR_EXIT_CAM_LERP:
			// 俯瞰位置からプレイヤー視点へ補間。完了したらプレイヤー操作に戻す
			{
				g_CamLerpT += CAM_LERP_SPEED;
				if (g_CamLerpT > 1.0f) g_CamLerpT = 1.0f;
				Ghost* ghost = GetGhost();
				XMFLOAT3 gp = ghost ? ghost->GetPos() : XMFLOAT3(0.0f, 0.0f, 0.0f);
				XMFLOAT3 dstCamPos = { gp.x, gp.y + CAMERA_OFFSET_Y + 6.0f, gp.z - 0.01f };
				XMFLOAT3 dstAtPos  = { gp.x, gp.y + CAMERA_OFFSET_Y,        gp.z };
				XMFLOAT3 camPos =
				{
					g_LerpStartCamPos.x + (dstCamPos.x - g_LerpStartCamPos.x) * g_CamLerpT,
					g_LerpStartCamPos.y + (dstCamPos.y - g_LerpStartCamPos.y) * g_CamLerpT,
					g_LerpStartCamPos.z + (dstCamPos.z - g_LerpStartCamPos.z) * g_CamLerpT
				};
				XMFLOAT3 atPos =
				{
					g_LerpStartAtPos.x + (dstAtPos.x - g_LerpStartAtPos.x) * g_CamLerpT,
					g_LerpStartAtPos.y + (dstAtPos.y - g_LerpStartAtPos.y) * g_CamLerpT,
					g_LerpStartAtPos.z + (dstAtPos.z - g_LerpStartAtPos.z) * g_CamLerpT
				};
				Camera* cam = GetCamera();
				if (cam) cam->UpdateView(camPos, atPos);
				Shader_SetCameraPos(camPos);
				if (g_CamLerpT >= 1.0f)
				{
					// 補間完了 → プレイヤー操作に戻して階段を待つ
					g_FloorExitState = FLOOR_EXIT_PLAYER_WALK;
				}
			}
			return;

		case FLOOR_EXIT_PLAYER_WALK:
			// プレイヤーが操作して階段(ID5/6)の上に乗ったらフェードして移行
			{
				Camera_Update();
				Shader_SetCameraPos(GetCamera()->GetPos());
				Field_Update();
				UI_Update();
				Furniture_Update();
				Ghost_Update();

				Ghost* ghost = GetGhost();
				if (ghost && fadeState == FADE_NONE)
				{
					XMFLOAT3 gp = ghost->GetPos();
					int blockID = Field_GetRawBlockID(gp.x, gp.z);
					if (blockID == 5 || blockID == 6)
					{
						StartFade(SCENE_NONE);
						g_FloorExitState = FLOOR_EXIT_FADEIN;
					}
				}
			}
			return;

		case FLOOR_EXIT_FADEIN:
			// フェードアウト完了後にフロア移行＋バスターズ生成してフェードイン
			if (fadeState == FADE_MAX)
			{
				if (g_FloorBeforeExit == END_FLOOR - 1 || g_FloorBeforeExit == 0)
				{
					// クリア階 or 1階 → 勝利シーンへ遷移
					g_FloorExitState     = FLOOR_EXIT_NONE;
					g_FloorExitTimer     = 0;
					g_OverviewFrameCount = 0;
					g_FloorTransferDone  = false;
					g_FloorExitAnimRequested = false;
					g_FloorBeforeExit    = -1;
					SetScene(SCENE_ANM_WIN);
				}
				else
				{
					// フロア移行：プレイヤーを下の階へ移動
					int nextFloor = g_FloorBeforeExit - 1;
					Ghost* ghost = GetGhost();
					if (ghost)
					{
						XMFLOAT3 ghostPos = ghost->GetPos();
						Field_ChangeFloor(nextFloor);
						ghost->SetPos(ghostPos);
						Camera_SetTargetPos(ghostPos);
					}
					// 下の階にバスターズを生成
					Busters_SpawnOnFloor(nextFloor);
					UI_ResetScareGauge();
					AddScareGauge(BUSTERS_DEFOURT_GAUGE);
					Fade_StartIn();
					g_FloorExitState     = FLOOR_EXIT_NONE;
					g_FloorExitTimer     = 0;
					g_FloorTransferDone  = false;
					g_FloorExitAnimRequested = false;
					g_FloorBeforeExit    = -1;
				}
			}
			return;

		default:
			break;
		}
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

	// ゲージMAX判定：MAXになったらバスターズを階段へ走らせる
	if (g_FloorExitState == FLOOR_EXIT_NONE && GetFadeState() == FADE_NONE)
	{
		if (UI_IsScareGaugeMax())
		{
			SetupOverviewCamera();
			g_FloorExitState = FLOOR_EXIT_OVERVIEW;
			g_FloorExitTimer = 0;
			g_OverviewFrameCount = 0;
			g_FloorBeforeExit = Field_GetCurrentFloor();
			Busters_StartFloorExitAnim();
		}
	}

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
