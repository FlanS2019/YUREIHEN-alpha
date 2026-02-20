#pragma execution_character_set("utf-8")

#include <d3d11.h>
#include <DirectXMath.h>
#include <cmath>
#include <functional>
using namespace DirectX;
#include "UI_Tutorial.h"
#include "keyboard.h"
#include "mouse.h"
#include "field.h"
#include "define.h"
#include "ghost.h"
#include "camera.h"
#include "game.h"
#include <windows.h>
#include "debug_ostream.h"

// ==========================================
// チュートリアル画面用の変数
// ==========================================
static bool g_IsTutorial    = false;
static bool g_IsPreTutorial = true;
static bool g_IsWaiting     = false; // 条件待機中（ゲームに制御を返している）
static HoleSprite* g_pTutorialBG = nullptr;

// フロア変更（シーン遷移）後、指定フレームだけ開始を遅らせる
static int g_TutorialDelayFrames = 0;

// 待機中ビネットのアニメーション
static float g_VignetteRadius    = 0.0f;   // 現在の穴半径
static bool  g_VignetteFadingOut = false;  // true=縮小中（チュートリアル再開前）
static const float VIGNETTE_RADIUS_TARGET = 500.0f;
static const float VIGNETTE_ANIM_SPEED    = 20.0f; // 1フレームあたりの変化量


namespace {
	// ページ管理
	std::vector<TutorialPage> g_Pages;
	int g_CurrentPage = 0;
	int g_NextPage = -1; // クロスフェード先ページ

	// フェード状態
	enum class TutorialState {
		FadeIn,
		Active,
		CrossFade // クロスフェード中（旧ページ→新ページ）
	};
	TutorialState g_State = TutorialState::FadeIn;
	float g_FadeAlpha = 0.0f;
	float g_CrossFadeProgress = 0.0f; // 0.0=旧ページ, 1.0=新ページ

	// スキップ機能
	float g_SkipHoldTime = 0.0f;
	const float SKIP_HOLD_REQUIRED = 1.5f; // 1.5秒長押しでスキップ

	// クロスフェード速度（1フレームあたりの進行量）
	const float CROSSFADE_SPEED = 0.04f; // 25フレーム（約0.4秒）でクロスフェード完了
}

// ページ案内用テキスト（共通）
static FontRenderer* g_pGuideFont = nullptr;

// スキップ案内テキスト
static FontRenderer* g_pSkipGuideFont = nullptr;

// スキップバー背景・前景
static Sprite* g_pSkipBarBG = nullptr;
static Sprite* g_pSkipBarFG = nullptr;

// テストプレイ中の操作ヒント
static FontRenderer* g_pPlayHintFont = nullptr;

static void InitSkipUI()
{
	// スキップバー背景
	if (!g_pSkipBarBG) {
		g_pSkipBarBG = new Sprite(
			{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 80.0f },
			{ 300.0f, 10.0f },
			0,
			{ 0.3f, 0.3f, 0.3f, 0.6f },
			BLENDSTATE_ALFA,
			L"asset/texture/fade.png"
		);
	}
	// スキップバー前景（ゲージ）
	if (!g_pSkipBarFG) {
		g_pSkipBarFG = new Sprite(
			{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 80.0f },
			{ 300.0f, 10.0f },
			0,
			{ 1.0f, 0.85f, 0.2f, 0.9f },
			BLENDSTATE_ALFA,
			L"asset/texture/fade.png"
		);
	}

	// スキップ案内テキスト
	if (!g_pSkipGuideFont) {
		g_pSkipGuideFont = new FontRenderer(
			{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 110.0f },
			25.0f, 0.0f, { 1.0f, 0.85f, 0.2f, 1.0f },
			"[SPACE長押し] スキップ"
		);
	}

	// テストプレイ中の操作ヒント（左下）
	if (!g_pPlayHintFont) {
		g_pPlayHintFont = new FontRenderer(
			{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 30.0f },
			50.0f, 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f },
			"[W][A][S][D] 移動  [マウス] 視点"
		);
		g_pPlayHintFont->PreCacheGlyphs();
	}
}

static void FinalizeSkipUI()
{
	if (g_pSkipBarBG) {
		delete g_pSkipBarBG;
		g_pSkipBarBG = nullptr;
	}
	if (g_pSkipBarFG) {
		delete g_pSkipBarFG;
		g_pSkipBarFG = nullptr;
	}
	if (g_pSkipGuideFont) {
		delete g_pSkipGuideFont;
		g_pSkipGuideFont = nullptr;
	}
	if (g_pPlayHintFont) {
		delete g_pPlayHintFont;
		g_pPlayHintFont = nullptr;
	}
}

