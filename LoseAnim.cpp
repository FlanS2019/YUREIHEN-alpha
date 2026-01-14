#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "LoseAnim.h"
#include "sound.h"
#include <timeapi.h>
#include <cmath>	// 揺れ用の sinf（揺れを消す場合は残しても問題ありません）
#pragma comment(lib, "winmm.lib")

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Lose Animation (負けアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sprite* g_LoseBgSprite = nullptr;		// 背景（Losehaikei）
Sprite* g_LoseGhostSprite = nullptr;	// ゴーストエフェクト（LoseGhost）
Sprite* g_LoseInkSprite = nullptr;		// インク画像（Loseink）
Sprite* g_LoseAnimeLogoSprite = nullptr;		// インク画像（LoseAnimeLogo）
Sprite* g_LoseVinetSprite = nullptr;	// 新規：ビネットオーバーレイ（LoseVinet）※追加

static DWORD g_LoseStartTime = 0;
static const float GHOST_APPEAR_TIME = 0.8f;	// ゴースト表示開始時間（秒）
static const float GHOST_FADE_DURATION = 0.6f; // ゴーストのフェードインにかける時間（秒）
static const float INK_DROP_START_TIME = 1.2f;	// インク降下開始時間（秒）
static const float INK_DROP_DURATION = 1.0f;	// インク降下時間（秒）
static float g_LoseInkInitialY = 0.0f;	// インク画像の初期Y座標

// インクスプライトのベース位置（Updateで揺らすために保持）
static float g_LoseInkBaseX = 0.0f;
static float g_LoseInkBaseY = 0.0f;

// （垂れ幕用パラメータは残すが、今回は使用しない）
static const float INK_HANG_START_TIME = INK_DROP_START_TIME;
static const float INK_HANG_DURATION = 1.0f;
static float g_LoseInkFullHeight = 1080.0f;
static float g_LoseInkTopY = 0.0f;

static SoundData* g_pBGM = nullptr;

void Animation_Lose_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 背景スプライト
	g_LoseBgSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1280, 1080 },								// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\LoseAnim\\Losehaikei.png"		// テクスチャパス
	);

	// ビネットオーバーレイ（LoseVinet）を生成
	// 画面サイズに合わせ、少し透過させてオーバーレイにする
	g_LoseVinetSprite = new Sprite(
		{ SCREEN_WIDTH/2, SCREEN_HEIGHT/2},	// 位置（画面中央）
		{ 1280, 1080 },			// サイズ（画面サイズに合わせる）
		0.0f,										// 回転
		{ 1.0f, 1.0f, 1.0f, 0.85f },				// 色（アルファを少し下げる）
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\LoseAnim\\LoseVinet.PNG"	// テクスチャパス
	);

	// ゴーストスプライト（初期は透明）
	float ghostNativePixelSize = 1028.0f;
	float ghostScaleFactor = 0.4f;
	float ghostDisplaySize = ghostNativePixelSize * ghostScaleFactor;

	g_LoseGhostSprite = new Sprite(
		{ SCREEN_WIDTH * 0.47f, SCREEN_HEIGHT * 0.45f },	// 位置（光の中央）
		{ 1028, 1028 },				// サイズ 
		0.0f,											// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 0.0f },					// 色（初期は完全透明）
		BLENDSTATE_ALFA,								// BlendState
		L"asset\\yureihen\\LoseAnim\\LoseGhost.png"				// テクスチャパス
	);

	// インクの開始Yを画面外上部に設定（画像高さの半分分上）
	g_LoseInkInitialY = -(1028.0f * 0.5f);

	// インク画像（初期位置を画面外上部にして、初期は透明にしておく）
	g_LoseAnimeLogoSprite = new Sprite(
		{ SCREEN_WIDTH / 2, g_LoseInkInitialY },
		{ 1028, 1028 }, // 画像本来のサイズ
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f }, // 初期は透明（表示されないようにする）
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\LoseAnim\\LoseAnimeLogo.png"
	);

	// LoseInk を画面中央に表示する（今回はスプライト全体を上から下へ移動させる）
	g_LoseInkBaseX = SCREEN_WIDTH / 2;   // 中央 X
	g_LoseInkBaseY = SCREEN_HEIGHT / 2;  // 目標の中央 Y

	// スプライト全体で降りてくるので、サイズは画像本来のサイズに設定し、初期は画面外上に配置
	g_LoseInkSprite = new Sprite(
		{ g_LoseInkBaseX, g_LoseInkInitialY }, // 画面外上
		{ 1280, 1080 },                       // サイズ（固定）
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\LoseAnim\\LoseInk.png"
	);
	// サウンド再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}
	g_LoseStartTime = timeGetTime();
}

