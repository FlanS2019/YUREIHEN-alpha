#pragma execution_character_set("utf-8")

#include <d3d11.h>
#include <DirectXMath.h>
#include <cmath>
#include <functional>
#include "UI_Tutorial.h"
#include "keyboard.h"
#include "mouse.h"
#include "field.h"
#include "define.h"
#include "ghost.h"
#include "camera.h"
#include "Tutorial_Object.h"
#include <windows.h>
#include "debug_ostream.h"
#include "UI_Tutorial_Internal.h"
using namespace DirectX;

// ==========================================
// チュートリアル画面用の変数
// ==========================================
static bool g_IsTutorial = false;
static bool g_IsPreTutorial = true;
static bool g_IsWaiting = false;
static HoleSprite* g_pTutorialBG = nullptr;

static int g_TutorialDelayFrames = 0;

static float g_VignetteRadius = 0.0f;
static bool  g_VignetteFadingOut = false;
static const float VIGNETTE_RADIUS_TARGET = 500.0f;
static const float VIGNETTE_ANIM_SPEED = 20.0f;

static bool     g_CameraOverrideActive = false;
static XMFLOAT3 g_SavedCameraPos = { 0,0,0 };
static XMFLOAT3 g_SavedCameraAt = { 0,0,0 };

static bool     g_IsCameraTransitioning = false;
static XMFLOAT3 g_CamStartPos = { 0,0,0 };
static XMFLOAT3 g_CamStartAt = { 0,0,0 };
static XMFLOAT3 g_CamEndPos = { 0,0,0 };
static XMFLOAT3 g_CamEndAt = { 0,0,0 };

static int g_TotalPageCount = 0;

namespace {
	std::vector<TutorialPage> g_Pages;
	int g_CurrentPage = 0;
	int g_NextPage = -1;

	enum class TutorialState {
		FadeIn,
		Active,
		CrossFade
	};
	TutorialState g_State = TutorialState::FadeIn;
	float g_FadeAlpha = 0.0f;
	float g_CrossFadeProgress = 0.0f;

	float g_SkipHoldTime = 0.0f;
	const float SKIP_HOLD_REQUIRED = 1.5f;

	const float CROSSFADE_SPEED = 0.04f;

	// ページ番号カウンター（InitPages内でリセット）
	int s_PageCounter = 0;

	// 次に追加するページの onEnter に積むコールバックキュー
	std::vector<std::function<void()>> s_PendingOnEnter;
}

// テキストガイド用フォント
static FontRenderer* g_pGuideFont = nullptr;

// スキップバー関連
static FontRenderer* g_pSkipGuideFont = nullptr;
static Sprite* g_pSkipBarBG = nullptr;
static Sprite* g_pSkipBarFG = nullptr;

// プレイヒント用フォント
static FontRenderer* g_pPlayHintFont = nullptr;
// ページ番号表示用フォント
static FontRenderer* g_pPageCountFont = nullptr;

static void InitSkipUI()
{
	if (!g_pSkipBarBG) {
		g_pSkipBarBG = new Sprite(
			{ TUT_SKIPBAR_CENTER_X, TUT_SKIPBAR_CENTER_Y },
			{ TUT_SKIPBAR_WIDTH, TUT_SKIPBAR_HEIGHT },
			0,
			{ 0.3f, 0.3f, 0.3f, 0.6f },
			BLENDSTATE_ALFA,
			L"asset/texture/fade.png"
		);
	}
	if (!g_pSkipBarFG) {
		g_pSkipBarFG = new Sprite(
			{ TUT_SKIPBAR_CENTER_X, TUT_SKIPBAR_CENTER_Y },
			{ TUT_SKIPBAR_WIDTH, TUT_SKIPBAR_HEIGHT },
			0,
			{ 1.0f, 0.85f, 0.2f, 0.9f },
			BLENDSTATE_ALFA,
			L"asset/texture/fade.png"
		);
	}
	if (!g_pSkipGuideFont) {
		g_pSkipGuideFont = new FontRenderer(
			{ TUT_SKIPBAR_CENTER_X, TUT_SKIPBAR_CENTER_Y + TUT_SKIP_LABEL_OFFSET_Y },
			25.0f, 0.0f, { 1.0f, 0.85f, 0.2f, 1.0f },
			"[SPACE長押し] スキップ"
		);
	}
}

