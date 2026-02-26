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
#include <math.h>

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

// ロゴアニメーション用定数
static const float LOGO_ANIM_DURATION = 0.6f;	// 縮小アニメの秒数
static const float LOGO_FINAL_W = 640.0f;		// ロゴの最終幅
static const float LOGO_FINAL_H = 354.0f;		// ロゴの最終高さ

// ロゴ自動フェードまでの時間（秒）
static const float LOGO_AUTO_FADE_TIME = 3.0f; // 必要に応じて秒数を調整

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Win Animation (勝ちアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sprite* g_WinSprite = nullptr;
Sprite* g_pWinAnimGhost = nullptr;		// 新規：背景スプライト（WinAnimGhost)
Sprite* g_pWinAnimBasuta = nullptr;		// 新規：エフェクトスプライト（WinAnimEffect)
Sprite* g_pWinAnimLogo = nullptr;

void Animation_Win_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_WinSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ SCREEN_WIDTH,SCREEN_HEIGHT },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\WinAnim\\WinAnimBG.png"
	);

	g_pWinAnimGhost = new Sprite(
		{ SCREEN_WIDTH / 2 - 300.0f, SCREEN_HEIGHT / 2 },
		{ 500,500 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\WinAnim\\WinAnimGhost.png"
	);

	g_pWinAnimBasuta = new Sprite(
		{ SCREEN_WIDTH / 2 + 50.0f, SCREEN_HEIGHT / 2 },
		{ 1080,1080 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\WinAnim\\WinAnimBasuta.png"
	);

	// ロゴは最初に画面全体サイズで表示
	g_pWinAnimLogo = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\WinAnim\\WinAnime2.png"
	);

	// サウンド再生
	g_pBGM = LoadMP3("asset/sound/se/Fluffy_SE.m4a");
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

	// --- Ghost：上下に浮かせるアニメーション ---
	if (g_pWinAnimGhost)
	{
		const float GHOST_BASE_Y   = SCREEN_HEIGHT / 2.0f;
		const float GHOST_FLOAT_AMP   = 20.0f;	// 振れ幅（px）
		const float GHOST_FLOAT_SPEED = 2.0f;	// 周期（rad/s）
		float ghostY = GHOST_BASE_Y + sinf(elapsedSeconds * GHOST_FLOAT_SPEED) * GHOST_FLOAT_AMP;
		g_pWinAnimGhost->SetPosY(ghostY);
	}

	// --- Basuta：横幅を引き伸ばすばてたアニメーション ---
	if (g_pWinAnimBasuta)
	{
		const float BASUTA_BASE_W     = 1080.0f;
		const float BASUTA_BASE_H     = 1080.0f;
		const float BASUTA_STRETCH_AMP   = 80.0f;	// 横引き伸ばし量（px）
		const float BASUTA_SQUASH_AMP    = 40.0f;	// 縦縮み量（px）
		const float BASUTA_ANIM_SPEED    = 1.2f;	// 周期（rad/s）
		float t = sinf(elapsedSeconds * BASUTA_ANIM_SPEED);
		float basutaW = BASUTA_BASE_W + t * BASUTA_STRETCH_AMP;
		float basutaH = BASUTA_BASE_H - fabsf(t) * BASUTA_SQUASH_AMP;
		g_pWinAnimBasuta->SetSize({ basutaW, basutaH });
	}

	// --- Logo：画面全体から縮小して中央へ ---
	if (g_pWinAnimLogo)
	{
		float t = elapsedSeconds / LOGO_ANIM_DURATION;
		if (t > 1.0f) t = 1.0f;
		// イーズアウト（1-(1-t)^2）
		float ease = 1.0f - (1.0f - t) * (1.0f - t);
		float logoW = SCREEN_WIDTH  + (LOGO_FINAL_W - SCREEN_WIDTH)  * ease;
		float logoH = SCREEN_HEIGHT + (LOGO_FINAL_H - SCREEN_HEIGHT) * ease;
		g_pWinAnimLogo->SetSize({ logoW, logoH });
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
	if(elapsedSeconds >= LOGO_AUTO_FADE_TIME)
	{
		// ★ 自動遷移時もResult画面に値を渡す
		if (g_HasResultData)
		{
			Result_SetTimerValue(g_ResultTimeValue);
			Result_SetCombo(g_ResultComboValue);
			
			// デバッグ出力
			hal::dout << "Passed to Result (auto): time=" << g_ResultTimeValue << ", combo=" << g_ResultComboValue << std::endl;
		}
		
		StartFade(SCENE_RESULT);
	}
}

void Animation_Win_Draw(void)
{
	g_WinSprite->Draw();
	g_pWinAnimBasuta->Draw();
	g_pWinAnimGhost->Draw();
	g_pWinAnimLogo->Draw();
}

void Animation_Win_Finalize(void)
{
	delete g_WinSprite;
	g_WinSprite = nullptr;
	delete g_pWinAnimGhost;
	g_pWinAnimGhost = nullptr;
	delete g_pWinAnimBasuta;
	g_pWinAnimBasuta = nullptr;
	delete g_pWinAnimLogo;
	g_pWinAnimLogo = nullptr;

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