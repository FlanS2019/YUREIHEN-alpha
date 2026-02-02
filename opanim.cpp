#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "opanim.h"
#include "mouse.h"
#include "sound.h"
#include "font.h"
#include <timeapi.h>
#include <cmath>
#include <vector>
#include "define.h"
#pragma comment(lib, "winmm.lib")

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Opening Animation (オープニングアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// グローバル変数
static Sprite* g_OpBgSprite = nullptr;			// 背景（黒紫）
/* 雨スプライトを単一から複数化 */
struct RainDrop {
	Sprite* sprite;
	XMFLOAT2 pos;
	float speed;
	float alpha;
	float width;
	float height;
	float swayPhase;
	float swayAmp;
};
static std::vector<RainDrop> g_OpRainDrops;		// 雨ドロップ集合
static const int g_OpRainCount = 70;			// 雨ドロップ数（調整可）

static Sprite* g_OpYakataSprite = nullptr;		// 屋敷
static Sprite* g_OpBasutaSprite = nullptr;		// バスター
static Sprite* g_OpYureiSprite = nullptr;		// 幽霊
static Sprite* g_OpBikkuriSprite = nullptr;		// びっくり（！マーク）
static Sprite* g_OpInazumaSprite = nullptr;		// 稲妻

static DWORD g_OpStartTime = 0;
static SoundData* g_pBGM = nullptr;
static bool g_BGMPlayed = false;				// BGM再生フラグ

//日本語フォント
static FontRenderer* g_pOpAnimFont = nullptr;
static FontRenderer* g_pOpAnimFont2 = nullptr;

// フレームレート設定（40 FPS用）
static const float g_OpAnimFPS = 40.0f;
static const float g_OpAnimDeltaTime = 1.0f / g_OpAnimFPS;	// 0.025秒（40 FPS）

// ========================
// アニメーションパラメータ
// ========================

// 屋敷
static XMFLOAT2 g_YakataPos = { 0.0f, 0.0f };

// バスター（移動）
static struct {
	XMFLOAT2 startPos;
	XMFLOAT2 currentPos;
	XMFLOAT2 targetPos;
	float alpha;
	float moveTimer;
	bool moving;
} g_Basuta = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, 0.0f, 0.0f, false };

// 幽霊（移動・反応）
// 追加フィールド：startPos, startSize, currentSize, exitTargetPos, startAlpha を持たせて
static struct {
	XMFLOAT2 pos;
	XMFLOAT2 currentPos;
	XMFLOAT2 startPos;		// 反応開始時の位置
	XMFLOAT2 startSize;		// 反応開始時のサイズ
	XMFLOAT2 currentSize;	// 現在描画に使うサイズ
	XMFLOAT2 exitTargetPos;	// フェードアウト前に移動する最終到達位置
	float startAlpha;		// 反応開始時のアルファ
	float alpha;
	float timer;
	bool reacting;
	bool flipped;
	bool removed;
} g_Yurei = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, {0.0f,0.0f}, {500.0f,500.0f}, {500.0f,500.0f}, {0.0f,0.0f}, 0.0f, 0.0f, false, false, false };

// 稲妻
static struct {
	float timer;
	float nextTrigger;
	bool active;
	float flashAlpha;
	unsigned int seed;
} g_Inazuma = { 0.0f, 0.2f, false, 0.0f, 0xC0FFEEu };

