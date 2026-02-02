#include "main.h"
#include "animation.h"
#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "OpAnim.h"
#include "component.h"
#include "shader.h"
#include "direct3d.h"
#include "define.h"
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
using namespace DirectX;

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Logo Animation (ロゴアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
enum LogoState
{
	LS_SCENEIN = 0,
	LS_INFADEEND,
	LS_DARKCHANGESTART,
	LS_DARKCHANGETEX,
	LS_DARKCHANGEEND,
};

SplitSprite* g_LogoSprite = nullptr;
Sprite* g_BG = nullptr;
LogoState State = LS_SCENEIN;
FADESTAT StateChanged = FADE_IN;
static DWORD g_LogoStartTime = 0;	// ロゴ開始時刻
static const float LOGO_AUTO_FADE_TIME = 1.0f;	// 自動フェード開始時間（1秒）

void Animation_Logo_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	State = LS_SCENEIN;
	StateChanged = FADE_IN;

	g_LogoSprite = new SplitSprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },// 位置
		{ 1000.0f, 1000.0f },					// サイズ
		0.0f,									// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,						// BlendState
		L"asset\\texture\\violisunlogo.png",	// テクスチャパス
		2, 1									// 分割数X, Y
	);
	g_BG = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },// 位置
		{ SCREEN_WIDTH, SCREEN_HEIGHT },		// サイズ
		0.0f,									// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,						// BlendState
		L"asset\\texture\\fade.png"			// テクスチャパス
	);

}

void Animation_Logo_Update(void)
{

	if (Keyboard_IsKeyDown(KK_E))
	{
		SetScene(SCENE_GAME);//Debug用にゲームシーンへ直接飛ぶ
	}

	// フェード状態が変化したら状態を進める
	if (StateChanged != GetFadeState())
	{
		State = (LogoState)((int)State + 1);
		StateChanged = GetFadeState();

		if (State == LS_INFADEEND || State == LS_DARKCHANGEEND)
		{
			g_LogoStartTime = timeGetTime();
		}
	}

	// 正確な経過時間を計算（ミリ秒）
	DWORD currentTime = timeGetTime();
	DWORD elapsedTime = currentTime - g_LogoStartTime;
	float elapsedSeconds = elapsedTime / 1000.0f;

	//スペースを押されるか時間でロゴ変化・シーン遷移
	if (Keyboard_IsKeyDownTrigger(KK_ENTER) || elapsedSeconds >= LOGO_AUTO_FADE_TIME)
	{
		// 1枚目表示中なら、次のロゴへ向けてフェードアウト
		if (State == LS_INFADEEND)
		{
			StartFade();
		}
		// 2枚目表示が完了していたら、タイトル画面へ遷移
		else if (State >= LS_DARKCHANGEEND && GetFadeState() == FADE_NONE)
		{
			StartFade(SCENE_TITLE);
		}
	}

	// フェードアウトが完了してテクスチャ切り替えタイミングなら
	if (State == LS_DARKCHANGETEX)
	{
		g_LogoSprite->SetTextureNumber(1);
		g_BG->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
		// 2枚目のロゴを表示するためにフェードインを開始
		Fade_StartIn();
	}
}

void Animation_Logo_Draw(void)
{
	g_BG->Draw();
	g_LogoSprite->Draw();
}

void Animation_Logo_Finalize(void)
{
	delete g_LogoSprite;
	delete g_BG;
}

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Op Animation (Openingアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void Animation_Op_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	SetFPS(40);
	OpAnim_Initialize(pDevice, pContext);
}

void Animation_Op_Update(void)
{
	OpAnim_Update();

	// ENTERキーでタイトル画面へ遷移
	if (Keyboard_IsKeyDownTrigger(KK_ENTER))
	{
		StartFade(SCENE_GAME);
	}

}

void Animation_Op_Draw(void)
{
	OpAnimDraw();
}

void Animation_Op_Finalize(void)
{
	OpAnim_Finalize();
	SetFPS(FPS);
}