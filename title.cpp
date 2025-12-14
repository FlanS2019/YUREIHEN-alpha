#include "title.h"
#include "main.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"

// ①Spriteのインスタンス、ポインタ用意
static SplitSprite* g_pTitleSprite = nullptr;

void Title_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// ②各種初期化
	g_pTitleSprite = new SplitSprite(
		{ SCREEN_WIDTH / 2 - 200.0f, SCREEN_HEIGHT / 2.0f - 100.0f},		//位置
		{ SCREEN_WIDTH * 0.7f, SCREEN_HEIGHT * 0.7f },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_NONE,								//BlendState
		L"asset\\texture\\title.png",					//テクスチャパス
		2, 1											//分割数X, Y
	);
}

void Title_Update(void)
{
	// ③適当な処理　アニメーションなどもここで
	if (Keyboard_IsKeyDown(KK_ENTER))
	{
		StartFade(SCENE_ANM_OP);
	}
}

void Title_Draw(void)
{
	// ④Drawするだけでいい！！！！！！！
	g_pTitleSprite->Draw();
}

void Title_Finalize(void)
{
	if (g_pTitleSprite) {
		delete g_pTitleSprite;
		g_pTitleSprite = nullptr;
	}
}