//バスターズ
static const float BASUTA_MOVE_START_TIME = 2.0f;// バスター移動開始時間
static const float BASUTA_MOVE_SPEED = 110.0f;// バスター移動速度（ピクセル/秒）
static const float BASUTA_FADE_DURATION = 2.0f;//バスターフェードイン時間
static const float BASUTA_WOBBLE_AMPLITUDE = 8.0f;// バスター横揺れ振幅
static const float BASUTA_WOBBLE_FREQUENCY = 3.5f;// バスター横揺れ周波数
//幽霊
static const float YUREI_APPEAR_TIME = 2.0f;// 幽霊出現時間
static const float YUREI_FADE_DURATION = 1.5f;// 幽霊フェードイン時間
static const float YUREI_REACT_DISTANCE = 800.0f;// 幽霊反応距離,画面中央からの距離
static const float YUREI_FLIP_DURATION = 0.3f;// 幽霊反転時間
// 移動遅延・速度調整の追加定数
static const float YUREI_MOVE_DELAY_AFTER_LEFT = 0.5f; // 左向きの後に待つ時間（秒） — ワンテンポ
static const float YUREI_MOVE_SPEED = 0.5f; // 1.0 = 標準、<1 なら遅く（小さいほど遅い）
static const float YUREI_FADE_OUT_TIME = 5.0f;// 幽霊フェードアウト開始時間
static const float YUREI_FADE_OUT_DURATION = 0.5f;// 幽霊フェードアウト時間
static const float YUREI_WOBBLE_AMPLITUDE = 8.0f;// 幽霊横揺れ振幅
static const float YUREI_WOBBLE_FREQUENCY = 4.5f;// 幽霊横揺れ周波数

// 追加：幽霊がフェードアウト前に長く斜め左下へ移動しながら縮むための定数
static const XMFLOAT2 YUREI_EXIT_OFFSET = { -900.0f, 500.0f };	// 反応開始時からの相対移動量（左下）
static const XMFLOAT2 YUREI_TARGET_SIZE = { 180.0f, 180.0f };	// 最終的な小ささ
// 稲妻
static const float INAZUMA_FLASH_DURATION_MIN = 0.08f;
static const float INAZUMA_FLASH_DURATION_MAX = 0.2f;
static const float INAZUMA_INTERVAL_MIN = 1.0f;
static const float INAZUMA_INTERVAL_MAX = 3.5f;
static const float INAZUMA_BASE_ALPHA = 0.5f;
// フェード全体
static const float FADE_OUT_TIME = 8.0f;// フェードアウト開始時間(OPアニメーション全体の流す時間)
static const float YUREI_DETECT_TIME = 2.0f;// 幽霊反応開始時間
// アニメーション定数セクションに追加
static const float BIKKURI_APPEAR_DELAY = 0.3f;	// びっくり表示遅延（秒）

// ========================
// ユーティリティ関数
// ========================

// 線形合同法による乱数生成 [0.0, 1.0)
static float Rand01()
{
	g_Inazuma.seed = g_Inazuma.seed * 1664525u + 1013904223u;
	return (float)(g_Inazuma.seed & 0x00FFFFFFu) / (float)0x01000000u;
}

// イージング関数（ease-out cubic）
static float EaseOutCubic(float t)
{
	if (t <= 0.0f) return 0.0f;
	if (t >= 1.0f) return 1.0f;
	float inv = 1.0f - t;
	return 1.0f - inv * inv * inv;
}

