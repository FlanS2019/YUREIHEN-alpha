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

	Font_InitializeGlobalData();

	// タイトル文字
	g_pVoteTitleFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 60.0f },
		80.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"VOTE"
	);

	// ガイド文字
	g_pVoteGuideFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 80.0f },
		35.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"スペースキーでタイトルに戻る"
	);

	// ありがとうメッセージ
	g_pThankYouFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 140.0f },
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
	if (g_pVoteTitleFont) g_pVoteTitleFont->Draw();
	if (g_pVoteGuideFont) g_pVoteGuideFont->Draw();
	if (g_pThankYouFont)  g_pThankYouFont->Draw();
}

void Vote_Finalize(void)
{
	if (g_pVoteBG)        { delete g_pVoteBG;        g_pVoteBG        = nullptr; }
	if (g_pVoteTitleFont) { delete g_pVoteTitleFont; g_pVoteTitleFont = nullptr; }
	if (g_pVoteGuideFont) { delete g_pVoteGuideFont; g_pVoteGuideFont = nullptr; }
	if (g_pThankYouFont)  { delete g_pThankYouFont;  g_pThankYouFont  = nullptr; }
}
