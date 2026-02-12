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
#include "debug_ostream.h"

// ==========================================
// チュートリアル画面用の変数
// ==========================================
static bool g_IsTutorial = false;
static bool g_IsPreTutorial = true;
static HoleSprite* g_pTutorialBG = nullptr;

namespace {
	// ページ管理
	std::vector<TutorialPage> g_Pages;
	int g_CurrentPage = 0;

	// フェード状態
	enum class TutorialState {
		FadeIn,
		Active,
		FadeOut
	};
	TutorialState g_State = TutorialState::FadeIn;
	float g_FadeAlpha = 0.0f;
}

// ページ案内用テキスト（共通）
static FontRenderer* g_pGuideFont = nullptr;

// フロア変更（シーン遷移）後、指定フレームだけ開始を遅らせる
static int g_TutorialDelayFrames = 0;

// ==========================================
// ページデータ初期化
// ==========================================
static void AddPage(const XMFLOAT2& holeCenter, float holeRadius, const std::vector<std::string>& texts)
{
	g_Pages.emplace_back();
	TutorialPage& page = g_Pages.back();

	page.holeCenter = holeCenter;
	page.holeRadius = holeRadius;

	float yOffset = 0.0f;
	for (const auto& text : texts)
	{
		page.fonts.push_back(new FontRenderer(
			{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f + yOffset },
			40.0f, 0.0f, { 1,1,1,1 }, text
		));
		yOffset += 50.0f;
	}
}

static void InitPages()
{
	// ページ生成前にクリア
	g_Pages.clear();

	// --- 0 ---
	AddPage({ SCREEN_WIDTH / 2,SCREEN_HEIGHT / 2 }, 0.0f, {
		"遊んでくれてありがとう！「幽霊変」の遊び方を説明していくね！"
		});

	// --- 1 ---
	AddPage({ 120.0f, 120.0f }, 150.0f, {
		"まずはタイマー！",
		"制限時間は１階につき２分。過ぎると強制的に負けちゃうよ"
		});

	// --- 2 ---
	AddPage({ 1023.0f, 83.0f }, 200.0f, {
		"これは「恐怖ゲージ」バスターズをうまく驚かせられると、溜まっていくよ！",
		"右まで貯めるとバスターズが逃げてステージクリア！次の階へ進もう"
		});

	// --- 3 ---
	AddPage({ 1163.0f, 201.0f }, 90.0f, {
		"これは「恐怖コンボ」。バスターズへの驚かせが連鎖すると、恐怖ゲージが倍増するよ！"
		});

	// --- 4 ---
	AddPage({ 147.0f, 563.0f }, 150.0f, {
		"これはミニマップ。動きの参考にしよう"
		});

	// --- 5 ---
	AddPage({ SCREEN_WIDTH / 2,SCREEN_HEIGHT / 2 }, 150.0f, {
		"あとは実際にやってみるターンを作る。一旦ゲームスタート（仮テキスト）"
		});
}

// ==========================================
// ページデータ解放
// ==========================================
static void FinalizePages()
{
	for (auto& page : g_Pages)
	{
		for (auto* s : page.sprites) { delete s; }
		page.sprites.clear();

		for (auto* f : page.fonts) { delete f; }
		page.fonts.clear();
	}
	g_Pages.clear();
}

// ==========================================
// 現在ページの穴位置を暗幕に反映
// ==========================================
static void ApplyPageHole()
{
	if (!g_pTutorialBG) return;
	if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size()) {
		const TutorialPage& page = g_Pages[g_CurrentPage];
		g_pTutorialBG->SetHoleCenterPx(page.holeCenter);
		// フェードAlphaを掛けて穴の開閉を行う
		g_pTutorialBG->SetHoleRadiusPx(page.holeRadius * g_FadeAlpha);
	}
}

// ==========================================

void UI_Tutorial_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) return;

	g_IsTutorial = false;
	g_IsPreTutorial = true;
	g_CurrentPage = 0;
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

	// フェード初期化
	g_State = TutorialState::FadeIn;
	g_FadeAlpha = 0.0f;

	g_pTutorialBG = new HoleSprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0,
		{ 0,0,0,0.7f },
		BLENDSTATE_ALFA,
		L"asset/texture/fade.png"
	);
	g_pTutorialBG->SetHoleSoftnessPx(8.0f);

	// ページデータ構築
	InitPages();

	// ページ案内テキスト
	g_pGuideFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 40.0f },
		30.0f, 0.0f, { 1.0f, 0.85f, 0.2f, 1.0f },
		"[SPACE] 次へ"
	);

	// 初期ページの穴位置を適用
	ApplyPageHole();
}

