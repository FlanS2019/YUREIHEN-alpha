#include "result.h"
#include "main.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"

// ①Spriteのインスタンス、ポインタ用意
static Sprite* g_pResultSprite = nullptr;

void Result_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// ②各種初期化
	g_pResultSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2.0f },		//位置
		{ SCREEN_WIDTH , SCREEN_HEIGHT },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_NONE,								//BlendState
		L"asset\\texture\\result.png"					//テクスチャパス
	);
}

void Result_Update(void)
{
	// ③適当な処理　アニメーションなどもここで
	if (Keyboard_IsKeyDown(KK_ENTER))
	{
		StartFade(SCENE_ANM_LOGO);
	}
}

void Result_Draw(void)
{
	// ④Drawするだけでいい！！！！！！！
	g_pResultSprite->Draw();
}

void Result_Finalize(void)
{
	if (g_pResultSprite) {
		delete g_pResultSprite;
		g_pResultSprite = nullptr;
	}
}