// 値をクランプ
static float Clamp(float value, float min, float max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

// 2点間の距離を計算
static float Distance(const XMFLOAT2& a, const XMFLOAT2& b)
{
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	return sqrtf(dx * dx + dy * dy);
}

// ========================
// 初期化・終了
// ========================

void OpAnim_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 背景スプライト（黒紫）
	g_OpBgSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ SCREEN_WIDTH, SCREEN_HEIGHT },			// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\Alpha_Tex\\yoru.png"	// テクスチャパス
	);

	// 雨ドロップを複数生成（画面全体に散らす）
	g_OpRainDrops.clear();
	g_OpRainDrops.reserve(g_OpRainCount);
	for (int i = 0; i < g_OpRainCount; ++i)
	{
		RainDrop rd{};
		// ランダム幅・高さ（見た目の多様性）
		float w = 6.0f + Rand01() * 18.0f;	// 6..24
		float h = 18.0f + Rand01() * 40.0f;	// 18..58

		rd.pos.x = Rand01() * (float)SCREEN_WIDTH;
		// 初期Yは画面内外に分散させる
		rd.pos.y = (Rand01() * 1.5f - 0.5f) * (float)SCREEN_HEIGHT;
		rd.speed = 200.0f + Rand01() * 400.0f;	// 200..600 px/s
		rd.alpha = 0.3f + Rand01() * 0.6f;		// 0.3..0.9
		rd.width = 100;
		rd.height = 100;
		rd.swayPhase = Rand01() * 6.2831853f;
		rd.swayAmp = 6.0f + Rand01() * 10.0f;	// 横揺れ幅

		rd.sprite = new Sprite(
			{ rd.pos.x, rd.pos.y },				// 位置
			{ rd.width, rd.height },			// サイズ
			0.0f,								// 回転（度）
			{ 1.0f, 1.0f, 1.0f, rd.alpha },		// 色（透明度含む）
			BLENDSTATE_ALFA,					// BlendState
			L"asset\\yureihen\\Op_rain.png"	// テクスチャパス
		);

		g_OpRainDrops.push_back(rd);
	}

	// 屋敷スプライト
	g_YakataPos.x = SCREEN_WIDTH / 2 - 320.0f;
	g_YakataPos.y = SCREEN_HEIGHT / 2 + 1.0f;

	//g_YakataPos.x = SCREEN_WIDTH / 2;
	//g_YakataPos.y = SCREEN_HEIGHT / 2;

	g_OpYakataSprite = new Sprite(
		g_YakataPos,								// 位置
		{ 700,700 },						// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\Alpha_Tex\\yakata_jimen.png"	// テクスチャパス
	);

	// バスタースプライト（初期は透明・画面外右）
	g_Basuta.startPos.x = SCREEN_WIDTH + 200.0f;
	g_Basuta.startPos.y = SCREEN_HEIGHT / 2 + 50.0f;
	g_Basuta.currentPos = g_Basuta.startPos;
	g_Basuta.targetPos.x = g_YakataPos.x - 150.0f;
	g_Basuta.targetPos.y = g_YakataPos.y + 50.0f;
	g_Basuta.alpha = 0.0f;

	g_OpBasutaSprite = new Sprite(
		g_Basuta.currentPos,						// 位置
		{ 500.0f, 500.0f },						// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, g_Basuta.alpha },	// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\Busters_OP.png"	// テクスチャパス
	);

	// 幽霊スプライト（初期は透明）
	g_Yurei.pos.x = g_YakataPos.x + 40.0f;
	g_Yurei.pos.y = g_YakataPos.y - 50.0f;
	g_Yurei.currentPos = g_Yurei.pos;
	g_Yurei.alpha = 0.0f;
	// startSize と currentSize を初期化（Sprite 作成と一致させる）
	g_Yurei.startSize = { SCREEN_WIDTH,SCREEN_HEIGHT };// 幽霊スプライトサイズ
	g_Yurei.currentSize = g_Yurei.startSize;
	g_Yurei.exitTargetPos = { g_Yurei.pos.x + YUREI_EXIT_OFFSET.x, g_Yurei.pos.y + YUREI_EXIT_OFFSET.y };

	g_OpYureiSprite = new Sprite(
		g_Yurei.currentPos,						// 位置
		{ 1924,1361 },						// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, g_Yurei.alpha },	// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\Alpha_Tex\\yurei2.png"	// テクスチャパス
	);

	// びっくりマークスプライト（初期は透明）
	g_OpBikkuriSprite = new Sprite(
		{ g_Yurei.currentPos.x, g_Yurei.currentPos.y - 100.0f },	// 位置（幽霊上方）
		{ 100.0f, 100.0f },						// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 0.0f },				// 色（初期は透明）
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\Alpha_Tex\\bikkuri2.png"	// テクスチャパス
	);

	// 稲妻スプライト
	g_OpInazumaSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置（画面中央）
		{ 1280.0f, 720.0f },						// サイズ
		0.0f,										// 回転（度）
		{ 0.95f, 0.95f, 1.0f, INAZUMA_BASE_ALPHA },	// 色
		BLENDSTATE_ADD,								// BlendState（加算合成）
		L"asset\\yureihen\\inazuma2.png"		// テクスチャパス
	);

	//日本語フォント描画
	g_pOpAnimFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, (SCREEN_HEIGHT / 5.0f) * 4 },	//位置（画面中央）
		70.0f,													//フォントサイズ（ピクセル）
		0.0f,													//回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },								//RGBA
		"Press Enter - エンターキーを押してスタート！"			//テキスト
	);

	//日本語フォント描画
	g_pOpAnimFont2 = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },			//位置（画面中央）
		200.0f,													//フォントサイズ（ピクセル）
		0.0f,													//回転
		{ 0.0f, 0.8f, 0.8f, 0.8f },								//RGBA
		"g_pTitleFont2"											//テキスト
	);


	// 稲妻初期化
	g_Inazuma.nextTrigger = 0.5f + Rand01() * 1.5f;
	g_Inazuma.active = false;

	// BGM再生
	g_pBGM = LoadMP3("asset/sound/se/ghost1.mp3");
	g_BGMPlayed = false;

	// BGM再生
	g_pBGM = LoadMP3("asset/sound/se/lightning_rain.mp3");
	g_BGMPlayed = false;

	// OP画面ではマウスカーソルを表示・絶対モードに設定
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);

	g_OpStartTime = timeGetTime();
}

