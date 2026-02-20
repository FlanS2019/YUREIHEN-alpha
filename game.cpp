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
#include "Tutorial_Bustars.h"
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

// ==========================================
// チュートリアル用 enban (円盤) モデル
// ==========================================
static MODEL* g_pEnbanModel = nullptr;
static bool     g_EnbanTouched = false;  // 既に触れたか（一度だけ通知）

// 配置位置
static const XMFLOAT3 ENBAN_POS = { -5.0f, 0.5f, -10.0f };
static const XMFLOAT3 ENBAN_ROT = { 0.0f, 0.0f,   0.0f };
static const XMFLOAT3 ENBAN_SCALE = { 1.0f, 1.0f,   1.0f };
static const float    ENBAN_TOUCH_RADIUS = 0.8f; // プレイヤーが円盤に触れたとみなす距離

static void Enban_Initialize()
{
	g_pEnbanModel = ModelLoad("asset\\model\\enban.fbx");
	g_EnbanTouched = false;
}

static void Enban_Finalize()
{
	if (g_pEnbanModel)
	{
		ModelRelease(g_pEnbanModel);
		g_pEnbanModel = nullptr;
	}
	g_EnbanTouched = false;
}

bool* Game_GetEnbanTouchedPtr(void)
{
	return &g_EnbanTouched;
}

// 毎フレーム：プレイヤー（Ghost）と円盤の距離を判定
static void Enban_Update()
{
	if (!g_pEnbanModel) return;
	if (g_EnbanTouched) return;
	// 待機中のときだけ判定する
	if (!UI_Tutorial_IsWaiting()) return;

	Ghost* pGhost = GetGhost();
	if (!pGhost) return;

	XMFLOAT3 gPos = pGhost->GetPos();
	float dx = gPos.x - ENBAN_POS.x;
	float dz = gPos.z - ENBAN_POS.z;
	float dist = sqrtf(dx * dx + dz * dz);

	if (dist <= ENBAN_TOUCH_RADIUS)
	{
		g_EnbanTouched = true;
		// UI_Tutorial_ResumeFromWait() は不要（AddTestPlayがフラグを直接監視）
	}
}

static void Enban_Draw()
{
	if (!g_pEnbanModel) return;
	// チュートリアル3階（フロアインデックス2）にいるときだけ描画
	if (Field_GetCurrentFloor() != 2) return;

	ModelDraw(g_pEnbanModel, ENBAN_POS, ENBAN_ROT, ENBAN_SCALE);
}

// ==========================================

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

	TutorialBusters_Initialize({ 0.0f, PATROL_HEIGHT, 0.0f });

	// チュートリアル円盤の初期化
	Enban_Initialize();

	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) PlaySound(g_pBGM, true);

	UI_PauseMenu_Initialize(pDevice, pContext);
	UI_Tutorial_Initialize(pDevice, pContext);
}

void Game_Update(void)
{
	FADESTAT fadeState = GetFadeState();
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

	// ========================================================
// ポーズメニュー更新
// ========================================================
	UI_PauseMenu_Update();
	if (UI_PauseMenu_IsPaused())
	{
		return;
	}


	// ========================================================
	// チュートリアル更新（IsWaiting中も Update を通す）
	// ========================================================
	UI_Tutorial_Update();

	// 条件待機中：ゲームは動かすが円盤判定のみ行う
	if (UI_Tutorial_IsWaiting())
	{
		Camera_Update();
		Shader_SetCameraPos(GetCamera()->GetPos());
		Ghost_Update();
		Enban_Update();
		return;
	}

	if (UI_Tutorial_IsActive())
	{
		return; // チュートリアルUI表示中はゲーム更新停止
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

	if (Field_GetCurrentFloor() == 2)
	{
		TutorialBusters_Update();
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
		TutorialBusters_Draw();
	}

	// チュートリアル円盤の描画
	Enban_Draw();

	Furniture_Draw();
	Ghost_Draw();
	DebugDraw_Draw();

	SetDepthTest(false);

	Sprite_BeginDraw2D();
	UI_Draw();
	Minimap_Draw();

	UI_Tutorial_Draw();
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

	TutorialBusters_Finalize();
	Enban_Finalize();
}