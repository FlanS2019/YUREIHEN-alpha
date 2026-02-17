#include "result.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "font.h"
#include <sstream>
#include <iomanip>
using namespace DirectX;


// ①Spriteのインスタンス、ポインタ用意
static Sprite* g_pResultSprite = nullptr;
static FontRenderer* g_pResultTimeFont = nullptr;
static float g_pResultTime = 0.0f;

void Result_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// ②各種初期化
	g_pResultSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2.0f },		//位置
		{ SCREEN_WIDTH , SCREEN_HEIGHT },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_NONE,								//BlendState
		L"asset\\yureihen\\Alpha_Tex\\siro.png"					//テクスチャパス
	);

	// グローバルフォントデータを初期化
	Font_InitializeGlobalData();

	// タイマー結果表示用フォント
	g_pResultTimeFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 100.0f },	// 位置
		60.0f,		// フォントサイズ
		0.0f,		// 回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		""	// テキストは空に（g_pResultTimeで設定済みの値を使用）
	);

	// 既に Result_SetTimerValue() で時間がセット済みの場合、それを反映
	if (g_pResultTime > 0.0f)
	{
		std::ostringstream oss;
		oss << "Time: " << std::fixed << std::setprecision(1) << g_pResultTime << " sec";
		g_pResultTimeFont->SetText(oss.str());
	}
}

void Result_Update(void)
{
	// ③適当な処理　アニメーションなどもここで
	if (Keyboard_IsKeyDown(KK_ENTER))
	{
		StartFade(SCENE_TITLE);
	}
}

void Result_Draw(void)
{
	// ④Drawするだけでいい！！！！！！！
	g_pResultSprite->Draw();

	// タイマー結果を描画
	if (g_pResultTimeFont)
	{
		g_pResultTimeFont->Draw();
	}
}

void Result_Finalize(void)
{
	if (g_pResultSprite) {
		delete g_pResultSprite;
		g_pResultSprite = nullptr;
	}

	if (g_pResultTimeFont) {
		delete g_pResultTimeFont;
		g_pResultTimeFont = nullptr;
	}

	Font_FinalizeGlobalData();
}

// タイマー結果をセット
void Result_SetTimerValue(float time)
{
	g_pResultTime = time;

	if (g_pResultTimeFont)
	{
		// 秒数をフォーマット（小数点第1位まで）
		std::ostringstream oss;
		oss << "Time: " << std::fixed << std::setprecision(1) << time << " sec";
		g_pResultTimeFont->SetText(oss.str());
	}
}