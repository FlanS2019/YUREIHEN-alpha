#pragma execution_character_set("utf-8")

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "UI_PauseMenu.h"
#include "keyboard.h"
#include "mouse.h"
#include "camera.h"
#include "scene.h"
#include "define.h"
#include "sound.h"
#include "fade.h"
#include "UI_Tutorial.h"
#include <windows.h>
#include <string>

// ==========================================
// ポーズ画面用の変数
// ==========================================
static bool g_IsPause = false;
static int  g_PauseCursor = 0; // 0:ゲームに戻る, 1:音量, 2:マウス感度, 3:明るさ, 4:タイトル, 5:チュートリアル
static Sprite* g_pPauseBG = nullptr;
static FontRenderer* g_pResumeButtonFont = nullptr;
static FontRenderer* g_pVolumeButtonFont = nullptr;
static FontRenderer* g_pMouseSensitivityButtonFont = nullptr;
static FontRenderer* g_pBrightnessButtonFont = nullptr;
static FontRenderer* g_pTitleButtonFont = nullptr;
static FontRenderer* g_pTutorialButtonFont = nullptr;

// 左右矢印用フォント
static FontRenderer* g_pLeftArrowFont = nullptr;
static FontRenderer* g_pRightArrowFont = nullptr;

// 設定値
static float g_Volume = 1.0f;
static float g_MouseSensitivity = 1.0f;
static float g_Brightness = 0.4f; // MainLightの環境光初期値に合わせる

// マウスカーソル状態フラグ
static bool g_PauseMouseStateChangedFlag = false;

// マウスクリック関連
struct MenuButton {
	float x, y;
	float width, height;
	int index;
};

static MenuButton g_MenuButtons[6] = {};

// 矢印表示オフセット定数
static const float ARROW_OFFSET_X = 150.0f; // 中央からの水平オフセット
static const float ARROW_HIT_MARGIN_X = 40.0f; // クリック判定のX軸マージン
static const float ARROW_HIT_MARGIN_Y = 30.0f; // クリック判定のY軸マージン

// キーボード操作フラグ：キーボードで操作されたらマウスを無視
static bool g_KeyboardUsed = false;
static float g_KeyboardIgnoreTimer = 0.0f;
static const float KEYBOARD_IGNORE_DURATION = 0.3f; // マウス無視期間（秒）
// ==========================================

void UI_PauseMenu_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) return;

	// ポーズ用初期化
	g_IsPause = false;
	g_PauseCursor = 0;
	g_Volume = 1.0f;
	g_MouseSensitivity = 1.0f;
	g_Brightness = 0.4f;
	g_PauseMouseStateChangedFlag = false;

	// カメラに初期マウス感度を反映
	Camera_SetSensitivity(g_MouseSensitivity);

	// 背景
	g_pPauseBG = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0,
		{ 0,0,0,0.7f },
		BLENDSTATE_ALFA,
		L"asset/texture/fade.png");

	// ボタン
	float bx = SCREEN_WIDTH / 2;
	float by = SCREEN_HEIGHT / 4;
	float gap = 70.0f;

	// Resume (ゲームに戻る)
	g_pResumeButtonFont = new FontRenderer({ bx, by }, 40.0f, 0.0f, { 1,1,1,1 }, "ゲームに戻る");
	g_MenuButtons[0] = { bx - 150.0f, by - 25.0f, 300.0f, 50.0f, 0 };

	// Volume
	g_pVolumeButtonFont = new FontRenderer({ bx, by + gap }, 40.0f, 0.0f, { 1,1,1,1 }, "音量");
	g_MenuButtons[1] = { bx - 200.0f, by + gap - 25.0f, 400.0f, 50.0f, 1 };

	// MouseSensitivity
	g_pMouseSensitivityButtonFont = new FontRenderer({ bx, by + gap * 2 }, 40.0f, 0.0f, { 1,1,1,1 }, "マウス感度");
	g_MenuButtons[2] = { bx - 280.0f, by + gap * 2 - 25.0f, 560.0f, 50.0f, 2 };

	// Brightness
	g_pBrightnessButtonFont = new FontRenderer({ bx, by + gap * 3 }, 40.0f, 0.0f, { 1,1,1,1 }, "明るさ");
	g_MenuButtons[3] = { bx - 200.0f, by + gap * 3 - 25.0f, 400.0f, 50.0f, 3 };

	// Title
	g_pTitleButtonFont = new FontRenderer({ bx, by + gap * 4 }, 40.0f, 0.0f, { 1,1,1,1 }, "タイトルへ戻る");
	g_MenuButtons[4] = { bx - 180.0f, by + gap * 4 - 25.0f, 360.0f, 50.0f, 4 };

	// Tutorial
	g_pTutorialButtonFont = new FontRenderer({ bx, by + gap * 5 }, 40.0f, 0.0f, { 1,1,1,1 }, "チュートリアルを見る");
	g_MenuButtons[5] = { bx - 240.0f, by + gap * 5 - 25.0f, 480.0f, 50.0f, 5 };

	// 左右矢印フォント（初期は透明）
	g_pLeftArrowFont = new FontRenderer({ bx - ARROW_OFFSET_X, by + gap }, 70.0f, 0.0f, { 1,1,1,0 }, "←");
	g_pRightArrowFont = new FontRenderer({ bx + ARROW_OFFSET_X, by + gap },70.0f, 0.0f, { 1,1,1,0 }, "→");
}