void OpAnim_Finalize(void)
{
	delete g_OpBgSprite; g_OpBgSprite = nullptr;

	// 雨ドロップ解放
	for (auto& rd : g_OpRainDrops) {
		if (rd.sprite) {
			delete rd.sprite;
			rd.sprite = nullptr;
		}
	}
	g_OpRainDrops.clear();

	delete g_OpYakataSprite; g_OpYakataSprite = nullptr;
	delete g_OpBasutaSprite; g_OpBasutaSprite = nullptr;
	delete g_OpYureiSprite; g_OpYureiSprite = nullptr;
	delete g_OpBikkuriSprite; g_OpBikkuriSprite = nullptr;
	delete g_OpInazumaSprite; g_OpInazumaSprite = nullptr;
	delete g_pOpAnimFont; g_pOpAnimFont = nullptr;
	delete g_pOpAnimFont2; g_pOpAnimFont2 = nullptr;


	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}

	g_BGMPlayed = false;
}

// ========================
// 更新処理
// ========================

static void UpdateInazuma(float elapsedSeconds)
{
	g_Inazuma.nextTrigger -= g_OpAnimDeltaTime;
	if (g_Inazuma.nextTrigger <= 0.0f && !g_Inazuma.active)
	{
		g_Inazuma.active = true;
		g_Inazuma.flashAlpha = 0.7f + Rand01() * 0.3f;
		g_Inazuma.timer = 0.0f;
	}

	if (g_Inazuma.active)
	{
		g_Inazuma.timer += g_OpAnimDeltaTime;
		float duration = INAZUMA_FLASH_DURATION_MIN + Rand01() * (INAZUMA_FLASH_DURATION_MAX - INAZUMA_FLASH_DURATION_MIN);
		float fade = 1.0f - EaseOutCubic(g_Inazuma.timer / duration);
		g_Inazuma.flashAlpha *= fade;

		if (g_Inazuma.timer >= duration)
		{
			g_Inazuma.active = false;
			g_Inazuma.flashAlpha = 0.0f;
			g_Inazuma.nextTrigger = INAZUMA_INTERVAL_MIN + Rand01() * (INAZUMA_INTERVAL_MAX - INAZUMA_INTERVAL_MIN);
		}
	}

	// 稲妻アルファ更新
	float inazumaAlpha = INAZUMA_BASE_ALPHA + g_Inazuma.flashAlpha * 0.9f;
	if (g_OpInazumaSprite) {
		g_OpInazumaSprite->SetColor({ 0.95f, 0.95f, 1.0f, Clamp(inazumaAlpha, 0.0f, 1.5f) });
	}
}

