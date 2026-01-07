#include "title.h"
#include "main.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "shader.h"
#include "direct3d.h"
#include "font.h"

// ①Spriteのインスタンス、ポインタ用意
static SplitSprite* g_pTitleSprite = nullptr;
static Light* g_pTitleLight = nullptr;
static FontRenderer* g_pTitleFont = nullptr;
static FontRenderer* g_pTitleFont2 = nullptr;
static Sprite* g_pSizeComparisonSprite = nullptr;

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

	// タイトル画面用ライト（無効 + 白環境光）
	g_pTitleLight = new Light(
		FALSE,
		XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f),
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
	);
}

void Title_Update(void)
{
	// Debug: Wキーで勝利アニメーションへ直接遷移（タイトル上でのデバッグ用）
	if (Keyboard_IsKeyDown(KK_W))
	{
		SetScene(SCENE_ANM_WIN);// Debug用に勝利アニメーションへ直接飛ぶ
		return;
	}
	if (Keyboard_IsKeyDown(KK_L))
	{
		SetScene(SCENE_ANM_LOSE);// Debug用にロゴアニメーションへ直接飛ぶ
	}
	// ③適当な処理　アニメーションなどもここで
	if (Keyboard_IsKeyDownTrigger(KK_ENTER))
	{
		StartFade(SCENE_ANM_OP);
	}
}

void Title_Draw(void)
{
	// ライト設定（Title画面用）
	Shader_SetLight(g_pTitleLight);

	g_pTitleSprite->Draw();
	//g_pSizeComparisonSprite->Draw();
	g_pTitleFont->Draw();
	//g_pTitleFont2->Draw();
}

void Title_Finalize(void)
{
	delete g_pTitleSprite;
	g_pTitleSprite = nullptr;

	delete g_pSizeComparisonSprite;
	g_pSizeComparisonSprite = nullptr;

	delete g_pTitleFont;
	g_pTitleFont = nullptr;

	delete g_pTitleFont2;
	g_pTitleFont2 = nullptr;

	delete g_pTitleLight;
	g_pTitleLight = nullptr;
}