static void FinalizeSkipUI()
{
	if (g_pSkipBarBG)    { delete g_pSkipBarBG;    g_pSkipBarBG    = nullptr; }
	if (g_pSkipBarFG)    { delete g_pSkipBarFG;    g_pSkipBarFG    = nullptr; }
	if (g_pSkipGuideFont){ delete g_pSkipGuideFont; g_pSkipGuideFont = nullptr; }
	if (g_pPlayHintFont) { delete g_pPlayHintFont;  g_pPlayHintFont  = nullptr; }
	if (g_pPageCountFont){ delete g_pPageCountFont; g_pPageCountFont = nullptr; }
}

// ==========================================
// ページデータ初期化用ヘルパー
// ==========================================

// 保留中のコールバックをページの onEnter に適用するヘルパー
static void FlushPendingOnEnter(TutorialPage& page)
{
	if (s_PendingOnEnter.empty()) return;
	auto captured = s_PendingOnEnter;
	s_PendingOnEnter.clear();
	page.onEnter = [captured]() {
		for (auto& fn : captured) fn();
	};
}

// 通常ページ追加
void AddPage(const XMFLOAT2& holeCenter, float holeRadius,
	const std::vector<std::string>& texts,
	XMFLOAT2 textPos,
	float fontSize)
{
	g_Pages.emplace_back();
	TutorialPage& page = g_Pages.back();
	page.holeCenter = holeCenter;
	page.holeRadius = holeRadius;
	page.isPageType = true;
	page.pageNumber = ++s_PageCounter;

	FlushPendingOnEnter(page);

	float yOffset = 0.0f;
	for (const auto& text : texts)
	{
		page.fonts.push_back(new FontRenderer(
			{ textPos.x, textPos.y + yOffset },
			fontSize, 0.0f, { 1,1,1,1 }, text
		));
		yOffset += fontSize + 10.0f;
	}
	page.waitCondition = nullptr;
}

// テストプレイページ追加（AddPage_Play → ページ番号カウント対象）
void AddPage_Play(const std::vector<std::string>& texts, bool* pFlag,
	XMFLOAT2 textPos,
	float fontSize)
{
	g_Pages.emplace_back();
	TutorialPage& page = g_Pages.back();
	page.holeCenter = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
	page.holeRadius = 0.0f;
	page.isPageType = true;
	page.pageNumber = ++s_PageCounter;

	FlushPendingOnEnter(page);

	float yOffset = 0.0f;
	for (const auto& text : texts)
	{
		page.fonts.push_back(new FontRenderer(
			{ textPos.x, textPos.y + yOffset },
			fontSize, 0.0f, { 1,1,1,1 }, text
		));
		yOffset += fontSize + 10.0f;
	}
	page.waitCondition = [pFlag]() -> bool {
		return pFlag && *pFlag;
		};
	page.autoWait = true;
}

// カメラ移動ページ追加（ページ番号カウント対象）
void AddPage_Camera(const XMFLOAT2& holeCenter, float holeRadius,
	const std::vector<std::string>& texts,
	const XMFLOAT3& targetPos, const XMFLOAT3& targetAt,
	XMFLOAT2 textPos,
	float fontSize)
{
	AddPage(holeCenter, holeRadius, texts, textPos, fontSize);
	TutorialPage& page = g_Pages.back();
	page.cameraOverride = true;
	page.cameraPos = targetPos;
	page.cameraAt = targetAt;
}

// カメラの注視点を変える（通し番号カウント対象、ページ番号はカウントしない）
void SetCameraFocusPoint(const XMFLOAT3& pos)
{
	g_Pages.emplace_back();
	TutorialPage& page = g_Pages.back();
	page.holeCenter = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
	page.holeRadius = 0.0f;
	page.isPageType = false; // ページ番号カウントしない
	page.cameraOverride = true;
	page.cameraPos = pos;
	page.cameraAt = pos; // 注視点として使用

	// このページはテキストなし・自動で即次へ…ではなく
	// カメラオーバーライドエントリとして機能させるため autoWait=false, waitCondition=nullptr
	// （実際にはAddPage_Playの前段として登録し、そちらにカメラを委ねる設計）
	page.waitCondition = nullptr;
	page.autoWait = false;
}

// チュートリアルマーカーの表示・非表示と位置を設定（次ページの onEnter に積む）
void SetTutorialMarker(bool use, const XMFLOAT3& pos)
{
	s_PendingOnEnter.push_back([use, pos]() {
		TutorialMarker* m = GetTutorialMarker();
		if (!m) return;
		if (use)
		{
			m->SetPos(pos);
			m->SetVisible(true);
		}
		else
		{
			m->SetVisible(false);
		}
	});
}

