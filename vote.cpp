#include "vote.h"
#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "define.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
using namespace DirectX;

static Sprite*        g_pVoteBG        = nullptr;
static Sprite*        g_pVoteFrame     = nullptr;	// フレーム（画面全体）
static Sprite*        g_pVoteQR        = nullptr;	// QRコード（右側）
static FontRenderer*  g_pVoteTitleFont  = nullptr;
static FontRenderer*  g_pVoteGuideFont  = nullptr;
static FontRenderer*  g_pThankYouFont   = nullptr;	// 「プレイしてくれてありがとう！」

void Vote_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 画面全体）
	g_pVoteFrame = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\end.png"
	);

	Font_InitializeGlobalData();
	// ガイド文字（左側）
	g_pVoteGuideFont = new FontRenderer(
		{ SCREEN_WIDTH - 300.0f, SCREEN_HEIGHT - 33.0f },
		45.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"スペースキーでタイトルに戻る"
	);

	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);
}

void Vote_Update(void)
{
	if (Keyboard_IsKeyDown(KK_SPACE))
	{
		StartFade(SCENE_TITLE);
	}
}

void Vote_Draw(void)
{
	if (g_pVoteFrame)     g_pVoteFrame->Draw();
	if (g_pVoteGuideFont) g_pVoteGuideFont->Draw();
}

void Vote_Finalize(void)
{
	if (g_pVoteFrame)     { delete g_pVoteFrame;     g_pVoteFrame     = nullptr; }
	if (g_pVoteGuideFont) { delete g_pVoteGuideFont; g_pVoteGuideFont = nullptr; }
}
