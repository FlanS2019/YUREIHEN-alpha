#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "sound.h"
#include "WinAnim.h"
#include <timeapi.h>
#include "define.h"
#pragma comment(lib, "winmm.lib")

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
//sound
static SoundData* g_pBGM = nullptr;

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Win Animation (勝ちアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sprite* g_WinSprite = nullptr;

void Animation_Win_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_WinSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1028,720 },			// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\WinAnim\\WinAnime.png"				// テクスチャパス
	);

	// サウンド再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}
}

void Animation_Win_Update(void)
{
	// ENTERキーでタイトル画面へ遷移
	if (Keyboard_IsKeyDownTrigger(KK_ENTER))
	{
		StartFade(SCENE_RESULT);
	}
}

void Animation_Win_Draw(void)
{
	g_WinSprite->Draw();
}

void Animation_Win_Finalize(void)
{
	delete g_WinSprite;

	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}

}