// チュートリアルバスターズの表示・非表示と位置を設定（次ページの onEnter に積む）
void SetTutorialBuster(bool use, const XMFLOAT3& pos)
{
	s_PendingOnEnter.push_back([use, pos]() {
		TutorialBusters* b = GetTutorialBusters();
		if (!b) return;
		if (use)
		{
			b->SetPos(pos);
			TutorialObject_SetBustersVisible(true);
		}
		else
		{
			TutorialObject_SetBustersVisible(false);
		}
	});
}

// 円盤の表示・非表示（次ページの onEnter に積む）
void SetEnbanVisible(bool visible)
{
	s_PendingOnEnter.push_back([visible]() {
		TutorialObject_SetEnbanVisible(visible);
	});
}

// バスターズの調査対象座標を設定（次ページの onEnter に積む）
void SetTutorialBusterTarget(const XMFLOAT3& pos)
{
	s_PendingOnEnter.push_back([pos]() {
		TutorialBusters* b = GetTutorialBusters();
		if (b) b->SetTarget(pos);
	});
}

// ==========================================
// ページ遷移・カメラ処理
// ==========================================

static void SetupCameraTransition(int fromPage, int toPage)
{
	Camera* cam = GetCamera();
	if (!cam) return;

	bool toOverride = (toPage >= 0 && toPage < (int)g_Pages.size() && g_Pages[toPage].cameraOverride);
	bool fromOverride = (fromPage >= 0 && fromPage < (int)g_Pages.size() && g_Pages[fromPage].cameraOverride);

	if (toOverride || fromOverride)
	{
		g_IsCameraTransitioning = true;
		g_CamStartPos = cam->GetPos();
		g_CamStartAt = cam->GetAtPos();

		if (toOverride)
		{
			if (!g_CameraOverrideActive)
			{
				g_SavedCameraPos = cam->GetPos();
				g_SavedCameraAt = cam->GetAtPos();
				g_CameraOverrideActive = true;
			}
			const TutorialPage& p = g_Pages[toPage];
			g_CamEndPos = p.cameraPos;
			g_CamEndAt = p.cameraAt;
		}
		else if (fromOverride && g_CameraOverrideActive)
		{
			g_CamEndPos = g_SavedCameraPos;
			g_CamEndAt = g_SavedCameraAt;
		}
	}
	else
	{
		g_IsCameraTransitioning = false;
	}
}

static void AdvanceToNextPage()
{
	int nextPage = g_CurrentPage + 1;
	if (nextPage >= (int)g_Pages.size())
	{
		UI_Tutorial_End();
		return;
	}
	SetupCameraTransition(g_CurrentPage, nextPage);
	g_NextPage = nextPage;
	g_State = TutorialState::CrossFade;
	g_CrossFadeProgress = 0.0f;
}

// 現在の通し番号から表示ページ番号を取得
static int GetDisplayPageNumber(int serialIndex)
{
	if (serialIndex < 0 || serialIndex >= (int)g_Pages.size()) return 0;
	return g_Pages[serialIndex].pageNumber;
}

static void InitPages()
{
	g_Pages.clear();
	s_PageCounter = 0;
	g_TotalPageCount = 0;

	Tutorial_Pages_Init();

	g_TotalPageCount = s_PageCounter;

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
	g_TotalPageCount = 0;
}

