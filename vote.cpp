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
	// 背景（黒）
	g_pVoteBG = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0.0f,
		{ 0.0f, 0.0f, 0.0f, 1.0f },
		BLENDSTATE_NONE,
		L"asset\\texture\\white.png"
	);

	// フレーム（画面全体）
	g_pVoteFrame = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALPHA,
		L"asset\\texture\\frame.png"
	);

	// QRコード（右側）
	g_pVoteQR = new Sprite(
		{ SCREEN_WIDTH * 3.0f / 4.0f, SCREEN_HEIGHT / 2.0f },
		{ 250.0f, 250.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALPHA,
		L"asset\\texture\\qr.png"
	);

	Font_InitializeGlobalData();

	// タイトル文字（左側）
	g_pVoteTitleFont = new FontRenderer(
		{ SCREEN_WIDTH / 4.0f, SCREEN_HEIGHT / 2.0f - 60.0f },
		80.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"VOTE"
	);

	// ガイド文字（左側）
	g_pVoteGuideFont = new FontRenderer(
		{ SCREEN_WIDTH / 4.0f, SCREEN_HEIGHT / 2.0f + 80.0f },
		35.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"スペースキーでタイトルに戻る"
	);

	// ありがとうメッセージ（左側）
	g_pThankYouFont = new FontRenderer(
		{ SCREEN_WIDTH / 4.0f, SCREEN_HEIGHT / 2.0f + 140.0f },
		40.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"プレイしてくれてありがとう！"
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
	if (g_pVoteBG)        g_pVoteBG->Draw();
	if (g_pVoteFrame)     g_pVoteFrame->Draw();
	if (g_pVoteQR)        g_pVoteQR->Draw();
	if (g_pVoteTitleFont) g_pVoteTitleFont->Draw();
	if (g_pVoteGuideFont) g_pVoteGuideFont->Draw();
	if (g_pThankYouFont)  g_pThankYouFont->Draw();
}

void Vote_Finalize(void)
{
	if (g_pVoteBG)        { delete g_pVoteBG;        g_pVoteBG        = nullptr; }
	if (g_pVoteFrame)     { delete g_pVoteFrame;     g_pVoteFrame     = nullptr; }
	if (g_pVoteQR)        { delete g_pVoteQR;        g_pVoteQR        = nullptr; }
	if (g_pVoteTitleFont) { delete g_pVoteTitleFont; g_pVoteTitleFont = nullptr; }
	if (g_pVoteGuideFont) { delete g_pVoteGuideFont; g_pVoteGuideFont = nullptr; }
	if (g_pThankYouFont)  { delete g_pThankYouFont;  g_pThankYouFont  = nullptr; }
}
