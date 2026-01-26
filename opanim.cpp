#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "opanim.h"
#include "sound.h"
#include <timeapi.h>
#include <cmath>
#pragma comment(lib, "winmm.lib")

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Opening Animation (オープニングアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// グローバル変数
static Sprite* g_OpBgSprite = nullptr;			// 背景（黒紫）
static Sprite* g_OpYakataSprite = nullptr;		// 屋敷
static Sprite* g_OpBasutaSprite = nullptr;		// バスター
static Sprite* g_OpYureiSprite = nullptr;		// 幽霊
static Sprite* g_OpBikkuriSprite = nullptr;		// びっくり（！マーク）
static Sprite* g_OpInazumaSprite = nullptr;		// 稲妻

static DWORD g_OpStartTime = 0;
static SoundData* g_pBGM = nullptr;

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
static struct {
	XMFLOAT2 pos;
	XMFLOAT2 currentPos;
	float alpha;
	float timer;
	bool reacting;
	bool flipped;
	bool removed;
} g_Yurei = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, 0.0f, 0.0f, false, false, false };

// 稲妻
static struct {
	float timer;
	float nextTrigger;
	bool active;
	float flashAlpha;
	unsigned int seed;
} g_Inazuma = { 0.0f, 0.2f, false, 0.0f, 0xC0FFEEu };

// アニメーション定数
static const float BASUTA_MOVE_START_TIME = 2.0f;
static const float BASUTA_MOVE_SPEED = 110.0f;
static const float BASUTA_FADE_DURATION = 2.0f;
static const float BASUTA_WOBBLE_AMPLITUDE = 8.0f;
static const float BASUTA_WOBBLE_FREQUENCY = 3.5f;

static const float YUREI_APPEAR_TIME = 2.0f;
static const float YUREI_FADE_DURATION = 1.5f;
static const float YUREI_REACT_DISTANCE = 600.0f;
static const float YUREI_FLIP_DURATION = 0.3f;
static const float YUREI_LEFT_MOVE_SPEED = -120.0f;
static const float YUREI_FADE_OUT_TIME = 5.0f;
static const float YUREI_FADE_OUT_DURATION = 0.5f;
static const float YUREI_WOBBLE_AMPLITUDE = 8.0f;
static const float YUREI_WOBBLE_FREQUENCY = 4.5f;

static const float INAZUMA_FLASH_DURATION_MIN = 0.08f;
static const float INAZUMA_FLASH_DURATION_MAX = 0.2f;
static const float INAZUMA_INTERVAL_MIN = 1.0f;
static const float INAZUMA_INTERVAL_MAX = 3.5f;
static const float INAZUMA_BASE_ALPHA = 0.5f;

static const float FADE_OUT_TIME = 8.0f;
static const float YUREI_DETECT_TIME = 3.5f;
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
		L"asset\\yureihen\\Alpha_Tex\\kuromurasaki.png"	// テクスチャパス
	);

	// 屋敷スプライト
	g_YakataPos.x = SCREEN_WIDTH / 2 - 400.0f;
	g_YakataPos.y = SCREEN_HEIGHT / 2 + 120.0f;

	g_OpYakataSprite = new Sprite(
		g_YakataPos,								// 位置
		{ 500.0f, 500.0f },						// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\yakata_jimen1.png"	// テクスチャパス
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
	g_Yurei.pos.x = g_YakataPos.x + 80.0f;
	g_Yurei.pos.y = g_YakataPos.y - 150.0f;
	g_Yurei.currentPos = g_Yurei.pos;
	g_Yurei.alpha = 0.0f;

	g_OpYureiSprite = new Sprite(
		g_Yurei.currentPos,						// 位置
		{ 280.0f, 280.0f },						// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, g_Yurei.alpha },	// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\Alpha_Tex\\yurei1.png"	// テクスチャパス
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

	// 稲妻初期化
	g_Inazuma.nextTrigger = 0.5f + Rand01() * 1.5f;
	g_Inazuma.active = false;

	// BGM再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}

	g_OpStartTime = timeGetTime();
}

void OpAnim_Finalize(void)
{
	delete g_OpBgSprite; g_OpBgSprite = nullptr;
	delete g_OpYakataSprite; g_OpYakataSprite = nullptr;
	delete g_OpBasutaSprite; g_OpBasutaSprite = nullptr;
	delete g_OpYureiSprite; g_OpYureiSprite = nullptr;
	delete g_OpBikkuriSprite; g_OpBikkuriSprite = nullptr;
	delete g_OpInazumaSprite; g_OpInazumaSprite = nullptr;

	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}
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
		}
	}

	// 上下揺れ
	g_Yurei.currentPos = g_Yurei.pos;
	g_Yurei.currentPos.y += sinf(elapsedSeconds * YUREI_WOBBLE_FREQUENCY) * YUREI_WOBBLE_AMPLITUDE;

	// 出現フェード
	if (!g_Yurei.removed && elapsedSeconds >= YUREI_APPEAR_TIME)
	{
		float fadeTime = elapsedSeconds - YUREI_APPEAR_TIME;
		g_Yurei.alpha = Clamp(fadeTime / YUREI_FADE_DURATION, 0.0f, 1.0f);
	}

	// 館に向かって移動
	if (g_Yurei.reacting)
	{
		g_Yurei.timer += g_OpAnimDeltaTime;

		// フェードアウト
		if (g_Yurei.timer > YUREI_FADE_OUT_TIME)
		{
			float outRatio = Clamp((g_Yurei.timer - YUREI_FADE_OUT_TIME) / YUREI_FADE_OUT_DURATION, 0.0f, 1.0f);
			g_Yurei.alpha *= (1.0f - outRatio);

			if (g_Yurei.alpha < 0.01f) {
				g_Yurei.alpha = 0.0f;
				g_Yurei.reacting = false;
				g_Yurei.removed = true;
			}
		}

		// 反応開始時にフリップ
		if (!g_Yurei.flipped && g_Yurei.timer > YUREI_FLIP_DURATION)
		{
			g_Yurei.flipped = true;
		}
	}

	if (g_OpYureiSprite) {
		g_OpYureiSprite->SetPos({ g_Yurei.currentPos.x, g_Yurei.currentPos.y });
		g_OpYureiSprite->SetColor({ 1.0f, 1.0f, 1.0f, g_Yurei.alpha });
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

void OpAnim_Update(void)
{
	if (g_OpStartTime == 0) return; // 初期化前ガード

	DWORD currentTime = timeGetTime();
	DWORD elapsedTime = currentTime - g_OpStartTime;
	float elapsedSeconds = elapsedTime / 1000.0f;

	UpdateInazuma(elapsedSeconds);
	UpdateBasuta(elapsedSeconds);
	UpdateYurei(elapsedSeconds);

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
	if (g_OpYakataSprite) g_OpYakataSprite->Draw();	// 屋敷
	if (g_OpBasutaSprite) g_OpBasutaSprite->Draw();	// バスター
	if (g_OpYureiSprite) g_OpYureiSprite->Draw();	// 幽霊
	if (g_OpBikkuriSprite) g_OpBikkuriSprite->Draw();	// びっくり
	if (g_OpInazumaSprite) g_OpInazumaSprite->Draw();	// 稲妻
}