/*==============================================================================

   ポリゴン描画 [game.cpp]

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "main.h"
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
#include "ghost.h"
#include "furniture.h"
#include "fade.h"
#include "busters.h"
#include "debugdraw.h"
#include "sound.h"
#include "minimap.h"

Light* MainLight;
Light* g_pUILight = nullptr;
static SoundData* g_pBGM = nullptr;

static int g_NextFloorID = -1;

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Game_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	MainLight = new Light
	(TRUE,
		XMFLOAT4(0.0f, -1.0f, -1.0f, 1.0f),	//向き（左奥上方から照射）
		XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f),	//光の色（白、スペキュラ用に強めに）
		XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f)	//環境光（より暗めに）
	);

	// UI用ライト初期化（環境光のみ）
	g_pUILight = new Light(
		FALSE,
		XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f),
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
	);

	Camera_Initialize();
	Ghost_Initialize(pDevice, pContext);
	Field_Initialize(pDevice, pContext);
	UI_Initialize();
	Furniture_Initialize();
	Busters_Initialize();
	Minimap_Initialize();
	DebugDraw_Initialize();

	// BGM読み込み・再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}
}

void Game_Update(void)
{

	// ---------------------------------------------------
		// 1. フェード処理の監視
		// ---------------------------------------------------
	FADESTAT fadeState = GetFadeState();

	// フェードアウトが完了して「真っ暗(FADE_MAX)」になった瞬間
	if (fadeState == FADE_MAX)
	{
		// ここでマップを切り替える！
		if (g_NextFloorID != -1)
		{
			// マップ読み込み
			LoadMapData(g_NextFloorID);

			// プレイヤー位置の調整（例：スタート位置へ）
			GetGhost()->SetPos(XMFLOAT3(0, 0, 0));

			// IDリセット
			g_NextFloorID = -1;

			// 手動でフェードインを開始させる
			Fade_StartIn();
		}
		return; // マップロード中は他の更新をしない
	}

	// フェード中（IN/OUT）はゲーム操作をさせない
	if (fadeState != FADE_NONE)
	{
		return;
	}

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
	MainLight->SetEnable(true);
	Shader_SetLight(MainLight);

	//3D描画の前に深度テストを有効にする
	SetDepthTest(true);

	Field_Draw();
	Busters_Draw();
	Furniture_Draw();
	Ghost_Draw();
	DebugDraw_Draw();

	SetDepthTest(false);
	
	// UI描画時用のライト設定（環境光のみ）
	Shader_SetLight(g_pUILight);

	//2D描画処理をここに記述
	
	UI_Draw();
	Minimap_Draw();
}

void Game_Finalize(void)
{
	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}

	delete MainLight;
	if (g_pUILight) {
		delete g_pUILight;
		g_pUILight = nullptr;
	}

	Camera_Finalize();
	Ghost_Finalize();
	Field_Finalize();
	UI_Finalize();
	Furniture_Finalize();
	Minimap_Finalize();
	Busters_Finalize();
	DebugDraw_Finalize();
}
