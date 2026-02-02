#include "title.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "shader.h"
#include "direct3d.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
#include <cmath>
using namespace DirectX;


// ①Spriteのインスタンス、ポインタ用意
static SplitSprite* g_pTitleSprite = nullptr;
static FontRenderer* g_pTitleFont = nullptr;
static FontRenderer* g_pTitleFont2 = nullptr;
static Sprite* g_pSizeComparisonSprite = nullptr;
static Sprite* g_pinazuma = nullptr;
static SoundData* g_pBGM = nullptr;

// 稲妻関連（opanim.cppから移植）
static Sprite* g_pInazumaSprite = nullptr;

static struct {
	float timer;
	float nextTrigger;
	bool active;
	float flashAlpha;
	unsigned int seed;
} g_Inazuma = { 0.0f, 0.2f, false, 0.0f, 0xC0FFEEu };

static const float INAZUMA_FLASH_DURATION_MIN = 0.08f;
static const float INAZUMA_FLASH_DURATION_MAX = 0.2f;
static const float INAZUMA_INTERVAL_MIN = 0.5f;
static const float INAZUMA_INTERVAL_MAX = 3.5f;
static const float INAZUMA_BASE_ALPHA = 0.5f;

// タイトルロゴ瞬き関連
static struct {
	float blinkTimer;
	float blinkCycle;
} g_LogoBlink = { 0.0f, 1.5f };	// 1.5秒周期で瞬く

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

void Title_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// ②各種初期化
	g_pTitleSprite = new SplitSprite(
		{ SCREEN_WIDTH / 2 - 200.0f, SCREEN_HEIGHT / 2.0f - 100.0f },		//位置
		{ SCREEN_WIDTH * 0.7f, SCREEN_HEIGHT * 0.7f },						//サイズ
		0.0f,																//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },											//RGBA
		BLENDSTATE_NONE,													//BlendState
		L"asset\\texture\\title.png",										//テクスチャパス
		2, 1																//分割数X, Y
	);

	//日本語フォント描画
	g_pTitleFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, (SCREEN_HEIGHT / 5.0f) * 4 },	//位置（画面中央）
		70.0f,													//フォントサイズ（ピクセル）
		0.0f,													//回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },								//RGBA
		"Press Enter - エンターキーを押してスタート！"			//テキスト
	);

	//日本語フォント描画
	g_pTitleFont2 = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },			//位置（画面中央）
		200.0f,													//フォントサイズ（ピクセル）
		0.0f,													//回転
		{ 0.0f, 0.8f, 0.8f, 0.8f },								//RGBA
		"g_pTitleFont2"											//テキスト
	);

	// サイズ比較用Sprite（1.png 32x32 中央配置）
	g_pSizeComparisonSprite = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },			//位置（画面中央）
		{ SCREEN_HEIGHT, SCREEN_HEIGHT },						//フォントサイズ（ピクセル）
		0.0f,													//回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },								//RGBA
		BLENDSTATE_ALFA,										//テキスト
		L"asset\\texture\\guide.png"
	);

	g_pinazuma = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },			//位置（画面中央）
		{ 1280,720 },						//フォントサイズ（ピクセル）
		0.0f,													//回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },								//RGBA
		BLENDSTATE_ALFA,										//テキスト
		L"asset\\yureihen\\inazuma2.png"
	);

	// 稲妻スプライト初期化
	g_pInazumaSprite = new Sprite(
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

	// タイトル画面ではマウスカーソルを表示・絶対モードに設定
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);
}

void Title_Update(void)
{
	// タイトルロゴ瞬きタイマー更新
	g_LogoBlink.blinkTimer += (1.0f / 60.0f);	// 60FPS想定
	if (g_LogoBlink.blinkTimer >= g_LogoBlink.blinkCycle) {
		g_LogoBlink.blinkTimer = 0.0f;
	}

	// 瞬き周期内での時間比率 [0.0, 1.0)
	float blinkProgress = g_LogoBlink.blinkTimer / g_LogoBlink.blinkCycle;
	// 0.0～0.15: 消える、0.15～1.0: 現れる
	float logoAlpha = (blinkProgress < 0.15f) ? (1.0f - blinkProgress / 0.15f) : 1.0f;

	// 稲妻更新
	g_Inazuma.nextTrigger -= (1.0f / 60.0f);	// 60FPS想定
	if (g_Inazuma.nextTrigger <= 0.0f && !g_Inazuma.active)
	{
		g_Inazuma.active = true;
		g_Inazuma.flashAlpha = 0.7f + Rand01() * 0.3f;
		g_Inazuma.timer = 0.0f;
	}

	if (g_Inazuma.active)
	{
		g_Inazuma.timer += (1.0f / 60.0f);
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
	if (g_pInazumaSprite) {
		g_pInazumaSprite->SetColor({ 0.95f, 0.95f, 1.0f, Clamp(inazumaAlpha, 0.0f, 1.5f) });
	}

	// Debug: Wキーで勝利アニメーションへ直接遷移（タイトル上でのデバッグ用）
	//if (Keyboard_IsKeyDown(KK_W))
	//{
	//	SetScene(SCENE_ANM_WIN);// Debug用に勝利アニメーションへ直接飛ぶ
	//	return;
	//}
	//if (Keyboard_IsKeyDown(KK_L))
	//{
	//	SetScene(SCENE_ANM_LOSE);// Debug用にロゴアニメーションへ直接飛ぶ
	//}
	// ③適当な処理　アニメーションなどもここで
	if (Keyboard_IsKeyDownTrigger(KK_ENTER))
	{
		StartFade(SCENE_ANM_OP);
	}
}

void Title_Draw(void)
{
	// タイトルロゴ瞬きアルファ値の計算
	float blinkProgress = g_LogoBlink.blinkTimer / g_LogoBlink.blinkCycle;
	float logoAlpha = (blinkProgress < 0.15f) ? (1.0f - blinkProgress / 0.15f) : 1.0f;

	// タイトルロゴスプライトへのアルファ値適用（親クラスのSetColor互換メソッドが存在する場合）
	// 参考：g_pinazumaと同様にSetColor経由でアルファ値を設定

	g_pTitleSprite->Draw();
	//g_pSizeComparisonSprite->Draw();
	g_pTitleFont->Draw();
	//g_pTitleFont2->Draw();
	g_pinazuma->Draw();
	if (g_pInazumaSprite) g_pInazumaSprite->Draw();	// 稲妻描画
}

void Title_Finalize(void)
{

	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}


	delete g_pTitleSprite;
	g_pTitleSprite = nullptr;

	delete g_pSizeComparisonSprite;
	g_pSizeComparisonSprite = nullptr;

	delete g_pTitleFont;
	g_pTitleFont = nullptr;

	delete g_pTitleFont2;
	g_pTitleFont2 = nullptr;

	delete g_pinazuma;
	g_pinazuma = nullptr;

	delete g_pInazumaSprite;
	g_pInazumaSprite = nullptr;

	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}
}
