#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "LoseAnimED.h"
#include "sound.h"
#include <timeapi.h>
#include <cmath>	// 揺れ用の sinf（揺れを消す場合は残しても問題ありません）
#pragma comment(lib, "winmm.lib")

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;

Sprite* g_LoseEDBasuta = nullptr;		// 背景（Losehaikei）
Sprite* g_LoseEDGhost = nullptr;		// 背景（Losehaikei）
Sprite* g_LoseEDrope = nullptr;		// 背景（Losehaikei）

static SoundData* g_pBGM = nullptr;

void Animation_LoseED_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	//バスターズスプライト
	g_LoseEDBasuta = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1280, 1080 },								// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\LoseAnim\\LoseEDBasuta.png"		// テクスチャパス
	);

	//バスターズスプライト
	g_LoseEDGhost = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1280, 1080 },								// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\LoseAnim\\LoseEDGhost.png"		// テクスチャパス
	);

	//バスターズスプライト
	g_LoseEDrope = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1280, 1080 },								// サイズw
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\LoseAnim\\LoseEDrope.png"		// テクスチャパス
	);

	// サウンド再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}

}
void Animation_LoseED_Update(void)
{
}
void Animation_LoseED_Draw(void)
{
	if (g_LoseEDBasuta) g_LoseEDBasuta->Draw();		//バスターズ
	if (g_LoseEDGhost) g_LoseEDGhost->Draw();		//幽霊
	if (g_LoseEDrope) g_LoseEDrope->Draw();		//幽霊
}
void Animation_LoseED_Finalize(void)
{
	delete g_LoseEDBasuta; g_LoseEDBasuta = nullptr;//バスターズ
	delete g_LoseEDGhost; g_LoseEDGhost = nullptr;//幽霊
	delete g_LoseEDrope; g_LoseEDrope = nullptr;//幽霊

	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}
}