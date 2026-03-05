#pragma execution_character_set("utf-8")

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "UI_RetryMenu.h"
#include "sprite.h"
#include "ClickFont.h"
#include "keyboard.h"
#include "mouse.h"
#include "camera.h"
#include "scene.h"
#include "define.h"
#include "fade.h"
#include "field.h"
#include "ghost.h"
#include "busters.h"
#include "UI.h"
#include "shader.h"
#include "game.h"
#include <windows.h>
#include <string>

// ==========================================
// リトライメニュー用の変数
// ==========================================
static bool g_IsActive = false;
static bool g_RetryPending = false; // リトライフェード待ち中
static int  g_Cursor = 0; // 0:同じ階からやり直す, 1:タイトルへ戻る

static Sprite* g_pBG = nullptr;
static ClickFont* g_pRetryButtonFont = nullptr;
static ClickFont* g_pTitleButtonFont = nullptr;
static FontRenderer* g_pMessageFont = nullptr;

// ==========================================
// リトライ実行（フェードMAX到達後に呼ばれる）
// ==========================================
static void DoRetryExec(void)
{
	// Loseアニメのリソース解放＋状態リセット＋BGM再開
	Game_EndLoseAnim();

	// マウスをゲームモードに戻す
	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
	ShowCursor(FALSE);

	// 現在の階を取得
	int currentFloor = Field_GetCurrentFloor();

	// ゴーストをリセットして同じ階のスタート位置に戻す
	Ghost* ghost = GetGhost();
	if (ghost)
	{
		ghost->ResetPos();
		XMFLOAT3 spawnPos = GetGhostStartPos(currentFloor);
		ghost->SetPos(spawnPos);
		ghost->SetState(GS_MOVING);
		ghost->SetIsDraw(true);
		ghost->SetIsDetectedByBuster(false);
		ghost->m_InvincibleTimer = 0;
		ghost->m_EscapeTapCount = 0;
		ghost->m_CaughtPenaltyTimer = 0;
		Camera_SetTargetPos(spawnPos);
	}

	// バスターズを同じ階に再生成
	Busters_SpawnOnFloor(currentFloor);

	// ゲージとタイマーをリセット
	UI_ResetScareGauge();
	UI_ResetTimer();

	// フェードイン開始
	Fade_StartIn();

	// マウス復帰
	Mouse_ReacquireFocus();
	Camera* cam = GetCamera();
	if (cam) cam->SkipNextInput(5);

	g_IsActive = false;
	g_RetryPending = false;
}

// ==========================================
// リトライ開始（フェードアウトを開始する）
// ==========================================
static void DoRetry(void)
{
	if (g_RetryPending) return;
	g_RetryPending = true;

	// フェードアウト開始（シーン遷移なし）
	StartFade(SCENE_NONE);
}

static void DoTitle(void)
{
	g_IsActive = false;

	// Loseアニメのリソース解放＋状態リセット
	Game_EndLoseAnim();

	StartFade(SCENE_TITLE);
}

// ==========================================
// 初期化
// ==========================================
void UI_RetryMenu_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (g_pRetryButtonFont != nullptr) return;
	if (!pDevice || !pContext) return;

	g_IsActive = false;
	g_RetryPending = false;
	g_Cursor = 0;

	// 半透明背景
	g_pBG = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0,
		{ 0, 0, 0, 0.75f },
		BLENDSTATE_ALFA,
		L"asset/texture/fade.png");

	float bx = SCREEN_WIDTH / 2.0f;
	float by = SCREEN_HEIGHT / 2.0f - 30.0f;
	float gap = 80.0f;
	XMFLOAT4 normal = { 1, 1, 1, 1 };
	XMFLOAT4 hover = { 1.0f, 0.85f, 0.2f, 1.0f };

	// メッセージ
	g_pMessageFont = new FontRenderer(
		{ bx, by - 80.0f },
		48.0f,
		0.0f,
		{ 1.0f, 0.6f, 0.6f, 1.0f },
		"同じ階からやり直す？"
	);

	// やり直すボタン
	g_pRetryButtonFont = new ClickFont(
		{ bx, by + 20.0f }, 40.0f, 0.0f, normal, hover, "やり直す");
	g_pRetryButtonFont->SetHitSize({ 300.0f, 50.0f });
	g_pRetryButtonFont->SetOnClick([]() {
		DoRetry();
	});

	// タイトルへ戻るボタン
	g_pTitleButtonFont = new ClickFont(
		{ bx, by + 20.0f + gap }, 40.0f, 0.0f, normal, hover, "タイトルへ戻る");
	g_pTitleButtonFont->SetHitSize({ 360.0f, 50.0f });
	g_pTitleButtonFont->SetOnClick([]() {
		DoTitle();
	});
}

// ==========================================
// 終了
// ==========================================
void UI_RetryMenu_Finalize(void)
{
	if (g_pBG) { delete g_pBG; g_pBG = nullptr; }
	if (g_pRetryButtonFont) { delete g_pRetryButtonFont; g_pRetryButtonFont = nullptr; }
	if (g_pTitleButtonFont) { delete g_pTitleButtonFont; g_pTitleButtonFont = nullptr; }
	if (g_pMessageFont) { delete g_pMessageFont; g_pMessageFont = nullptr; }
}

// ==========================================
// 表示開始
// ==========================================
void UI_RetryMenu_Show(void)
{
	if (g_IsActive) return;
	g_IsActive = true;
	g_Cursor = 0;

	// マウスを絶対モード＋表示にする
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	ShowCursor(TRUE);
}

// ==========================================
// 更新
// ==========================================
void UI_RetryMenu_Update(void)
{
	if (!g_IsActive) return;

	// リトライフェード待ち中：FADE_MAXになったらリセット実行
	if (g_RetryPending)
	{
		if (GetFadeState() == FADE_MAX)
		{
			DoRetryExec();
		}
		return;
	}

	// ClickFont更新
	if (g_pRetryButtonFont) g_pRetryButtonFont->Update();
	if (g_pTitleButtonFont) g_pTitleButtonFont->Update();

	// マウスホバーでカーソル更新
	if (g_pRetryButtonFont && g_pRetryButtonFont->IsHover()) g_Cursor = 0;
	else if (g_pTitleButtonFont && g_pTitleButtonFont->IsHover()) g_Cursor = 1;

	// キーボード操作
	if (Keyboard_IsKeyDownTrigger(KK_UP) || Keyboard_IsKeyDownTrigger(KK_W))
	{
		g_Cursor--;
		if (g_Cursor < 0) g_Cursor = 1;
	}
	if (Keyboard_IsKeyDownTrigger(KK_DOWN) || Keyboard_IsKeyDownTrigger(KK_S))
	{
		g_Cursor++;
		if (g_Cursor > 1) g_Cursor = 0;
	}

	// Enterキーまたはスペースキーで決定
	if (Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		if (g_Cursor == 0) DoRetry();
		else DoTitle();
	}
}

// ==========================================
// 描画
// ==========================================
void UI_RetryMenu_Draw(void)
{
	if (!g_IsActive) return;

	if (g_pBG) g_pBG->Draw();
	if (g_pMessageFont) g_pMessageFont->Draw();

	// Sprite描画前に白にリセット
	Shader_SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	if (g_pRetryButtonFont) g_pRetryButtonFont->Draw();
	if (g_pTitleButtonFont) g_pTitleButtonFont->Draw();
}

// ==========================================
// アクティブ判定
// ==========================================
bool UI_RetryMenu_IsActive(void)
{
	return g_IsActive;
}