void UI_Tutorial_Finalize(void)
{
	FinalizePages();

	if (g_pTutorialBG) {
		delete g_pTutorialBG;
		g_pTutorialBG = nullptr;
	}

	if (g_pGuideFont) {
		delete g_pGuideFont;
		g_pGuideFont = nullptr;
	}
}

void UI_Tutorial_Update(void)
{
	// チュートリアル中の更新
	if (g_IsTutorial)
	{
#if defined(DEBUG) || defined(_DEBUG)
		{
			static bool s_PrevLeft = false;
			Mouse_State ms;
			Mouse_GetState(&ms);
			if (ms.leftButton && !s_PrevLeft)
			{
				hal::dout << "{" << ms.x << "," << ms.y << "}" << std::endl;
			}
			s_PrevLeft = ms.leftButton;
		}
#endif

	// フェード更新ステートマシン
	switch (g_State)
	{
	case TutorialState::FadeIn:
		g_FadeAlpha += 0.05f;
		if (g_FadeAlpha >= 1.0f) {
			g_FadeAlpha = 1.0f;
			g_State = TutorialState::Active;
		}
		ApplyPageHole();
		break;

	case TutorialState::Active:
		if (Keyboard_IsKeyDownTrigger(KK_SPACE))
		{
			g_State = TutorialState::FadeOut;
		}
		break;

	case TutorialState::FadeOut:
		g_FadeAlpha -= 0.05f;
		if (g_FadeAlpha <= 0.0f) {
			g_FadeAlpha = 0.0f;

			// 次のページへ
			g_CurrentPage++;
			if (g_CurrentPage >= (int)g_Pages.size())
			{
				// 最終ページを終了 → ゲームに戻る
				UI_Tutorial_End();
				return;
			}

			// 次のページ開始
			g_State = TutorialState::FadeIn;
			ApplyPageHole();
		}
		ApplyPageHole();
		break;
	}
	}
	// チュートリアル開始後のカメラ初期化待ち
	else if (g_IsPreTutorial)
	{
		g_TutorialDelayFrames--;

		if (g_TutorialDelayFrames <= 0)
		{
			g_IsPreTutorial = false;
			g_IsTutorial = true;
			g_CurrentPage = 0;

			// チュートリアル開始時はフェードインから
			g_State = TutorialState::FadeIn;
			g_FadeAlpha = 0.0f;
			ApplyPageHole();
		}
	}
}

void UI_Tutorial_Draw(void)
{
	if (g_IsPreTutorial || g_IsTutorial)
	{
		// 暗幕描画
		if (g_pTutorialBG) g_pTutorialBG->Draw();

		// 現在ページのスプライト・テキスト描画
		if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size())
		{
			const TutorialPage& page = g_Pages[g_CurrentPage];

			for (auto* s : page.sprites)
			{
				if (s) {
					// アルファ適用
					XMFLOAT4 col = s->GetColor();
					XMFLOAT4 drawCol = { col.x, col.y, col.z, col.w * g_FadeAlpha };
					s->SetColor(drawCol);
					s->Draw();
					s->SetColor(col); // 戻しておく
				}
			}
			for (auto* f : page.fonts)
			{
				if (f) {
					// アルファ適用
					XMFLOAT4 col = f->GetColor();
					XMFLOAT4 drawCol = { col.x, col.y, col.z, g_FadeAlpha }; // 文字は完全不透明ベースなのでそのままAlpha適用
					f->SetColor(drawCol);
					f->Draw();
					f->SetColor(col); // 戻しておく
				}
			}
		}

		// 案内テキスト（これも一緒にフェードさせるか、常時表示させるかだが、一緒にフェードさせるほうが見栄えが良い）
		if (g_pGuideFont) {
			XMFLOAT4 col = g_pGuideFont->GetColor();
			XMFLOAT4 drawCol = { col.x, col.y, col.z, g_FadeAlpha };
			g_pGuideFont->SetColor(drawCol);
			g_pGuideFont->Draw();
			g_pGuideFont->SetColor(col);
		}
	}
}

bool UI_Tutorial_IsActive(void)
{
	return g_IsTutorial;
}

void UI_Tutorial_Start()
{
	if (g_IsTutorial || g_IsPreTutorial) return;

	g_IsTutorial = true;
	g_CurrentPage = 0;
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

	ApplyPageHole();

	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);
}

void UI_Tutorial_End()
{
	g_IsTutorial = false;
	g_IsPreTutorial = false;
	g_CurrentPage = 0;
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
	Mouse_SetVisible(false);
}