// ==========================================
// ページデータ初期化
// ==========================================}

// 通常ページ追加
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
	// waitCondition は nullptr（SPACEで次へ）
	page.waitCondition = nullptr;
}

// 条件待機ページ追加：
//   ページが表示された後 SPACE 押下でチュートリアルを一時停止し、
//   cond() が true を返したら自動で次ページへ進む
static void AddPageWithWait(const XMFLOAT2& holeCenter, float holeRadius,
	const std::vector<std::string>& texts,
	std::function<bool()> cond)
{
	AddPage(holeCenter, holeRadius, texts);
	g_Pages.back().waitCondition = cond;
}

// テストプレイページ追加：
//   ページが表示された瞬間に自動でテストプレイ開始し、
//   *pFlag が true になったら自動で次ページへ進む
static void AddTestPlay(const std::vector<std::string>& texts, bool* pFlag)
{
	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, texts);
	g_Pages.back().waitCondition = [pFlag]() -> bool {
		return pFlag && *pFlag;
	};
	g_Pages.back().autoWait = true;
}

// ==========================================
// 条件待機から次ページへ進む内部処理
// ==========================================
static void AdvanceToNextPage()
{
	int nextPage = g_CurrentPage + 1;
	if (nextPage >= (int)g_Pages.size())
	{
		UI_Tutorial_End();
		return;
	}
	g_NextPage = nextPage;
	g_State    = TutorialState::CrossFade;
	g_CrossFadeProgress = 0.0f;
}

static void InitPages()
{
	g_Pages.clear();

	// --- 0 : ウェルカムメッセージ ---
	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"遊んでくれてありがとう！「幽霊変」の遊び方を説明していくね！"
	});

	// --- 1 : キー操作説明 ---
	AddTestPlay({
		"まずは操作説明！",
		"[W][A][S][D] で移動、マウスで視点を動かせるよ",
		"前に進んで円盤に触れてみよう！"
	}, Game_GetEnbanTouchedPtr());

	AddPage({ 120.0f, 120.0f }, 150.0f, {
		"まずはタイマー！",
		"制限時間は１階につき２分。過ぎると強制的に負けちゃうよ"
		});

	AddPage({ 1023.0f, 83.0f }, 200.0f, {
		"これは「恐怖ゲージ」バスターズをうまく驚かせられると、溜まっていくよ！",
		"右まで貯めるとバスターズが逃げてステージクリア！次の階へ進もう"
		});

	AddPage({ 1163.0f, 201.0f }, 90.0f, {
		"これは「恐怖コンボ」。バスターズへの驚かせが連鎖すると、恐怖ゲージが倍増するよ！"
		});

	AddPage({ 147.0f, 563.0f }, 150.0f, {
		"これはミニマップ。動きの参考にしよう"
		});

	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 150.0f, {
		"あとは実際にやってみるターンを作る。一旦ゲームスタート（仮テキスト）"
		});

	for (auto& page : g_Pages)
	{
		for (auto* f : page.fonts)
		{
			if (f) f->PreCacheGlyphs();
		}
	}
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

	// クロスフェード中は新旧ページの穴位置を補間
	if (g_State == TutorialState::CrossFade &&
		g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
		g_NextPage    >= 0 && g_NextPage    < (int)g_Pages.size())
	{
		const TutorialPage& oldPage = g_Pages[g_CurrentPage];
		const TutorialPage& newPage = g_Pages[g_NextPage];
		float t = g_CrossFadeProgress;

		// 穴の位置と半径を線形補間
		XMFLOAT2 center;
		center.x = oldPage.holeCenter.x * (1.0f - t) + newPage.holeCenter.x * t;
		center.y = oldPage.holeCenter.y * (1.0f - t) + newPage.holeCenter.y * t;
		float radius = oldPage.holeRadius * (1.0f - t) + newPage.holeRadius * t;

		g_pTutorialBG->SetHoleCenterPx(center);
		g_pTutorialBG->SetHoleRadiusPx(radius * g_FadeAlpha);
	}
	else if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size()) {
		const TutorialPage& page = g_Pages[g_CurrentPage];
		g_pTutorialBG->SetHoleCenterPx(page.holeCenter);
		g_pTutorialBG->SetHoleRadiusPx(page.holeRadius * g_FadeAlpha);
	}
}