void Animation_Lose_Update(void)
{
	if (g_LoseStartTime == 0) return; // 初期化前ガード

	DWORD currentTime = timeGetTime();
	DWORD elapsedTime = currentTime - g_LoseStartTime;
	float elapsedSeconds = elapsedTime / 1000.0f;

	// ========================
	// ゴーストのフェードイン処理（開始遅延 + 指定時間でフェード）
	// ========================
	if (g_LoseGhostSprite)
	{
		if (elapsedSeconds < GHOST_APPEAR_TIME)
		{
			// ゴースト表示開始前は完全透明
			g_LoseGhostSprite->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		}
		else
		{
			float fadeElapsed = elapsedSeconds - GHOST_APPEAR_TIME;
			if (fadeElapsed < GHOST_FADE_DURATION)
			{
				// 進行度 0..1
				float progress = fadeElapsed / GHOST_FADE_DURATION;

				// イージング（緩やかに出現させる）
				float eased = progress * progress; // ease-in

				g_LoseGhostSprite->SetColor({ 1.0f, 1.0f, 1.0f, eased });
			}
			else
			{
				// フェード完了
				g_LoseGhostSprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			}
		}
	}

	// ========================
	// インク画像の降下アニメーション（LoseAnimeLogoSprite）
	// ========================
	if (g_LoseAnimeLogoSprite)
	{
		if (elapsedSeconds >= INK_DROP_START_TIME)
		{
			float inkElapsedTime = elapsedSeconds - INK_DROP_START_TIME;

			if (inkElapsedTime <= INK_DROP_DURATION)
			{
				// 降下の進行度（0.0 ～ 1.0）
				float progress = inkElapsedTime / INK_DROP_DURATION;

				// イージング関数（加速度的に落ちる効果）
				float easedProgress = progress * progress;

				// 画面上部から画面中央付近までの移動距離
				float startY = g_LoseInkInitialY;			// 開始位置（画面外）
				float endY = SCREEN_HEIGHT * 0.35f;			// 終了位置（画面中央より少し上）
				float currentY = startY + (endY - startY) * easedProgress;

				// インク画像のY座標を更新（SetPosY を使用）
				g_LoseAnimeLogoSprite->SetPosY(currentY);

				// フェードイン効果
				float inkAlpha = progress;
				g_LoseAnimeLogoSprite->SetColor({ 1.0f, 1.0f, 1.0f, inkAlpha });
			}
			else
			{
				// 降下完了後は完全表示・位置固定
				g_LoseAnimeLogoSprite->SetPosY(SCREEN_HEIGHT * 0.35f);
				g_LoseAnimeLogoSprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			}
		}
	}

	// ========================
	// LoseInk：スプライト全体を上から下へ降ろす（垂れ幕ではなく移動）
	// ========================
	if (g_LoseInkSprite)
	{
		// 水平位置は中央に固定
		g_LoseInkSprite->SetPosX(g_LoseInkBaseX);

		// 降下開始タイミングから終了までの進行度で Y を補間
		if (elapsedSeconds >= INK_DROP_START_TIME)
		{
			float dropElapsed = elapsedSeconds - INK_DROP_START_TIME;
			float duration = INK_DROP_DURATION;
			float progress = dropElapsed / duration;
			if (progress > 1.0f) progress = 1.0f;

			// イージング（やわらかい ease-out）
			float t = 1.0f - (1.0f - progress) * (1.0f - progress);

			// 開始位置は画面外上（g_LoseInkInitialY）、終了位置は画面中央付近
			float startY = g_LoseInkInitialY;
			float endY = SCREEN_HEIGHT * 0.5f; // 中央に落ち着かせる（好みで調整可）
			float currentY = startY + (endY - startY) * t;

			g_LoseInkSprite->SetPosY(currentY);
		}
		else
		{
			// 開始前は画面外上に固定
			g_LoseInkSprite->SetPosY(g_LoseInkInitialY);
		}
	}

	// ENTERキーでゲームへ戻る
	if (Keyboard_IsKeyDownTrigger(KK_ENTER))
	{
		StartFade(SCENE_GAME);
	}

	// ENTERキーでゲームへ戻る
	if (Keyboard_IsKeyDownTrigger(KK_E))
	{
		StartFade(SCENE_ANM_LOSE_ED);
	}
}

void Animation_Lose_Draw(void)
{
	if (g_LoseBgSprite) g_LoseBgSprite->Draw();		// 背景
	if (g_LoseAnimeLogoSprite) g_LoseAnimeLogoSprite->Draw();	// アニメロゴ（降下）
	if (g_LoseInkSprite) g_LoseInkSprite->Draw();	// インク（NULL チェック）
	if (g_LoseGhostSprite) g_LoseGhostSprite->Draw();	// ゴースト（前面）

	// ビネットは最前面に描画（オーバーレイ）
	if (g_LoseVinetSprite) g_LoseVinetSprite->Draw();
}

void Animation_Lose_Finalize(void)
{
	delete g_LoseBgSprite; g_LoseBgSprite = nullptr;
	delete g_LoseGhostSprite; g_LoseGhostSprite = nullptr;
	delete g_LoseAnimeLogoSprite; g_LoseAnimeLogoSprite = nullptr;
	delete g_LoseInkSprite; g_LoseInkSprite = nullptr;
	delete g_LoseVinetSprite; g_LoseVinetSprite = nullptr; // ビネット解放（追加）
	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}
}