static void UpdateBasuta(float elapsedSeconds)
{
	if (elapsedSeconds >= BASUTA_MOVE_START_TIME && !g_Basuta.moving)
	{
		g_Basuta.moving = true;
		g_Basuta.moveTimer = 0.0f;
	}

	if (g_Basuta.moving)
	{
		g_Basuta.moveTimer += g_OpAnimDeltaTime;
		float dx = g_Basuta.targetPos.x - g_Basuta.startPos.x;
		float dy = g_Basuta.targetPos.y - g_Basuta.startPos.y;
		float totalDist = sqrtf(dx * dx + dy * dy);
		if (totalDist <= 0.001f) totalDist = 0.001f;

		float moveAmount = BASUTA_MOVE_SPEED * g_Basuta.moveTimer;
		float ratio = Clamp(moveAmount / totalDist, 0.0f, 1.0f);

		g_Basuta.currentPos.x = g_Basuta.startPos.x + dx * ratio;
		g_Basuta.currentPos.y = g_Basuta.startPos.y + dy * ratio;

		// 上下揺れ
		g_Basuta.currentPos.y += sinf(elapsedSeconds * BASUTA_WOBBLE_FREQUENCY) * BASUTA_WOBBLE_AMPLITUDE;

		// フェードイン
		float fadeTime = elapsedSeconds - BASUTA_MOVE_START_TIME;
		g_Basuta.alpha = Clamp(fadeTime / BASUTA_FADE_DURATION, 0.0f, 1.0f);

		if (g_OpBasutaSprite) {
			g_OpBasutaSprite->SetPos({ g_Basuta.currentPos.x, g_Basuta.currentPos.y });
			g_OpBasutaSprite->SetColor({ 1.0f, 1.0f, 1.0f, g_Basuta.alpha });
		}
	}
}