// ==========================================
// 現在ページの穴位置を暗幕に反映
// ==========================================
static void ApplyPageHole()
{
	if (!g_pTutorialBG) return;

	if (g_State == TutorialState::CrossFade &&
		g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
		g_NextPage >= 0 && g_NextPage < (int)g_Pages.size())
	{
		const TutorialPage& oldPage = g_Pages[g_CurrentPage];
		const TutorialPage& newPage = g_Pages[g_NextPage];
		float t = g_CrossFadeProgress;

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

	g_IsTutorial = false;
	g_IsPreTutorial = true;
	g_IsWaiting = false;
	g_CurrentPage = 0;
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

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

	InitPages();

	g_pGuideFont = new FontRenderer(
		{ TUT_SKIPBAR_CENTER_X, TUT_SKIPBAR_CENTER_Y + TUT_GUIDE_OFFSET_Y },
		30.0f, 0.0f, { 1.0f, 0.85f, 0.2f, 1.0f },
		"[SPACE] 次へ"
	);

	g_pPageCountFont = new FontRenderer(
		{ TUT_SKIPBAR_CENTER_X, TUT_SKIPBAR_CENTER_Y + TUT_PAGECOUNT_OFFSET_Y },
		22.0f, 0.0f, { 1.0f, 0.85f, 0.2f, 1.0f },
		"ページ 1 / 1"
	);

	InitSkipUI();
	ApplyPageHole();
}

void UI_Tutorial_Finalize(void)
{
	FinalizePages();
	FinalizeSkipUI();

	if (g_pTutorialBG) { delete g_pTutorialBG; g_pTutorialBG = nullptr; }
	if (g_pGuideFont) { delete g_pGuideFont;  g_pGuideFont = nullptr; }
}

void UI_Tutorial_Update(void)
{
	if (g_IsWaiting)
	{
		if (g_VignetteFadingOut)
		{
			g_VignetteRadius -= VIGNETTE_ANIM_SPEED;
			if (g_VignetteRadius <= 0.0f)
			{
				g_VignetteRadius = 0.0f;

				g_VignetteFadingOut = false;
				g_IsWaiting = false;
				g_IsTutorial = true;
				g_CurrentPage = g_CurrentPage + 1;
				if (g_CurrentPage >= (int)g_Pages.size())
				{
					UI_Tutorial_End();
					return;
				}
				g_NextPage = -1;
				g_FadeAlpha = 0.0f;
				g_State = TutorialState::FadeIn;
				ApplyPageHole();
				Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
				ShowCursor(TRUE);
				Mouse_SetVisible(true);
			}
		}
		else
		{
			g_VignetteRadius += VIGNETTE_ANIM_SPEED;
			if (g_VignetteRadius > VIGNETTE_RADIUS_TARGET)
				g_VignetteRadius = VIGNETTE_RADIUS_TARGET;

			if (g_VignetteRadius >= VIGNETTE_RADIUS_TARGET)
			{
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

	switch (g_State)
	{
	case TutorialState::FadeIn:
		g_FadeAlpha += 0.05f;
		if (g_FadeAlpha >= 1.0f) {
			g_FadeAlpha = 1.0f;
			g_State = TutorialState::Active;
			// ページ表示開始時にコールバックを実行
			if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
				g_Pages[g_CurrentPage].onEnter)
			{
				g_Pages[g_CurrentPage].onEnter();
			}
		}
		ApplyPageHole();
		break;

	case TutorialState::Active:
		if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
			g_Pages[g_CurrentPage].autoWait)
		{
			g_IsTutorial = false;
			g_IsWaiting = true;
			g_VignetteRadius = 0.0f;
			g_VignetteFadingOut = false;
			g_FadeAlpha = 0.0f;
			return;
		}

		if (Keyboard_IsKeyDown(KK_SPACE))
		{
			g_SkipHoldTime += 1.0f / FPS;
			if (g_SkipHoldTime >= SKIP_HOLD_REQUIRED)
			{
				UI_Tutorial_End();
				return;
			}
		}
		else
		{
			if (g_SkipHoldTime > 0.0f && g_SkipHoldTime < 0.2f)
			{
				if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
					g_Pages[g_CurrentPage].waitCondition != nullptr)
				{
					g_IsTutorial = false;
					g_IsWaiting = true;
					g_VignetteRadius = 0.0f;
					g_VignetteFadingOut = false;
				}
				else
				{
					g_NextPage = g_CurrentPage + 1;
					if (g_NextPage >= (int)g_Pages.size())
					{
						UI_Tutorial_End();
						return;
					}
					SetupCameraTransition(g_CurrentPage, g_NextPage);
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

			g_CurrentPage = g_NextPage;
			g_NextPage = -1;
			g_IsCameraTransitioning = false;

			// ページ遷移完了時にコールバックを実行
			if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
				g_Pages[g_CurrentPage].onEnter)
			{
				g_Pages[g_CurrentPage].onEnter();
			}

			if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() && !g_Pages[g_CurrentPage].cameraOverride)
			{
				g_CameraOverrideActive = false;
			}

			if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
				g_Pages[g_CurrentPage].autoWait)
			{
				g_IsTutorial = false;
				g_IsWaiting = true;
				g_VignetteRadius = 0.0f;
				g_VignetteFadingOut = false;
				g_FadeAlpha = 0.0f;
				g_State = TutorialState::Active;
				return;
			}

			// isPageType=false（SetCameraFocusPointなど通過専用エントリ）は即次ページへ
			if (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
				!g_Pages[g_CurrentPage].isPageType &&
				!g_Pages[g_CurrentPage].autoWait &&
				g_Pages[g_CurrentPage].waitCondition == nullptr)
			{
				AdvanceToNextPage();
				return;
			}

			g_State = TutorialState::Active;
		}

		if (g_IsCameraTransitioning)
		{
			Camera* cam = GetCamera();
			if (cam)
			{
				float t = g_CrossFadeProgress;
				float smoothT = t * t * (3.0f - 2.0f * t);

				XMFLOAT3 pos;
				pos.x = g_CamStartPos.x + (g_CamEndPos.x - g_CamStartPos.x) * smoothT;
				pos.y = g_CamStartPos.y + (g_CamEndPos.y - g_CamStartPos.y) * smoothT;
				pos.z = g_CamStartPos.z + (g_CamEndPos.z - g_CamStartPos.z) * smoothT;

				XMFLOAT3 at;
				at.x = g_CamStartAt.x + (g_CamEndAt.x - g_CamStartAt.x) * smoothT;
				at.y = g_CamStartAt.y + (g_CamEndAt.y - g_CamStartAt.y) * smoothT;
				at.z = g_CamStartAt.z + (g_CamEndAt.z - g_CamStartAt.z) * smoothT;

				cam->UpdateView(pos, at);
			}
		}

		ApplyPageHole();
		break;
	}
	}
	else if (g_IsPreTutorial)
	{
		g_TutorialDelayFrames--;

		if (g_TutorialDelayFrames <= 0)
		{
			g_IsPreTutorial = false;
			g_IsTutorial = true;
			g_CurrentPage = 0;

			g_State = TutorialState::FadeIn;
			g_FadeAlpha = 0.0f;
			ApplyPageHole();
		}
	}
}

