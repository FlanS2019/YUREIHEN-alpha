#pragma execution_character_set("utf-8")

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "UI_Tutorial.h"
#include "keyboard.h"
#include "mouse.h"
#include "field.h"
#include "define.h"
#include "ghost.h"
#include "camera.h"
#include <windows.h>

// ==========================================
// チュートリアル画面用の変数
// ==========================================
static bool g_IsTutorial = false;
static bool g_TutorialCompleted = false;
static int g_TutorialLastFloor = -1;
static Sprite* g_pTutorialBG = nullptr;
static FontRenderer* g_pTutorialFont = nullptr;
// ==========================================

static void UI_Tutorial_UpdateState(int currentFloor)
{
	if (currentFloor == 2 && !g_TutorialCompleted)
	{
		if (!g_IsTutorial)
		{
			g_IsTutorial = true;
			Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
			ShowCursor(TRUE);
		}
	}
}

void UI_Tutorial_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) return;

	g_IsTutorial = false;
	g_TutorialCompleted = false;
	g_TutorialLastFloor = Field_GetCurrentFloor();

	g_pTutorialBG = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0,
		{ 0,0,0,0.7f },
		BLENDSTATE_ALFA,
		L"asset/texture/fade.png");

	g_pTutorialFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		40.0f,
		0.0f,
		{ 1,1,1,1 },
		"tutorial画面。スペースででゲームを開始する");

	UI_Tutorial_UpdateState(g_TutorialLastFloor);
}

void UI_Tutorial_Finalize(void)
{
	if (g_pTutorialBG) {
		delete g_pTutorialBG;
		g_pTutorialBG = nullptr;
	}

	if (g_pTutorialFont) {
		delete g_pTutorialFont;
		g_pTutorialFont = nullptr;
	}
}

void UI_Tutorial_Update(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor != g_TutorialLastFloor)
	{
		g_TutorialLastFloor = currentFloor;
		UI_Tutorial_UpdateState(currentFloor);
	}

	if (!g_IsTutorial) return;

	if (Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		g_IsTutorial = false;
		g_TutorialCompleted = true;
		Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		ShowCursor(FALSE);
	}
}

void UI_Tutorial_Draw(void)
{
	if (!g_IsTutorial) return;

	if (g_pTutorialBG) g_pTutorialBG->Draw();
	if (g_pTutorialFont) g_pTutorialFont->Draw();
}

bool UI_Tutorial_IsActive(void)
{
	return g_IsTutorial;
}

