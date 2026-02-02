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
#include "ghost.h"
#include "furniture.h"
#include "fade.h"
#include "busters.h"
#include "debugdraw.h"
#include "sound.h"
#include "minimap.h"
#include "light.h"
#include <windows.h>
#include <string> // to_string用

static AmbientLight* g_pAmbientLight = nullptr;
static SoundData* g_pBGM = nullptr;

static int g_NextFloorID = -1;

// ==========================================
// ポーズ画面用の変数
// ==========================================
static bool g_IsPause = false;
static int  g_PauseCursor = 0; // 0:再開, 1:音量, 2:明るさ, 3:タイトル
static Sprite* g_pPauseBG = nullptr;
static Sprite* g_pButtonResume = nullptr;
static Sprite* g_pButtonVolume = nullptr;
static Sprite* g_pButtonBright = nullptr;
static Sprite* g_pButtonTitle = nullptr;

// 設定値
static float g_Volume = 1.0f;
static float g_Brightness = 0.6f; // MainLightの環境光初期値に合わせる

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

	// ポーズ用初期化
	g_IsPause = false;
	g_PauseCursor = 0;
	g_Volume = 1.0f;
	g_Brightness = 0.6f;

	// 背景
	g_pPauseBG = new Sprite({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, { SCREEN_WIDTH, SCREEN_HEIGHT }, 0, { 0,0,0,0.7f }, BLENDSTATE_ALFA, L"asset/texture/fade.png");

	// ボタン（仮画像として button.PNG を使用）
	float bx = SCREEN_WIDTH / 2;
	float by = SCREEN_HEIGHT / 2 - 100;
	float gap = 70.0f;

	// Resume
	g_pButtonResume = new Sprite({ bx, by }, { 300, 60 }, 0, { 1,1,1,1 }, BLENDSTATE_ALFA, L"asset/texture/button.PNG");
	// Volume
	g_pButtonVolume = new Sprite({ bx, by + gap }, { 300, 60 }, 0, { 1,1,1,1 }, BLENDSTATE_ALFA, L"asset/texture/button.PNG");
	// Brightness
	g_pButtonBright = new Sprite({ bx, by + gap * 2 }, { 300, 60 }, 0, { 1,1,1,1 }, BLENDSTATE_ALFA, L"asset/texture/button.PNG");
	// Title
	g_pButtonTitle = new Sprite({ bx, by + gap * 3 }, { 300, 60 }, 0, { 1,1,1,1 }, BLENDSTATE_ALFA, L"asset/texture/button.PNG");
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
	// ポーズ処理
	// ========================================================
	if (Keyboard_IsKeyDownTrigger(KK_ESCAPE))
	{
		g_IsPause = !g_IsPause;
		g_PauseCursor = 0;
	}

	if (g_IsPause)
	{
		// カーソル移動
		if (Keyboard_IsKeyDownTrigger(KK_UP)) g_PauseCursor--;
		if (Keyboard_IsKeyDownTrigger(KK_DOWN)) g_PauseCursor++;
		if (g_PauseCursor < 0) g_PauseCursor = 3;
		if (g_PauseCursor > 3) g_PauseCursor = 0;

		// 左右キーで値変更（音量・明るさ）
		if (g_PauseCursor == 1) // Volume
		{
			if (Keyboard_IsKeyDown(KK_RIGHT)) g_Volume += 0.01f;
			if (Keyboard_IsKeyDown(KK_LEFT))  g_Volume -= 0.01f;
			if (g_Volume > 1.0f) g_Volume = 1.0f;
			if (g_Volume < 0.0f) g_Volume = 0.0f;
			SetMasterVolume(g_Volume); // 反映
		}
		else if (g_PauseCursor == 2) // Brightness
		{
			if (Keyboard_IsKeyDown(KK_RIGHT)) g_Brightness += 0.01f;
			if (Keyboard_IsKeyDown(KK_LEFT))  g_Brightness -= 0.01f;
			if (g_Brightness > 2.0f) g_Brightness = 2.0f;
			if (g_Brightness < 0.0f) g_Brightness = 0.0f;

			// MainLightのAmbientを調整して明るさを擬似変更
			if (MainLight) {
				MainLight->SetAmbient({ g_Brightness, g_Brightness, g_Brightness, 1.0f });
			}
		}

		// 決定操作
		if (Keyboard_IsKeyDownTrigger(KK_SPACE) || Keyboard_IsKeyDownTrigger(KK_ENTER))
		{
			if (g_PauseCursor == 0) g_IsPause = false; // Resume
			if (g_PauseCursor == 3) { g_IsPause = false; SetScene(SCENE_TITLE); } // Title
		}

		// ボタン色更新（選択中は赤、それ以外は白。調整項目は値に応じて青っぽく）
		g_pButtonResume->SetColor({ 1,1,1,1 });
		g_pButtonVolume->SetColor({ 1,1,1,1 }); // 値によって色を変えたい場合はここで
		g_pButtonBright->SetColor({ 1,1,1,1 });
		g_pButtonTitle->SetColor({ 1,1,1,1 });

		XMFLOAT4 selColor = { 1.0f, 0.5f, 0.5f, 1.0f }; // 選択色（赤）

		switch (g_PauseCursor) {
		case 0: g_pButtonResume->SetColor(selColor); break;
		case 1: g_pButtonVolume->SetColor({ 0.5f, 1.0f, 0.5f, 1.0f }); break; // 音量は緑
		case 2: g_pButtonBright->SetColor({ 0.5f, 0.5f, 1.0f, 1.0f }); break; // 明るさは青
		case 3: g_pButtonTitle->SetColor(selColor); break;
		}

		// バーなどで値を可視化したい場合は、ボタンのサイズを変えるなどの工夫が可能
		// 例：音量ボタンの横幅を音量に合わせて伸縮
		g_pButtonVolume->SetSize({ 300.0f * g_Volume, 60.0f });
		g_pButtonBright->SetSize({ 300.0f * (g_Brightness / 1.0f), 60.0f }); // 1.0基準で伸縮

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

	// ポーズ描画
	if (g_IsPause)
	{
		if (g_pPauseBG) g_pPauseBG->Draw();
		if (g_pButtonResume) g_pButtonResume->Draw();
		if (g_pButtonVolume) g_pButtonVolume->Draw();
		if (g_pButtonBright) g_pButtonBright->Draw();
		if (g_pButtonTitle) g_pButtonTitle->Draw();
	}

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

	Camera_Finalize();
	Ghost_Finalize();
	Field_Finalize();
	UI_Finalize();
	Furniture_Finalize();
	Minimap_Finalize();
	Busters_Finalize();
	DebugDraw_Finalize();
}