static void UpdateYurei(float elapsedSeconds)
{
	// バスターを見つけたら反応
	if (!g_Yurei.reacting && elapsedSeconds > YUREI_DETECT_TIME)
	{
		float dist = Distance(g_Basuta.currentPos, g_Yurei.pos);

		if (dist < YUREI_REACT_DISTANCE)
		{
			g_Yurei.reacting = true;
			g_Yurei.timer = 0.0f;
			// 反応開始時に移動・サイズ・アルファの基準を記録
			g_Yurei.startPos = g_Yurei.currentPos;
			g_Yurei.startSize = g_Yurei.currentSize;
			g_Yurei.startAlpha = g_Yurei.alpha;
			g_Yurei.exitTargetPos = { g_Yurei.startPos.x + YUREI_EXIT_OFFSET.x, g_Yurei.startPos.y + YUREI_EXIT_OFFSET.y };
		}
	}

	// デフォルトの上下揺れ（基準位置に対して）
	g_Yurei.currentPos = g_Yurei.pos;
	g_Yurei.currentPos.y += sinf(elapsedSeconds * YUREI_WOBBLE_FREQUENCY) * YUREI_WOBBLE_AMPLITUDE;

	// 出現フェード
	if (!g_Yurei.removed && elapsedSeconds >= YUREI_APPEAR_TIME)
	{
		float fadeTime = elapsedSeconds - YUREI_APPEAR_TIME;
		g_Yurei.alpha = Clamp(fadeTime / YUREI_FADE_DURATION, 0.0f, 1.0f);
	}

	// 館に向かって移動（反応後の処理）
	if (g_Yurei.reacting)
	{
		g_Yurei.timer += g_OpAnimDeltaTime;

		// 反応開始時にフリップ
		if (!g_Yurei.flipped && g_Yurei.timer > YUREI_FLIP_DURATION)
		{
			g_Yurei.flipped = true;
		}

		// ワンテンポ待ってから遅く移動するロジック：
		// 基準となる時間範囲を計算（遅延を挟む）
		float baseStart = YUREI_FLIP_DURATION;
		float baseAvailable = YUREI_FADE_OUT_TIME - baseStart;
		if (baseAvailable < 0.01f) baseAvailable = 0.01f;

		float moveStart = baseStart + YUREI_MOVE_DELAY_AFTER_LEFT;
		// 移動所要時間を遅くする（YUREI_MOVE_SPEED < 1.0 なら長くなる）
		float moveDuration = (baseAvailable - YUREI_MOVE_DELAY_AFTER_LEFT) / YUREI_MOVE_SPEED;
		if (moveDuration < 0.01f) moveDuration = 0.01f;
		float moveEnd = moveStart + moveDuration;

		// 移動・縮小・フェード（移動中はイージングで進行）
		if (g_Yurei.timer >= moveStart && g_Yurei.timer <= moveEnd)
		{
			float t = Clamp((g_Yurei.timer - moveStart) / moveDuration, 0.0f, 1.0f);
			float e = EaseOutCubic(t);

			// 位置補間（startPos → exitTargetPos）
			g_Yurei.currentPos.x = g_Yurei.startPos.x + (g_Yurei.exitTargetPos.x - g_Yurei.startPos.x) * e;
			g_Yurei.currentPos.y = g_Yurei.startPos.y + (g_Yurei.exitTargetPos.y - g_Yurei.startPos.y) * e;

			// サイズ補間（startSize → YUREI_TARGET_SIZE）
			g_Yurei.currentSize.x = g_Yurei.startSize.x + (YUREI_TARGET_SIZE.x - g_Yurei.startSize.x) * e;
			g_Yurei.currentSize.y = g_Yurei.startSize.y + (YUREI_TARGET_SIZE.y - g_Yurei.startSize.y) * e;

			// アルファを同時に減少（反応開始時の alpha -> 0）
			g_Yurei.alpha = Clamp(g_Yurei.startAlpha * (1.0f - e), 0.0f, 1.0f);
		}
		else if (g_Yurei.timer > moveEnd)
		{
			// moveEnd を超えたら最終値を確実にセット（消去状態へ）
			g_Yurei.currentPos = g_Yurei.exitTargetPos;
			g_Yurei.currentSize = YUREI_TARGET_SIZE;
			g_Yurei.alpha = 0.0f;
			g_Yurei.reacting = false;
			g_Yurei.removed = true;
		}

		// 保険：さらに YUREI_FADE_OUT_TIME を超えた場合のフェード処理
		if (g_Yurei.timer > YUREI_FADE_OUT_TIME && !g_Yurei.removed)
		{
			float outRatio = Clamp((g_Yurei.timer - YUREI_FADE_OUT_TIME) / YUREI_FADE_OUT_DURATION, 0.0f, 1.0f);
			g_Yurei.alpha *= (1.0f - outRatio);

			if (g_Yurei.alpha < 0.01f) {
				g_Yurei.alpha = 0.0f;
				g_Yurei.reacting = false;
				g_Yurei.removed = true;
			}
		}
	}

	// スプライトへ反映
	if (g_OpYureiSprite) {
		g_OpYureiSprite->SetPos({ g_Yurei.currentPos.x, g_Yurei.currentPos.y });
		g_OpYureiSprite->SetColor({ 1.0f, 1.0f, 1.0f, g_Yurei.alpha });
		// サイズ（縮小）を反映
		g_OpYureiSprite->SetSize(g_Yurei.currentSize);
		// フリップ状態を反映
		if (g_Yurei.flipped) {
			g_OpYureiSprite->SetFlipType(FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL);
		}
		else {
			g_OpYureiSprite->SetFlipType(FLIPTYPE2D::FLIPTYPE2D_NONE);
		}
	}
	// びっくりマーク（幽霊が反応中のみ表示）
	if (g_OpBikkuriSprite) {
		XMFLOAT2 bikkuriPos = { g_Yurei.currentPos.x, g_Yurei.currentPos.y - 100.0f };
		g_OpBikkuriSprite->SetPos(bikkuriPos);
		// 反応開始から一定時間後に表示
		float bikkuriAlpha = (g_Yurei.reacting && g_Yurei.timer > BIKKURI_APPEAR_DELAY) ? g_Yurei.alpha : 0.0f;
		g_OpBikkuriSprite->SetColor({ 1.0f, 1.0f, 1.0f, bikkuriAlpha });
	}
}