void UI_PauseMenu_Finalize(void)
{
	if (g_pPauseBG) {
		delete g_pPauseBG;
		g_pPauseBG = nullptr;
	}

	if (g_pResumeButtonFont) {
		delete g_pResumeButtonFont;
		g_pResumeButtonFont = nullptr;
	}

	if (g_pVolumeButtonFont) {
		delete g_pVolumeButtonFont;
		g_pVolumeButtonFont = nullptr;
	}

	if (g_pMouseSensitivityButtonFont) {
		delete g_pMouseSensitivityButtonFont;
		g_pMouseSensitivityButtonFont = nullptr;
	}

	if (g_pBrightnessButtonFont) {
		delete g_pBrightnessButtonFont;
		g_pBrightnessButtonFont = nullptr;
	}

	if (g_pTitleButtonFont) {
		delete g_pTitleButtonFont;
		g_pTitleButtonFont = nullptr;
	}

	if (g_pTutorialButtonFont) {
		delete g_pTutorialButtonFont;
		g_pTutorialButtonFont = nullptr;
	}

	// 左右矢印フォント
	if (g_pLeftArrowFont) {
		delete g_pLeftArrowFont;
		g_pLeftArrowFont = nullptr;
	}
	if (g_pRightArrowFont) {
		delete g_pRightArrowFont;
		g_pRightArrowFont = nullptr;
	}
}

// マウスクリックでボタン判定を行うヘルパー関数
static bool IsMouseInButton(const MenuButton& button, const Mouse_State& mouseState)
{
	return (mouseState.x >= button.x && mouseState.x <= button.x + button.width &&
			mouseState.y >= button.y && mouseState.y <= button.y + button.height);
}

