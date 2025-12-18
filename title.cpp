#include "title.h"
#include "main.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "TextSprite.h"

// ①Spriteのインスタンス、ポインタ用意
static SplitSprite* g_pTitleSprite = nullptr;
static TextSprite* g_pTitleText = nullptr;
static FontData* g_pTitleFont = nullptr;

void Title_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// ②各種初期化
	g_pTitleSprite = new SplitSprite(
		{ SCREEN_WIDTH / 2 - 200.0f, SCREEN_HEIGHT / 2.0f - 100.0f },		//位置
		{ SCREEN_WIDTH * 0.7f, SCREEN_HEIGHT * 0.7f },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_NONE,								//BlendState
		L"asset\\texture\\title.png",					//テクスチャパス
		2, 1											//分割数X, Y
	);

	// TextSprite初期化
	TextSprite_Initialize();
	
	// フォント読み込み（KaiseiDecol-Medium.ttf、32ピクセル）
	g_pTitleFont = TextSprite_LoadFont("asset/font/KaiseiDecol-Medium.ttf", 64.0f, 512);
	
	if (g_pTitleFont) {
		g_pTitleText = new TextSprite(
			{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 0.85f },	//位置
			{ 1.0f, 1.0f },									//スケール
			0.0f,											//回転（度）
			{ 1.0f, 0.0f, 0.0f, 1.0f },						//RGBA
			BLENDSTATE_ALFA,								//BlendState
			L"はつねみく初音ミクhatsunemiku",							//テキスト
			g_pTitleFont
		);
	}
}

void Title_Update(void)
{
	if (Keyboard_IsKeyDown(KK_P))
	{
		SetScene(SCENE_ANM_WIN);// Debug用にロゴアニメーションへ直接飛ぶ
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
	// ④Drawするだけでいい！！！！！！！
	g_pTitleSprite->Draw();
	
	// テキスト描画
	if (g_pTitleText) {
		g_pTitleText->Draw();
	}
}

void Title_Finalize(void)
{
	if (g_pTitleSprite) {
		delete g_pTitleSprite;
		g_pTitleSprite = nullptr;
	}
	
	if (g_pTitleText) {
		delete g_pTitleText;
		g_pTitleText = nullptr;
	}
	
	TextSprite_Finalize();
}
