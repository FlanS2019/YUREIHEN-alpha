#include "title.h"
#include "main.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
//#include "SpriteFont.h"

// ①Spriteのインスタンス、ポインタ用意
static SplitSprite* g_pTitleSprite = nullptr;
//static SpriteFont2D* g_pStartText = nullptr;

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

	// スタート文字初期化
	//g_pStartText = new SpriteFont2D(
	//	{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f },	//位置
	//	0.0f,											//回転
	//	{ 1.0f, 1.0f },									//スケール
	//	FONT_KAISEIDECOL_M,								//フォントID
	//	L"スタート"										//テキスト
	//);
}

void Title_Update(void)
{
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
	//g_pStartText->Draw();
}

void Title_Finalize(void)
{
	if (g_pTitleSprite) {
		delete g_pTitleSprite;
		g_pTitleSprite = nullptr;
	}

	//if (g_pStartText) {
	//	delete g_pStartText;
	//	g_pStartText = nullptr;
	//}
}