void UI_PauseMenu_Update(void)
{
	// ESCキーでポーズ開始/終了
	if (Keyboard_IsKeyDownTrigger(KK_ESCAPE))
	{
		g_IsPause = !g_IsPause;
		g_PauseCursor = 0;
		g_KeyboardUsed = false;
		g_KeyboardIgnoreTimer = 0.0f;

		// ポーズ状態が変わったときにマウスカーソル状態を管理
		if (g_IsPause)
		{
			// ポーズ開始：マウスを絶対座標モード・表示
			Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
			ShowCursor(TRUE);
			g_PauseMouseStateChangedFlag = true;
		}
		else
		{
			// ポーズ終了：マウスを相対モード・非表示に戻す
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
			ShowCursor(FALSE);
			g_PauseMouseStateChangedFlag = false;
		}
	}

	if (!g_IsPause) return;

	// キーボード無視タイマーをデクリメント
	if (g_KeyboardIgnoreTimer > 0.0f)
	{
		g_KeyboardIgnoreTimer -= 1.0f / 60.0f; // 60FPS想定
	}
	else
	{
		g_KeyboardUsed = false;
	}

	// マウス状態取得
	Mouse_State mouseState;
	Mouse_GetState(&mouseState);

	// マウスホバーで選択状態を変更（キーボード操作後の無視期間中は無視）
	if (!g_KeyboardUsed)
	{
		for (int i = 0; i < 6; i++)
		{
			if (IsMouseInButton(g_MenuButtons[i], mouseState))
			{
				g_PauseCursor = i;
				break;
			}
		}
	}

	// マウスクリック処理
	if (mouseState.leftButton && !g_KeyboardUsed)
	{
		// 各メニューボタンのクリック判定
		for (int i = 0; i < 6; i++)
		{
			if (IsMouseInButton(g_MenuButtons[i], mouseState))
			{
				g_PauseCursor = i;

				// クリック判定直後に該当処理を実行
				if (i == 0) // Resume
				{
					g_IsPause = false;
					Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
					ShowCursor(FALSE);
					g_PauseMouseStateChangedFlag = false;
				}
				else if (i == 4) // Title
				{
					g_IsPause = false;
					StartFade(SCENE_TITLE);
				}
				else if (i == 5) // Tutorial
				{
					g_IsPause = false;
					UI_Tutorial_SetActive(true);
				}
				break;
			}
		}
	}

	// 数値変更用の左右矢印クリック処理（キーボード無視期間外で実行）
	if (mouseState.leftButton && !g_KeyboardUsed)
	{
		float by = SCREEN_HEIGHT / 4;
		float gap = 70.0f;
		float bx = SCREEN_WIDTH / 2;

		if (g_PauseCursor == 1) // Volume
		{
			float buttonCenterY = by + gap;

			// 左矢印クリック判定
			if (mouseState.x >= bx - ARROW_OFFSET_X - ARROW_HIT_MARGIN_X && mouseState.x <= bx - ARROW_OFFSET_X + ARROW_HIT_MARGIN_X &&
				mouseState.y >= buttonCenterY - ARROW_HIT_MARGIN_Y && mouseState.y <= buttonCenterY + ARROW_HIT_MARGIN_Y)
			{
				g_Volume -= 0.05f;
				if (g_Volume < 0.0f) g_Volume = 0.0f;
				SetMasterVolume(g_Volume);
			}
			// 右矢印クリック判定
			else if (mouseState.x >= bx + ARROW_OFFSET_X - ARROW_HIT_MARGIN_X && mouseState.x <= bx + ARROW_OFFSET_X + ARROW_HIT_MARGIN_X &&
					 mouseState.y >= buttonCenterY - ARROW_HIT_MARGIN_Y && mouseState.y <= buttonCenterY + ARROW_HIT_MARGIN_Y)
			{
				g_Volume += 0.05f;
				if (g_Volume > 1.0f) g_Volume = 1.0f;
				SetMasterVolume(g_Volume);
			}
		}
		else if (g_PauseCursor == 2) // Mouse Sensitivity
		{
			float buttonCenterY = by + gap * 2;

			// 左矢印クリック判定
			if (mouseState.x >= bx - ARROW_OFFSET_X - ARROW_HIT_MARGIN_X && mouseState.x <= bx - ARROW_OFFSET_X + ARROW_HIT_MARGIN_X &&
				mouseState.y >= buttonCenterY - ARROW_HIT_MARGIN_Y && mouseState.y <= buttonCenterY + ARROW_HIT_MARGIN_Y)
			{
				g_MouseSensitivity -= 0.05f;
				if (g_MouseSensitivity < 0.1f) g_MouseSensitivity = 0.1f;
				Camera_SetSensitivity(g_MouseSensitivity);
			}
			// 右矢印クリック判定
			else if (mouseState.x >= bx + ARROW_OFFSET_X - ARROW_HIT_MARGIN_X && mouseState.x <= bx + ARROW_OFFSET_X + ARROW_HIT_MARGIN_X &&
					 mouseState.y >= buttonCenterY - ARROW_HIT_MARGIN_Y && mouseState.y <= buttonCenterY + ARROW_HIT_MARGIN_Y)
			{
				g_MouseSensitivity += 0.05f;
				if (g_MouseSensitivity > 2.0f) g_MouseSensitivity = 2.0f;
				Camera_SetSensitivity(g_MouseSensitivity);
			}
		}
		else if (g_PauseCursor == 3) // Brightness
		{
			float buttonCenterY = by + gap * 3;

			// 左矢印クリック判定
			if (mouseState.x >= bx - ARROW_OFFSET_X - ARROW_HIT_MARGIN_X && mouseState.x <= bx - ARROW_OFFSET_X + ARROW_HIT_MARGIN_X &&
				mouseState.y >= buttonCenterY - ARROW_HIT_MARGIN_Y && mouseState.y <= buttonCenterY + ARROW_HIT_MARGIN_Y)
			{
				g_Brightness -= 0.05f;
				if (g_Brightness < 0.0f) g_Brightness = 0.0f;
			}
			// 右矢印クリック判定
			else if (mouseState.x >= bx + ARROW_OFFSET_X - ARROW_HIT_MARGIN_X && mouseState.x <= bx + ARROW_OFFSET_X + ARROW_HIT_MARGIN_X &&
					 mouseState.y >= buttonCenterY - ARROW_HIT_MARGIN_Y && mouseState.y <= buttonCenterY + ARROW_HIT_MARGIN_Y)
			{
				g_Brightness += 0.05f;
				if (g_Brightness > 1.0f) g_Brightness = 1.0f;
			}
		}
	}

	// カーソル移動（キーボード操作）
	if (Keyboard_IsKeyDownTrigger(KK_UP))
	{
		g_PauseCursor--;
		g_KeyboardUsed = true;
		g_KeyboardIgnoreTimer = KEYBOARD_IGNORE_DURATION;
	}
	if (Keyboard_IsKeyDownTrigger(KK_DOWN))
	{
		g_PauseCursor++;
		g_KeyboardUsed = true;
		g_KeyboardIgnoreTimer = KEYBOARD_IGNORE_DURATION;
	}
	if (g_PauseCursor < 0) g_PauseCursor = 5;
	if (g_PauseCursor > 5) g_PauseCursor = 0;

	// 左右キーで値変更（音量・マウス感度・明るさ）
	if (g_PauseCursor == 1) // Volume
	{
		if (Keyboard_IsKeyDown(KK_RIGHT))
		{
			g_Volume += 0.01f;
			g_KeyboardUsed = true;
			g_KeyboardIgnoreTimer = KEYBOARD_IGNORE_DURATION;
		}
		if (Keyboard_IsKeyDown(KK_LEFT))
		{
			g_Volume -= 0.01f;
			g_KeyboardUsed = true;
			g_KeyboardIgnoreTimer = KEYBOARD_IGNORE_DURATION;
		}
		if (g_Volume > 1.0f) g_Volume = 1.0f;
		if (g_Volume < 0.0f) g_Volume = 0.0f;
		SetMasterVolume(g_Volume); // 反映
		
		// テキスト更新
		int volumePercent = (int)(g_Volume * 100);
		std::string volumeText = "音量：" + std::to_string(volumePercent) + " ";
		if (g_pVolumeButtonFont) {
			g_pVolumeButtonFont->SetText(volumeText);
		}
	}
	else if (g_PauseCursor == 2) // Mouse Sensitivity
	{
		if (Keyboard_IsKeyDown(KK_RIGHT))
		{
			g_MouseSensitivity += 0.01f;
			g_KeyboardUsed = true;
			g_KeyboardIgnoreTimer = KEYBOARD_IGNORE_DURATION;
		}
		if (Keyboard_IsKeyDown(KK_LEFT))
		{
			g_MouseSensitivity -= 0.01f;
			g_KeyboardUsed = true;
			g_KeyboardIgnoreTimer = KEYBOARD_IGNORE_DURATION;
		}
		if (g_MouseSensitivity > 2.0f) g_MouseSensitivity = 2.0f;
		if (g_MouseSensitivity < 0.1f) g_MouseSensitivity = 0.1f;
		
		// カメラに反映
		Camera_SetSensitivity(g_MouseSensitivity);
		
		// テキスト更新
		int sensitivityPercent = (int)(g_MouseSensitivity * 100);
		std::string sensitivityText = "マウス感度：" + std::to_string(sensitivityPercent) + " ";
		if (g_pMouseSensitivityButtonFont) {
			g_pMouseSensitivityButtonFont->SetText(sensitivityText);
		}
	}
	else if (g_PauseCursor == 3) // Brightness
	{
		if (Keyboard_IsKeyDown(KK_RIGHT))
		{
			g_Brightness += 0.01f;
			g_KeyboardUsed = true;
			g_KeyboardIgnoreTimer = KEYBOARD_IGNORE_DURATION;
		}
		if (Keyboard_IsKeyDown(KK_LEFT))
		{
			g_Brightness -= 0.01f;
			g_KeyboardUsed = true;
			g_KeyboardIgnoreTimer = KEYBOARD_IGNORE_DURATION;
		}
		if (g_Brightness > 1.0f) g_Brightness = 1.0f;
		if (g_Brightness < 0.0f) g_Brightness = 0.0f;
		
		// テキスト更新
		int brightnessPercent = (int)(g_Brightness * 100);
		std::string brightnessText = "明るさ：" + std::to_string(brightnessPercent) + " ";
		if (g_pBrightnessButtonFont) {
			g_pBrightnessButtonFont->SetText(brightnessText);
		}
	}

	// 決定操作（キーボード）
	if (Keyboard_IsKeyDownTrigger(KK_SPACE) || Keyboard_IsKeyDownTrigger(KK_ENTER))
	{
		if (g_PauseCursor == 0)
		{
			// Resume：マウスを相対モード・非表示に戻す
			g_IsPause = false;
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
			ShowCursor(FALSE);
			g_PauseMouseStateChangedFlag = false;
		}
		if (g_PauseCursor == 4)
		{
			// Title
			g_IsPause = false;
			StartFade(SCENE_TITLE);
		}
		if (g_PauseCursor == 5)
		{
			// Tutorial
			g_IsPause = false;
			UI_Tutorial_SetActive(true);
		}
	}

	// テキスト更新（マウスクリック後の反映）
	if (g_pVolumeButtonFont)
	{
		int volumePercent = (int)(g_Volume * 100);
		std::string volumeText = "音量：" + std::to_string(volumePercent) + " ";
		g_pVolumeButtonFont->SetText(volumeText);
	}

	if (g_pMouseSensitivityButtonFont)
	{
		int sensitivityPercent = (int)(g_MouseSensitivity * 100);
		std::string sensitivityText = "マウス感度：" + std::to_string(sensitivityPercent) + " ";
		g_pMouseSensitivityButtonFont->SetText(sensitivityText);
	}

	if (g_pBrightnessButtonFont)
	{
		int brightnessPercent = (int)(g_Brightness * 100);
		std::string brightnessText = "明るさ：" + std::to_string(brightnessPercent) + " ";
		g_pBrightnessButtonFont->SetText(brightnessText);
	}

	// ボタン色更新
	if (g_pResumeButtonFont) {
		g_pResumeButtonFont->SetColor({ 1,1,1,1 });
	}
	if (g_pVolumeButtonFont) {
		g_pVolumeButtonFont->SetColor({ 1,1,1,1 });
	}
	if (g_pMouseSensitivityButtonFont) {
		g_pMouseSensitivityButtonFont->SetColor({ 1,1,1,1 });
	}
	if (g_pBrightnessButtonFont) {
		g_pBrightnessButtonFont->SetColor({ 1,1,1,1 });
	}
	if (g_pTitleButtonFont) {
		g_pTitleButtonFont->SetColor({ 1,1,1,1 });
	}
	if (g_pTutorialButtonFont) {
		g_pTutorialButtonFont->SetColor({ 1,1,1,1 });
	}

	XMFLOAT4 selColor = { 1.0f, 1.0f, 0.5f, 1.0f }; // 選択色（黄色）

	float bx = SCREEN_WIDTH / 2;
	float by = SCREEN_HEIGHT / 4;
	float gap = 70.0f;

	switch (g_PauseCursor) {
	case 0: 
		if (g_pResumeButtonFont) {
			g_pResumeButtonFont->SetColor(selColor);
		}
		break;
	case 1: 
		if (g_pVolumeButtonFont) {
			g_pVolumeButtonFont->SetColor(selColor);
		}
		// 矢印の表示更新（音量時のみ表示）
		if (g_pLeftArrowFont) {
			g_pLeftArrowFont->SetPos({ bx - ARROW_OFFSET_X, by + gap });
			// 最小値に達したら左矢印は非表示
			if (g_Volume <= 0.0f) {
				g_pLeftArrowFont->SetColor({ 1,1,1,0 });
			} else {
				g_pLeftArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		if (g_pRightArrowFont) {
			g_pRightArrowFont->SetPos({ bx + ARROW_OFFSET_X, by + gap });
			// 最大値に達したら右矢印は非表示
			if (g_Volume >= 1.0f) {
				g_pRightArrowFont->SetColor({ 1,1,1,0 });
			} else {
				g_pRightArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		break;
	case 2: 
		if (g_pMouseSensitivityButtonFont) {
			g_pMouseSensitivityButtonFont->SetColor(selColor);
		}
		// 矢印の表示更新（マウス感度時のみ表示）
		if (g_pLeftArrowFont) {
			g_pLeftArrowFont->SetPos({ bx - ARROW_OFFSET_X, by + gap * 2 });
			// 最小値に達したら左矢印は非表示
			if (g_MouseSensitivity <= 0.1f) {
				g_pLeftArrowFont->SetColor({ 1,1,1,0 });
			} else {
				g_pLeftArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		if (g_pRightArrowFont) {
			g_pRightArrowFont->SetPos({ bx + ARROW_OFFSET_X, by + gap * 2 });
			// 最大値に達したら右矢印は非表示
			if (g_MouseSensitivity >= 2.0f) {
				g_pRightArrowFont->SetColor({ 1,1,1,0 });
			} else {
				g_pRightArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		break;
	case 3: 
		if (g_pBrightnessButtonFont) {
			g_pBrightnessButtonFont->SetColor(selColor);
		}
		// 矢印の表示更新（明るさ時のみ表示）
		if (g_pLeftArrowFont) {
			g_pLeftArrowFont->SetPos({ bx - ARROW_OFFSET_X, by + gap * 3 });
			// 最小値に達したら左矢印は非表示
			if (g_Brightness <= 0.0f) {
				g_pLeftArrowFont->SetColor({ 1,1,1,0 });
			} else {
				g_pLeftArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		if (g_pRightArrowFont) {
			g_pRightArrowFont->SetPos({ bx + ARROW_OFFSET_X, by + gap * 3 });
			// 最大値に達したら右矢印は非表示
			if (g_Brightness >= 2.0f) {
				g_pRightArrowFont->SetColor({ 1,1,1,0 });
			} else {
				g_pRightArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		break;
	case 4: 
		if (g_pTitleButtonFont) {
			g_pTitleButtonFont->SetColor(selColor);
		}
		break;
	case 5:
		if (g_pTutorialButtonFont) {
			g_pTutorialButtonFont->SetColor(selColor);
		}
		break;
	}
}

void UI_PauseMenu_Draw(void)
{
	if (!g_IsPause) return;

	if (g_pPauseBG) g_pPauseBG->Draw();
	if (g_pResumeButtonFont) g_pResumeButtonFont->Draw();
	if (g_pVolumeButtonFont) g_pVolumeButtonFont->Draw();
	if (g_pMouseSensitivityButtonFont) g_pMouseSensitivityButtonFont->Draw();
	if (g_pBrightnessButtonFont) g_pBrightnessButtonFont->Draw();
	if (g_pTitleButtonFont) g_pTitleButtonFont->Draw();
	if (g_pTutorialButtonFont) g_pTutorialButtonFont->Draw();

	// 左右矢印（音量・マウス感度・明るさ変更用）
	if (g_PauseCursor == 1 || g_PauseCursor == 2 || g_PauseCursor == 3)
	{
		if (g_pLeftArrowFont) g_pLeftArrowFont->Draw();
		if (g_pRightArrowFont) g_pRightArrowFont->Draw();
	}
}

bool UI_PauseMenu_IsPaused(void)
{
	return g_IsPause;
}

void UI_PauseMenu_SetPause(bool isPause)
{
	g_IsPause = isPause;
	g_PauseCursor = 0;

	if (g_IsPause)
	{
		Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
		ShowCursor(TRUE);
		g_PauseMouseStateChangedFlag = true;
	}
	else
	{
		Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		ShowCursor(FALSE);
		g_PauseMouseStateChangedFlag = false;
	}
}

float UI_PauseMenu_GetBrightness(void)
{
	return g_Brightness;
}