// 雨更新処理
static void UpdateRain(float elapsedSeconds)
{
	// elapsedSeconds はシーン開始からの経過秒なので、常に現在時間で横揺れを計算する
	float time = elapsedSeconds;
	for (auto& rd : g_OpRainDrops)
	{
		// 縦移動
		rd.pos.y += rd.speed * g_OpAnimDeltaTime;

		// 軽い横揺れ（サイン波）を追加
		float sway = sinf(time * 2.0f + rd.swayPhase) * rd.swayAmp;
		rd.sprite->SetPos({ rd.pos.x + sway, rd.pos.y });

		// 透明度は固定（生成時にセット）だが、必要ならここで変化可能
		rd.sprite->SetColor({ 1.0f, 1.0f, 1.0f, rd.alpha });

		// 画面下に到達したら再生成（上から再登場）
		if (rd.pos.y - rd.height > (float)SCREEN_HEIGHT)
		{
			rd.pos.y = -(Rand01() * 120.0f + 20.0f); // 少しランダム化して上に出す
			rd.pos.x = Rand01() * (float)SCREEN_WIDTH;
			rd.speed = 200.0f + Rand01() * 400.0f;
			rd.alpha = 0.3f + Rand01() * 0.6f;
			// サイズのランダムリフレッシュ（見た目の変化）
			rd.width = 6.0f + Rand01() * 18.0f;
			rd.height = 18.0f + Rand01() * 40.0f;
			// サイズ変更が必要なら Sprite を再生成するか、サイズ変更 API を呼ぶ
			//（ここでは簡潔さのため新規スプライト生成は行わず、表示位置のみ更新）
			rd.swayPhase = Rand01() * 6.2831853f;
			rd.swayAmp = 6.0f + Rand01() * 10.0f;
			if (rd.sprite) {
				rd.sprite->SetPos({ rd.pos.x, rd.pos.y });
				rd.sprite->SetColor({ 1.0f, 1.0f, 1.0f, rd.alpha });
			}
		}
	}
}

void OpAnim_Update(void)
{
	if (g_OpStartTime == 0) return; // 初期化前ガード

	DWORD currentTime = timeGetTime();
	DWORD elapsedTime = currentTime - g_OpStartTime;
	float elapsedSeconds = elapsedTime / 1000.0f;

	// BGMを一度だけ再生
	if (!g_BGMPlayed && g_pBGM) {
		PlaySound(g_pBGM, true);
		g_BGMPlayed = true;
	}

	UpdateInazuma(elapsedSeconds);
	UpdateBasuta(elapsedSeconds);
	UpdateYurei(elapsedSeconds);

	// 雨更新を追加
	UpdateRain(elapsedSeconds);

	// フェードアウトと次シーン遷移
	if (elapsedSeconds > FADE_OUT_TIME && GetFadeState() == FADE_NONE)
	{
		StartFade(SCENE_GAME);
	}

	// Eキーでタイトルへ戻る
	if (Keyboard_IsKeyDownTrigger(KK_E))
	{
		SetScene(SCENE_TITLE);
	}
}

// ========================
// 描画処理
// ========================

void OpAnimDraw(void)
{
	if (g_OpBgSprite) g_OpBgSprite->Draw();			// 背景

	// 雨ドロップを描画（前景オーバーレイ）
	for (auto& rd : g_OpRainDrops) {
		if (rd.sprite) rd.sprite->Draw();
	}

	if (g_OpYakataSprite) g_OpYakataSprite->Draw();	// 屋敷
	if (g_OpBasutaSprite) g_OpBasutaSprite->Draw();	// バスター
	if (g_OpYureiSprite) g_OpYureiSprite->Draw();	// 幽霊
	if (g_OpBikkuriSprite) g_OpBikkuriSprite->Draw();	// びっくり
	if (g_OpInazumaSprite) g_OpInazumaSprite->Draw();	// 稲妻
	g_pOpAnimFont->Draw();
	//g_pOpAnimFont2->Draw();
}