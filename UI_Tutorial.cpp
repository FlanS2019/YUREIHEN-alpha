#pragma execution_character_set("utf-8")

#include <d3d11.h>
#include <DirectXMath.h>
#include <cmath>
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
static bool g_IsPreTutorial = false;
static bool g_TutorialCompleted = false;
static int g_TutorialLastFloor = -1;
static HoleSprite* g_pTutorialBG = nullptr;
static FontRenderer* g_pTutorialFont = nullptr;
static float g_TutorialHoleTime = 0.0f;

// フロア変更（シーン遷移）後、指定フレームだけ開始を遅らせる
static int g_TutorialDelayFrames = 0;
// ==========================================

static void UI_Tutorial_UpdateState(int currentFloor)
{
	// フロア切り替え直後は開始を遅延（カメラ初期化などを1フレーム通す）
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

	// 遅延中（背景だけ先に出す）
	g_IsPreTutorial = (currentFloor == 2 && !g_TutorialCompleted);

	//チュートリアル一旦無効化
	if (currentFloor == 2 && !g_TutorialCompleted)
	{
		if (!g_IsTutorial)
		{
			// ここでは開始条件だけ作り、実際の開始は遅延カウント後に行う
		}
	}
	else
	{
		g_IsPreTutorial = false;
	}
}

void UI_Tutorial_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) return;

	g_IsTutorial = false;
	g_IsPreTutorial = false;
	g_TutorialCompleted = false;
	g_TutorialLastFloor = Field_GetCurrentFloor();
	g_TutorialDelayFrames = 0;

	g_pTutorialBG = new HoleSprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0,
		{ 0,0,0,0.7f },
		BLENDSTATE_ALFA,
		L"asset/texture/fade.png");

	// ここで一括管理
	g_pTutorialBG->SetHoleRadiusPx(100.0f);
	g_pTutorialBG->SetHoleSoftnessPx(8.0f);
	g_pTutorialBG->SetHoleCenterPx({ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f });

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

	// チュートリアル開始待ち（指定フレームだけ通常更新を通す）
	if (!g_IsTutorial && !g_TutorialCompleted)
	{
		if (g_TutorialDelayFrames > 0)
		{
			--g_TutorialDelayFrames;
		}
		else
		{
			if (currentFloor == 2)
			{
				g_IsPreTutorial = false;
				g_IsTutorial = true;
				Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
				ShowCursor(TRUE);
			}
		}
	}

	if (!g_IsTutorial) return;

	// チュートリアル中はカーソル表示状態を確実に維持する
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);

	// デバッグ：穴を左右に往復移動
	g_TutorialHoleTime += 1.0f / 60.0f;
	const float amp = 200.0f;
	const float cx = (SCREEN_WIDTH * 0.5f) + sinf(g_TutorialHoleTime) * amp;
	const float cy = (SCREEN_HEIGHT * 0.5f);
	if (g_pTutorialBG) g_pTutorialBG->SetHoleCenterPx({ cx, cy });

	if (Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		g_IsTutorial = false;
		g_IsPreTutorial = false;
		g_TutorialCompleted = true;
		Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		Mouse_SetVisible(false);
	}
}

void UI_Tutorial_Draw(void)
{
	if (g_IsPreTutorial || g_IsTutorial)
	{
		g_pTutorialBG->Draw();
		g_pTutorialFont->Draw();
	}
}

bool UI_Tutorial_IsActive(void)
{
	return g_IsTutorial;
}

void UI_Tutorial_SetActive(bool active)
{
	g_IsTutorial = active;
	g_IsPreTutorial = false;
	if (g_IsTutorial)
	{
		Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
		Mouse_SetVisible(true);
	}
	else
	{
		Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		Mouse_SetVisible(false);
	}
}

