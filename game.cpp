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
	FLOOR_EXIT_CAM_LERP,    // 俯瞰→プレイヤー視点へカメラ補間
	FLOOR_EXIT_FADEIN,      // フェードイン中
};

static FLOOR_EXIT_ANIM_STATE g_FloorExitState = FLOOR_EXIT_NONE;
static int  g_FloorExitTimer      = 0;
static bool g_FloorTransferDone   = false; // フロア移行処理を実行済みか
static bool g_FloorExitAnimRequested = false; // アニメーション開始要求フラグ
static int  g_OverviewFrameCount  = 0;        // OVERVIEWステート経過フレーム数（即遷移防止）
static XMFLOAT3 g_OverviewCameraPos = { 0.0f, 0.0f, 0.0f };
static XMFLOAT3 g_StairsPosForAnim  = { 0.0f, 0.0f, 0.0f }; // アニメ開始時の階段座標（元フロア）
static int      g_FloorBeforeExit   = -1;                    // フロア移行前のフロア番号
static XMFLOAT3 g_LerpStartCamPos   = { 0.0f, 0.0f, 0.0f }; // 補間開始カメラ位置
static XMFLOAT3 g_LerpStartAtPos    = { 0.0f, 0.0f, 0.0f }; // 補間開始注視点
static float    g_CamLerpT          = 0.0f;                  // 補間進行度[0,1]
static const float CAM_LERP_SPEED   = 0.012f;                // 補間速度（1フレームあたり）

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

	// カメラをY+20の真上に置く。視線とUpベクトル(0,1,0)が平行にならないよう
	// atPos の Z を focusPos.z + 0.01f にして視線に僅かな傾きを与える
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
			// 俯瞰カメラをバスターズのX/Zに追従
			// UI_Update / Ghost_Update は呼ばない（タイマー・ゲージ減少・カメラ上書きを止めるため）
			{
				Busters_Update(); // バスターズが階段へ走るアニメーションを更新
				XMFLOAT3 focusPos = { 0.0f, 0.0f, 0.0f };
				Busters* buster = GetBusters();
				if (buster) focusPos = buster->GetPos();
				else { Ghost* ghost = GetGhost(); if (ghost) focusPos = ghost->GetPos(); }
				// 真上から見下ろす俯瞰カメラ。
				// 視線(camPos→atPos)が UpVec(0,1,0) と平行になると行列が壊れるため
				// atPos を少し前方(+Z)にずらして平行を回避する。
				XMFLOAT3 camPos = { focusPos.x, focusPos.y + 20.0f, focusPos.z };
				XMFLOAT3 atPos  = { focusPos.x, focusPos.y,         focusPos.z + 0.01f };
				Camera* cam = GetCamera();
				if (cam) cam->UpdateView(camPos, atPos);
				Shader_SetCameraPos(camPos);
			// バスターズが97（階段）に到達したらフロア移行してカメラ補間ステートへ
				// 最低1フレームは判定しない（StartFloorExitAnim直後の即trueを防ぐ）
				g_OverviewFrameCount++;
				if (g_OverviewFrameCount > 1 && Busters_IsFloorExitAnimDone())
				{
					Busters_DoFloorTransition();
					// 補間の開始点を現在の俯瞰カメラ位置で固定
					g_LerpStartCamPos = camPos;
					g_LerpStartAtPos  = atPos;
					g_CamLerpT        = 0.0f;
					g_OverviewFrameCount = 0;
					g_FloorExitState  = FLOOR_EXIT_CAM_LERP;
				}
			}
			return;

		case FLOOR_EXIT_CAM_LERP:
			// 俯瞰カメラ位置→プレイヤー視点へ線形補間し、完了後にフェードアウト開始
			{
				Busters_Update(); // フロア移行後の新フロアでバスターズを動かす
				g_CamLerpT += CAM_LERP_SPEED;
				if (g_CamLerpT > 1.0f) g_CamLerpT = 1.0f;
				Ghost* ghost = GetGhost();
				XMFLOAT3 gp = ghost ? ghost->GetPos() : XMFLOAT3(0.0f, 0.0f, 0.0f);
				// 補間先：プレイヤー視点カメラ
				XMFLOAT3 dstCamPos = { gp.x, gp.y + CAMERA_OFFSET_Y + 6.0f, gp.z - 0.01f };
				XMFLOAT3 dstAtPos  = { gp.x, gp.y + CAMERA_OFFSET_Y,        gp.z };
				// lerp
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
				// 補間完了でフェードアウト開始
				if (g_CamLerpT >= 1.0f && fadeState == FADE_NONE)
				{
					StartFade(SCENE_NONE);
					g_FloorExitState = FLOOR_EXIT_FADEIN;
				}
			}
			return;

		case FLOOR_EXIT_FADEIN:
			// フェードアウト（白くなる）中もプレイヤー視点を維持する
			{
				Ghost* ghost = GetGhost();
				XMFLOAT3 gp = ghost ? ghost->GetPos() : XMFLOAT3(0.0f, 0.0f, 0.0f);
				XMFLOAT3 camPos = { gp.x, gp.y + CAMERA_OFFSET_Y + 6.0f, gp.z - 0.01f };
				XMFLOAT3 atPos  = { gp.x, gp.y + CAMERA_OFFSET_Y,        gp.z };
				Camera* cam = GetCamera();
				if (cam) cam->UpdateView(camPos, atPos);
				Shader_SetCameraPos(camPos);
			}
		// フェードアウト完了後、勝利処理を実行
			if (fadeState == FADE_MAX)
			{
				// g_FloorBeforeExit は移行前フロア。DoFloorTransition 後は既にフロアが変わっているため
				// 移行前の値で勝利条件を判断する。
				if (g_FloorBeforeExit == END_FLOOR - 1 || g_FloorBeforeExit <= 1)
				{
					// クリア階 or 1階 → 勝利シーンへ遷移
					// SetScene より先にリセットする（SetScene後はGame_Updateが呼ばれない）
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
					// それ以外のフロア → カメラをプレイヤー視点に戻してフェードイン
					Ghost* ghost = GetGhost();
					if (ghost)
					{
						Camera_SetTargetPos(ghost->GetPos());
						Camera* cam = GetCamera();
						if (cam)
						{
							XMFLOAT3 gp = ghost->GetPos();
							XMFLOAT3 atPos  = { gp.x, gp.y + CAMERA_OFFSET_Y, gp.z };
							XMFLOAT3 camPos = { gp.x, gp.y + CAMERA_OFFSET_Y + 6.0f, gp.z - 0.01f };
							cam->UpdateView(camPos, atPos);
						}
					}
					// Fade_StartIn() の後にリセット。
					// 次フレームは fadeState=FADE_IN なので
					// if(fadeState != FADE_NONE) return に引っかかり
					// ゲージMAX が再トリガーされない。
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

	// ゲージMAX判定：俯瞰カメラへ切り替えて一定時間後に本来の処理（フロア移行 or 勝利）を実行
	if (g_FloorExitState == FLOOR_EXIT_NONE && GetFadeState() == FADE_NONE)
	{
		if (UI_IsScareGaugeMax())
		{
		// 俯瞰カメラを即座にセットしてOVERVIEWステートへ
			SetupOverviewCamera();
			g_FloorExitState = FLOOR_EXIT_OVERVIEW;
			g_FloorExitTimer = 0;
			g_OverviewFrameCount = 0;
		// マーカー97の座標へバスターズを走らせる
			g_FloorBeforeExit = Field_GetCurrentFloor(); // 移行前フロアを保存
			XMFLOAT3 stairsPos = Field_GetMarker97WorldPos(Field_GetCurrentFloor());
			Busters_StartFloorExitAnim(stairsPos);
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