// ==========================================

void UI_Tutorial_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) return;

	g_IsTutorial    = false;
	g_IsPreTutorial = true;
	g_IsWaiting     = false;
	g_CurrentPage   = 0;
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

	g_State    = TutorialState::FadeIn;
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

	InitPages();

	g_pGuideFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 40.0f },
		30.0f, 0.0f, { 1.0f, 0.85f, 0.2f, 1.0f },
		"[SPACE] 次へ"
	);

	InitSkipUI();
	ApplyPageHole();
}

void UI_Tutorial_Finalize(void)
{
	FinalizePages();
	FinalizeSkipUI();

	if (g_pTutorialBG) { delete g_pTutorialBG; g_pTutorialBG = nullptr; }
	if (g_pGuideFont)  { delete g_pGuideFont;  g_pGuideFont  = nullptr; }
}

void UI_Tutorial_Update(void)
{
	// 条件待機中は条件をポーリングするだけ
	if (g_IsWaiting)
	{
		// ビネット半径アニメーション
		if (g_VignetteFadingOut)
		{
			// 縮小（500→0）
			g_VignetteRadius -= VIGNETTE_ANIM_SPEED;
			if (g_VignetteRadius <= 0.0f)
			{
				g_VignetteRadius = 0.0f;
				// アニメ完了→チュートリアル再開
				g_VignetteFadingOut = false;
				g_IsWaiting  = false;
				g_IsTutorial = true;
				// クロスフェードを使わず直接次ページへ進めてFadeIn
				g_CurrentPage = g_CurrentPage + 1;
				if (g_CurrentPage >= (int)g_Pages.size())
				{
					UI_Tutorial_End();
					return;
				}
				g_NextPage  = -1;
				g_FadeAlpha = 0.0f;
				g_State     = TutorialState::FadeIn;
				ApplyPageHole();
				Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
				ShowCursor(TRUE); // SetModeは次フレーム反映のため即時表示
				Mouse_SetVisible(true);
			}
		}
		else
		{
			// 拡大（0→500）
			g_VignetteRadius += VIGNETTE_ANIM_SPEED;
			if (g_VignetteRadius > VIGNETTE_RADIUS_TARGET)
				g_VignetteRadius = VIGNETTE_RADIUS_TARGET;

			// 条件ポーリング（拡大完了後のみ判定）
			if (g_VignetteRadius >= VIGNETTE_RADIUS_TARGET)
			{
				// 拡大完了：ここでマウスをロック＆非表示にしてプレイ開始
				Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
				Mouse_SetVisible(false);

				if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size())
				{
					auto& cond = g_Pages[g_CurrentPage].waitCondition;
					if (cond && cond())
					{
						g_VignetteFadingOut = true;
					}
				}
			}
		}
		return;
	}

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
			// autoWaitページはSPACEを押さずに即テストプレイ開始
			if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
				g_Pages[g_CurrentPage].autoWait)
			{
				g_IsTutorial        = false;
				g_IsWaiting         = true;
				g_VignetteRadius    = 0.0f;
				g_VignetteFadingOut = false;
				return;
			}

			// スキップ機能：スペース長押しでチュートリアル全体をスキップ
			if (Keyboard_IsKeyDown(KK_SPACE))
			{
				g_SkipHoldTime += 1.0f / FPS;
				if (g_SkipHoldTime >= SKIP_HOLD_REQUIRED)
				{
					// チュートリアル全体をスキップ
					UI_Tutorial_End();
					return;
				}
			}
			else
			{
				// スペースを離したとき
				if (g_SkipHoldTime > 0.0f && g_SkipHoldTime < 0.2f)
				{
					// 現在ページが条件待機ページか確認
					if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
						g_Pages[g_CurrentPage].waitCondition != nullptr)
					{
						// ゲームに制御を返す（一時停止解除）
						g_IsTutorial        = false;
						g_IsWaiting         = true;
						g_VignetteRadius    = 0.0f;
						g_VignetteFadingOut = false;
						// マウスロックはビネット拡大完了後に行う（下記else内）
					}
					else
					{
						// 通常ページ → クロスフェードで次へ
						g_NextPage = g_CurrentPage + 1;
						if (g_NextPage >= (int)g_Pages.size())
						{
							// 最終ページ → ゲームに戻る
							UI_Tutorial_End();
							return;
						}
						g_State = TutorialState::CrossFade;
						g_CrossFadeProgress = 0.0f;
					}
				}
				g_SkipHoldTime = 0.0f;
			}
			break;

		case TutorialState::CrossFade:
			g_CrossFadeProgress += CROSSFADE_SPEED;
			if (g_CrossFadeProgress >= 1.0f) {
				g_CrossFadeProgress = 1.0f;

				// 遷移完了：新ページをカレントに
				g_CurrentPage = g_NextPage;
				g_NextPage    = -1;
				g_State       = TutorialState::Active;
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
			g_IsTutorial    = true;
			g_CurrentPage   = 0;

			// チュートリアル開始時はフェードインから
			g_State    = TutorialState::FadeIn;
			g_FadeAlpha = 0.0f;
			ApplyPageHole();
		}
	}
}

