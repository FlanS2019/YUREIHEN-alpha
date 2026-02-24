#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "sound.h"
#include "WinAnim.h"
#include "mouse.h"
#include <timeapi.h>
#include "define.h"
#include "result.h"	// 追加：Winアニメ終了後に結果画面へ値を渡すため
#pragma comment(lib, "winmm.lib")

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
//sound
static SoundData* g_pBGM = nullptr;

// 勝利結果の保存
static bool g_HasResultData = false;
static float g_ResultTimeValue = 0.0f;
static int g_ResultComboValue = 0;

void WinAnim_SetResultData(float time, int combo)
{
	g_ResultTimeValue = time;
	g_ResultComboValue = combo;
	g_HasResultData = true;
	
	// デバッグ出力：値が正しく渡されたか確認
	hal::dout << "WinAnim_SetResultData called: time=" << time << ", combo=" << combo << std::endl;
}

// Winアニメ用タイマー
static DWORD g_WinStartTime = 0;

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Win Animation (勝ちアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sprite* g_WinSprite = nullptr;

void Animation_Win_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_WinSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1028,720 },			// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\WinAnim\\WinAnime.png"				// テクスチャパス
	);

	// サウンド再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}

	// アニメーション画面ではマウスカーソルを表示・絶対モードに設定
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);

	// タイマー開始
	g_WinStartTime = timeGetTime();
}

void Animation_Win_Update(void)
{
	// 経過時間取得（秒）
	float elapsedSeconds = 0.0f;
	if (g_WinStartTime != 0) {
		DWORD current = timeGetTime();
		elapsedSeconds = (current - g_WinStartTime) / 1000.0f;
	}

	// 3秒経過で自動的にリザルトへ遷移
	if (elapsedSeconds >= 3.0f)
	{
		// ★ 結果データをResult画面に渡す
		if (g_HasResultData)
		{
			Result_SetTimerValue(g_ResultTimeValue);
			Result_SetCombo(g_ResultComboValue);
			
			// デバッグ出力
			hal::dout << "Passed to Result: time=" << g_ResultTimeValue << ", combo=" << g_ResultComboValue << std::endl;
		}
		
		StartFade(SCENE_RESULT);
		return;
	}

	// ENTERキーで直接リザルトへ遷移（スキップ）
	if (Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		// ★ スキップ時もResult画面に値を渡す
		if (g_HasResultData)
		{
			Result_SetTimerValue(g_ResultTimeValue);
			Result_SetCombo(g_ResultComboValue);
			
			// デバッグ出力
			hal::dout << "Passed to Result (skip): time=" << g_ResultTimeValue << ", combo=" << g_ResultComboValue << std::endl;
		}
		
		StartFade(SCENE_RESULT);
	}
}

void Animation_Win_Draw(void)
{
	g_WinSprite->Draw();
}

void Animation_Win_Finalize(void)
{
	delete g_WinSprite;
	g_WinSprite = nullptr;

	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}

	// タイマーリセット
	g_WinStartTime = 0;
	g_HasResultData = false;
}