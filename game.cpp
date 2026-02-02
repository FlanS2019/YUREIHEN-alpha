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
#include "ghost.h"
#include "furniture.h"
#include "fade.h"
#include "busters.h"
#include "debugdraw.h"
#include "sound.h"
#include "minimap.h"
#include "mouse.h"
#include "light.h"
#include <windows.h>
#include <string> // to_string用

static AmbientLight* g_pAmbientLight = nullptr;
static SoundData* g_pBGM = nullptr;

static int g_NextFloorID = -1;

// ==========================================

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) return;

	g_pAmbientLight = new AmbientLight(XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));

	Camera_Initialize();
	Ghost_Initialize(pDevice, pContext);
	Field_Initialize(pDevice, pContext);
	UI_Initialize();
	Furniture_Initialize();
	Busters_Initialize();
	Minimap_Initialize();
	DebugDraw_Initialize();

	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) PlaySound(g_pBGM, true);

	// ポーズメニュー初期化
	UI_PauseMenu_Initialize(pDevice, pContext);
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
		return; // ゲーム更新停止
	}
	// ========================================================

	Ghost_Update();

	Camera_Update();
	Shader_SetCameraPos(GetCamera()->GetPos());
	Field_Update();
	UI_Update();
	Furniture_Update();
#if !STOP_TIMER_BUSTER
	Busters_Update();
#endif
	DebugDraw_Update();
}

void Game_Draw(void)
{
	//3D描画の前に深度テストを有効にする
	SetDepthTest(true);

	// ライト情報のセット
	Shader_SetAmbientLight(g_pAmbientLight);
	Ghost_SetLight();

	Field_Draw();
	Busters_Draw();
	Furniture_Draw();
	Ghost_Draw();
	DebugDraw_Draw();

	SetDepthTest(false);

	Sprite_BeginDraw2D();
	UI_Draw();
	Minimap_Draw();

	// ポーズメニュー描画
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

	Camera_Finalize();
	Ghost_Finalize();
	Field_Finalize();
	UI_Finalize();
	Furniture_Finalize();
	Minimap_Finalize();
	Busters_Finalize();
	DebugDraw_Finalize();
}