void UI_Tutorial_Draw(void)
{
	if (!g_IsPreTutorial && !g_IsTutorial && !g_IsWaiting) return;

	if (g_IsWaiting)
	{
		if (g_pTutorialBG)
		{
			g_pTutorialBG->SetHoleCenterPx({ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f });
			g_pTutorialBG->SetHoleRadiusPx(g_VignetteRadius);
			g_pTutorialBG->Draw();
		}

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

		if (g_pPlayHintFont)
		{
			float hintAlpha = g_VignetteRadius / VIGNETTE_RADIUS_TARGET * 0.85f;
			XMFLOAT4 col = g_pPlayHintFont->GetColor();
			g_pPlayHintFont->SetColor({ col.x, col.y, col.z, hintAlpha });
			g_pPlayHintFont->Draw();
			g_pPlayHintFont->SetColor(col);
		}
		return;
	}

	if (g_pTutorialBG) g_pTutorialBG->Draw();

	if (g_State == TutorialState::CrossFade &&
		g_NextPage >= 0 && g_NextPage < (int)g_Pages.size())
	{
		float oldAlpha = g_FadeAlpha * (1.0f - g_CrossFadeProgress);
		float newAlpha = g_FadeAlpha * g_CrossFadeProgress;

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

		{
			const TutorialPage& newPage = g_Pages[g_NextPage];
			bool newPageIsAutoWait = newPage.autoWait;

			for (auto* s : newPage.sprites) {
				if (s) {
					XMFLOAT4 col = s->GetColor();
					s->SetColor({ col.x, col.y, col.z, col.w * newAlpha });
					s->Draw();
					s->SetColor(col);
				}
			}
			if (!newPageIsAutoWait)
			{
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
	}
	else
	{
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

	bool crossFadingToAutoWait = (g_State == TutorialState::CrossFade &&
		g_NextPage >= 0 && g_NextPage < (int)g_Pages.size() &&
		g_Pages[g_NextPage].autoWait);

	if (g_pGuideFont && !crossFadingToAutoWait) {
		bool isWaitPage = (g_CurrentPage >= 0 && g_CurrentPage < (int)g_Pages.size() &&
			g_Pages[g_CurrentPage].waitCondition != nullptr);

		std::string guideText = isWaitPage ? "[SPACE] でゲームを再開" : "[SPACE] 次へ";
		g_pGuideFont->SetText(guideText);

		XMFLOAT4 col = g_pGuideFont->GetColor();
		g_pGuideFont->SetColor({ col.x, col.y, col.z, g_FadeAlpha });
		g_pGuideFont->Draw();
		g_pGuideFont->SetColor(col);
	}

	if (g_IsTutorial && g_pSkipBarBG && g_pSkipBarFG && g_pSkipGuideFont)
	{
		{
			XMFLOAT4 col = g_pSkipBarBG->GetColor();
			g_pSkipBarBG->SetColor({ col.x, col.y, col.z, col.w * g_FadeAlpha });
			g_pSkipBarBG->Draw();
			g_pSkipBarBG->SetColor(col);
		}
		{
			float skipRatio = g_SkipHoldTime / SKIP_HOLD_REQUIRED;
			if (skipRatio > 1.0f) skipRatio = 1.0f;
			float barWidth = TUT_SKIPBAR_WIDTH * skipRatio;

			XMFLOAT4 col = g_pSkipBarFG->GetColor();
			g_pSkipBarFG->SetColor({ col.x, col.y, col.z, col.w * g_FadeAlpha });

			XMFLOAT2 origSize = g_pSkipBarFG->GetScale();
			XMFLOAT2 origPos = g_pSkipBarFG->GetPos();
			float leftEdge = TUT_SKIPBAR_CENTER_X - TUT_SKIPBAR_WIDTH / 2.0f;
			g_pSkipBarFG->SetSize({ barWidth, origSize.y });
			g_pSkipBarFG->SetPos({ leftEdge + barWidth / 2.0f, origPos.y });

			if (barWidth > 0.1f) g_pSkipBarFG->Draw();

			g_pSkipBarFG->SetSize(origSize);
			g_pSkipBarFG->SetPos(origPos);
			g_pSkipBarFG->SetColor(col);
		}
		{
			XMFLOAT4 col = g_pSkipGuideFont->GetColor();
			g_pSkipGuideFont->SetColor({ col.x, col.y, col.z, g_FadeAlpha });
			g_pSkipGuideFont->Draw();
			g_pSkipGuideFont->SetColor(col);
		}
	}

	// ページ数表示（ページ番号ベース）
	if (g_pPageCountFont && !crossFadingToAutoWait)
	{
		// 表示するページ番号：クロスフェード中は遷移先を使用
		int serialForDisplay = (g_State == TutorialState::CrossFade && g_NextPage >= 0)
			? g_NextPage : g_CurrentPage;
		int displayPage = GetDisplayPageNumber(serialForDisplay);

		// isPageType=false のエントリは番号0扱いなので、直前の有効ページ番号を探す
		if (displayPage == 0 && serialForDisplay > 0)
		{
			for (int i = serialForDisplay - 1; i >= 0; --i)
			{
				if (g_Pages[i].isPageType) { displayPage = g_Pages[i].pageNumber; break; }
			}
		}

		char pageText[64];
		sprintf_s(pageText, "%d / %d ページ", displayPage, g_TotalPageCount);
		g_pPageCountFont->SetText(pageText);

		XMFLOAT4 col = g_pPageCountFont->GetColor();
		g_pPageCountFont->SetColor({ col.x, col.y, col.z, g_FadeAlpha });
		g_pPageCountFont->Draw();
		g_pPageCountFont->SetColor(col);
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

void UI_Tutorial_ResumeFromWait(void)
{
	if (!g_IsWaiting) return;
	g_VignetteFadingOut = true;
}

void UI_Tutorial_Start()
{
	if (g_IsTutorial || g_IsPreTutorial) return;

	g_IsTutorial = true;
	g_IsWaiting = false;
	g_CurrentPage = 0;
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

	g_State = TutorialState::FadeIn;
	g_FadeAlpha = 0.0f;
	g_SkipHoldTime = 0.0f;

	ApplyPageHole();
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);
}

void UI_Tutorial_End()
{
	if (g_CameraOverrideActive)
	{
		Camera* cam = GetCamera();
		if (cam) cam->UpdateView(g_SavedCameraPos, g_SavedCameraAt);
		g_CameraOverrideActive = false;
	}
	g_IsCameraTransitioning = false;

	g_IsTutorial = false;
	g_IsPreTutorial = false;
	g_IsWaiting = false;
	g_CurrentPage = 0;
	g_TutorialDelayFrames = TUTORIAL_SKIP_FRAME;

	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
	Mouse_SetVisible(false);
}