void UI_Tutorial_Draw(void)
{
	// 待機中もビネット表示するためg_IsWaitingを条件に追加
	if (!g_IsPreTutorial && !g_IsTutorial && !g_IsWaiting) return;

	// 待機中は真ん中に大きな穴のビネット表示
	if (g_IsWaiting)
	{
		if (g_pTutorialBG)
		{
			g_pTutorialBG->SetHoleCenterPx({ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f });
			g_pTutorialBG->SetHoleRadiusPx(g_VignetteRadius);
			g_pTutorialBG->Draw();
		}

		// autoWaitページのテキストをビネットに重ねて表示
		if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
			g_Pages[g_CurrentPage].autoWait)
		{
			float textAlpha = g_VignetteRadius / VIGNETTE_RADIUS_TARGET;
			if (textAlpha > 1.0f) textAlpha = 1.0f;

			const TutorialPage& page = g_Pages[g_CurrentPage];
			for (auto* f : page.fonts)
			{
				if (f) {
					XMFLOAT4 col = f->GetColor();
					f->SetColor({ col.x, col.y, col.z, textAlpha });
					f->Draw();
					f->SetColor(col);
				}
			}
		}

		// 操作ヒントをビネット拡大完了後に表示（アルファをビネット進行に合わせる）
		if (g_pPlayHintFont && !g_VignetteFadingOut)
		{
			float hintAlpha = g_VignetteRadius / VIGNETTE_RADIUS_TARGET * 0.85f;
			XMFLOAT4 col = g_pPlayHintFont->GetColor();
			g_pPlayHintFont->SetColor({ col.x, col.y, col.z, hintAlpha });
			g_pPlayHintFont->Draw();
			g_pPlayHintFont->SetColor(col);
		}
		return;
	}

	// 暗幕描画
	if (g_pTutorialBG) g_pTutorialBG->Draw();

	// クロスフェード中は旧ページと新ページを同時描画
	if (g_State == TutorialState::CrossFade &&
		g_NextPage >= 0 && g_NextPage < (int)g_Pages.size())
	{
		float oldAlpha = g_FadeAlpha * (1.0f - g_CrossFadeProgress);
		float newAlpha = g_FadeAlpha * g_CrossFadeProgress;

		// 旧ページ描画（フェードアウト）
		if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size())
		{
			const TutorialPage& oldPage = g_Pages[g_CurrentPage];
			for (auto* s : oldPage.sprites) {
				if (s) {
					XMFLOAT4 col = s->GetColor();
					s->SetColor({ col.x, col.y, col.z, col.w * oldAlpha });
					s->Draw();
					s->SetColor(col);
				}
			}
			for (auto* f : oldPage.fonts) {
				if (f) {
					XMFLOAT4 col = f->GetColor();
					f->SetColor({ col.x, col.y, col.z, oldAlpha });
					f->Draw();
					f->SetColor(col);
				}
			}
		}

		// 新ページ描画（フェードイン）
		{
			const TutorialPage& newPage = g_Pages[g_NextPage];
			for (auto* s : newPage.sprites) {
				if (s) {
					XMFLOAT4 col = s->GetColor();
					s->SetColor({ col.x, col.y, col.z, col.w * newAlpha });
					s->Draw();
					s->SetColor(col);
				}
			}
			for (auto* f : newPage.fonts) {
				if (f) {
					XMFLOAT4 col = f->GetColor();
					f->SetColor({ col.x, col.y, col.z, newAlpha });
					f->Draw();
					f->SetColor(col);
				}
			}
		}
	}
	else
	{
		// 通常描画（FadeIn/Active状態）
		if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size())
		{
			const TutorialPage& page = g_Pages[g_CurrentPage];

			for (auto* s : page.sprites)
			{
				if (s) {
					XMFLOAT4 col = s->GetColor();
					XMFLOAT4 drawCol = { col.x, col.y, col.z, col.w * g_FadeAlpha };
					s->SetColor(drawCol);
					s->Draw();
					s->SetColor(col);
				}
			}
			for (auto* f : page.fonts)
			{
				if (f) {
					XMFLOAT4 col = f->GetColor();
					XMFLOAT4 drawCol = { col.x, col.y, col.z, g_FadeAlpha };
					f->SetColor(drawCol);
					f->Draw();
					f->SetColor(col);
				}
			}
		}
	}

	// 案内テキスト
	if (g_pGuideFont) {
		// 条件待機ページのときは案内テキストを変える
		bool isWaitPage = (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
			g_Pages[g_CurrentPage].waitCondition != nullptr);

		std::string guideText = isWaitPage ? "[SPACE] でゲームを再開" : "[SPACE] 次へ";
		g_pGuideFont->SetText(guideText);

		XMFLOAT4 col = g_pGuideFont->GetColor();
		g_pGuideFont->SetColor({ col.x, col.y, col.z, g_FadeAlpha });
		g_pGuideFont->Draw();
		g_pGuideFont->SetColor(col);
	}

	// スキップUI描画
	if (g_IsTutorial && g_pSkipBarBG && g_pSkipBarFG && g_pSkipGuideFont)
	{
		// スキップバー背景
		{
			XMFLOAT4 col = g_pSkipBarBG->GetColor();
			g_pSkipBarBG->SetColor({ col.x, col.y, col.z, col.w * g_FadeAlpha });
			g_pSkipBarBG->Draw();
			g_pSkipBarBG->SetColor(col);
		}
		// スキップゲージ（幅をスキップ進捗に応じて変更）
		{
			float skipRatio = g_SkipHoldTime / SKIP_HOLD_REQUIRED;
			if (skipRatio > 1.0f) skipRatio = 1.0f;
			float barFullWidth = 300.0f;
			float barWidth     = barFullWidth * skipRatio;

			XMFLOAT4 col = g_pSkipBarFG->GetColor();
			g_pSkipBarFG->SetColor({ col.x, col.y, col.z, col.w * g_FadeAlpha });

			XMFLOAT2 origSize = g_pSkipBarFG->GetScale();
			XMFLOAT2 origPos  = g_pSkipBarFG->GetPos();
			float leftEdge = SCREEN_WIDTH / 2.0f - barFullWidth / 2.0f;
			g_pSkipBarFG->SetSize({ barWidth, origSize.y });
			g_pSkipBarFG->SetPos({ leftEdge + barWidth / 2.0f, origPos.y });

			if (barWidth > 0.1f) g_pSkipBarFG->Draw();

			g_pSkipBarFG->SetSize(origSize);
			g_pSkipBarFG->SetPos(origPos);
			g_pSkipBarFG->SetColor(col);
		}
		// スキップ案内テキスト
		{
			XMFLOAT4 col = g_pSkipGuideFont->GetColor();
			g_pSkipGuideFont->SetColor({ col.x, col.y, col.z, g_FadeAlpha });
			g_pSkipGuideFont->Draw();
			g_pSkipGuideFont->SetColor(col);
		}
	}
}

bool UI_Tutorial_IsActive(void)
{
	return g_IsTutorial;
}

bool UI_Tutorial_IsWaiting(void)
{
	return g_IsWaiting;
}

// 条件待機中に外部から「条件達成」を通知して次ページへ進む
void UI_Tutorial_ResumeFromWait(void)
{
	if (!g_IsWaiting) return;

	// 縮小アニメを開始（Update内で完了後に復帰する）
	g_VignetteFadingOut = true;
}

void UI_Tutorial_Start()
{
	if (g_IsTutorial || g_IsPreTutorial) return;

	g_IsTutorial  = true;
	g_IsWaiting   = false;
	g_CurrentPage = 0;
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

	g_State    = TutorialState::FadeIn;
	g_FadeAlpha = 0.0f;
	g_SkipHoldTime = 0.0f;

	ApplyPageHole();
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);
}

void UI_Tutorial_End()
{
	g_IsTutorial    = false;
	g_IsPreTutorial = false;
	g_IsWaiting     = false;
	g_CurrentPage   = 0;
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
	Mouse_SetVisible(false);
}