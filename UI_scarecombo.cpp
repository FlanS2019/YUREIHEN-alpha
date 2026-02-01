#include "UI_scarecombo.h"
#include "UI.h"
#include "debug_ostream.h"
#include <windows.h>

#define SCARECOMBO_POS_X (SCREEN_WIDTH - 110.0f)
#define SCARECOMBO_POS_Y (170.0f)
#define SCARECOMBO_MAX (5)
#define SCARECOMBO_OVER_TIME (10000)// ミリ秒
#define SCARECOMBO_BAR_SIZE_X (140.0f)
#define SCARECOMBO_BAR_POS_X (SCARECOMBO_POS_X - 70.0f)


// グローバル変数
static Sprite* g_ScareComboBG = nullptr;//恐怖コンボの背景
static Number* g_ScareCombo = nullptr;
static Sprite* g_ScareComboBar = nullptr;//恐怖コンボの時間切れを表示
ULONGLONG g_StartTime = GetTickCount64();
ULONGLONG g_KeikaTime = GetTickCount64();


 // 恐怖コンボの初期化
void UI_ScareCombo_Initialize(void)
{
	g_ScareComboBG = new Sprite(
		{ SCARECOMBO_POS_X, SCARECOMBO_POS_Y },	// 位置
		{ 200.0f, 200.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\kanban.png"
	);

	g_ScareCombo = new Number(
		{ SCARECOMBO_POS_X + 25.0f, SCARECOMBO_POS_Y + 40.0f },	// 位置
		{ 70.0f, 70.0f },					// サイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },			// RGBA
		BLENDSTATE_ALFA,					// BlendState
		L"asset\\texture\\num.png",			// テクスチャパス
		5, 3,								// 分割数X, Y
		55.0f								// 桁間隔
	);

	g_ScareComboBar = new Sprite(
		{ SCARECOMBO_BAR_POS_X, SCARECOMBO_POS_Y + 70.0f},	// 位置
		{ SCARECOMBO_BAR_SIZE_X, 20.0f },					// サイズ
		0.0f,
		{ 1.0f, 1.0f, 0.0f, 1.0f },			// RGBA
		BLENDSTATE_ALFA,					// BlendState
		L"asset\\texture\\fade.png"			// テクスチャパス
	);

	g_ScareCombo->SetShowX(true); // 倍数接頭子「x」を表示
	g_ScareCombo->SetNumber(1);
	g_StartTime = GetTickCount64();
	g_KeikaTime = GetTickCount64();
}

// 恐怖コンボの更新
void UI_ScareCombo_Update(void)
{
	g_KeikaTime = GetTickCount64();

	// 恐怖コンボの残り時間バーのスケール更新
	if (g_ScareCombo->GetNumber() > 1)
	{
		g_ScareComboBar->SetPosX(SCARECOMBO_BAR_POS_X);

		float elapsed = static_cast<float>(g_KeikaTime - g_StartTime);
		float ratio = 1.0f - (elapsed / SCARECOMBO_OVER_TIME);
		if (ratio < 0.0f)
		{
			ratio = 0.0f;
		}

		g_ScareComboBar->SetScaleX(SCARECOMBO_BAR_SIZE_X * ratio);
		g_ScareComboBar->AddPosX((SCARECOMBO_BAR_SIZE_X * (ratio / 2)));
	}

	// 恐怖コンボ切れ処理
	if (g_KeikaTime - g_StartTime >= SCARECOMBO_OVER_TIME && g_ScareCombo->GetNumber() != 1)
	{
		g_ScareCombo->SetNumber(1);
	}
}

// 恐怖コンボの描画
void UI_ScareCombo_Draw(void)
{
	g_ScareComboBG->Draw();

	// 恐怖コンボの残り時間バーは1コンボ以上のときのみ表示
	if (g_ScareCombo->GetNumber() > 1)
	{
		g_ScareComboBar->Draw();
	}
	g_ScareCombo->Draw();
}

// 恐怖コンボの終了
void UI_ScareCombo_Finalize(void)
{
	delete g_ScareComboBG;
	g_ScareComboBG = nullptr;

	delete g_ScareCombo;
	g_ScareCombo = nullptr;

	delete g_ScareComboBar;
	g_ScareComboBar = nullptr;
}

void ScareComboUP(void)
{
	g_StartTime = GetTickCount64();
	//g_ScareComboNumを加算
	g_ScareCombo->AddNumber(1);
	//5を超えないように
	if (g_ScareCombo->GetNumber() > SCARECOMBO_MAX)
	{
		g_ScareCombo->SetNumber(SCARECOMBO_MAX);
	}
}

void UI_ScareCombo_Reset(void)
{
	if (g_ScareCombo)
	{
		g_ScareCombo->SetNumber(1);
	}
}

int UI_ScareCombo_GetNumber(void)
{
	return g_ScareCombo->GetNumber();
}
