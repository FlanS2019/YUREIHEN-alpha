#pragma execution_character_set("utf-8")
#include <cmath>
#include <DirectXMath.h>
using namespace DirectX;
#include "Tutorial_Object.h"
#include "sprite3d.h"
#include "field.h"
#include "ghost.h"
#include "UI_Tutorial.h"
#include "furniture.h"
#include "shader.h"
#include "define.h"
#include "keyboard.h"

// ==========================================
// 円盤（enban）Sprite3D
// ==========================================
static Sprite3D* g_pEnban      = nullptr;
static bool      g_EnbanTouched   = false;
static bool      g_EnbanVisible   = false; // 円盤の表示フラグ（コールバックで制御）
static bool      g_PianoPossessed = false;

// =================================================================
// グローバル変数
// =================================================================
static TutorialBusters* g_pTutorialBusters = nullptr;
static TutorialMarker*  g_pTutorialMarker  = nullptr;

// =================================================================
// TutorialMarker 定数定義
// =================================================================
// define.h のマクロを使用（TUTORIAL_MARKER_SIZE / BOB_AMP / BOB_SPEED / BASE_HEIGHT）

// =================================================================
// TutorialMarker クラスメンバ関数の実装
// =================================================================

TutorialMarker::TutorialMarker()
	: m_BasePos(0.0f, 0.0f, 0.0f)
	, m_Arrow(nullptr)
	, m_BobTimer(0.0f)
	, m_Visible(true)
{
}

TutorialMarker::~TutorialMarker()
{
	if (m_Arrow)
	{
		delete m_Arrow;
		m_Arrow = nullptr;
	}
}

void TutorialMarker::Initialize(const XMFLOAT3& pos)
{
	m_BasePos  = pos;
	m_BobTimer = 0.0f;
	m_Visible  = false; // コールバックで表示する

	m_Arrow = new Billboard();
	m_Arrow->Initialize(
		{ m_BasePos.x, m_BasePos.y + TUTORIAL_MARKER_BASE_HEIGHT, m_BasePos.z },
		{ TUTORIAL_MARKER_SIZE, TUTORIAL_MARKER_SIZE },
		{ 0.0f, 0.0f, 0.0f },
		true   // 両面描画
	);
	m_Arrow->SetIcon(BILLBOARD_ICON::DESTINATION);
}

void TutorialMarker::Update(void)
{
	if (!m_Arrow || !m_Visible) return;

	// ぴょんぴょんアニメーション（サイン波で上下）
	const float dt = 1.0f / 60.0f;
	m_BobTimer += TUTORIAL_MARKER_BOB_SPEED * dt;

	float offsetY = sinf(m_BobTimer) * TUTORIAL_MARKER_BOB_AMP;

	XMFLOAT3 arrowPos = {
		m_BasePos.x,
		m_BasePos.y + TUTORIAL_MARKER_BASE_HEIGHT + offsetY,
		m_BasePos.z
	};
	m_Arrow->SetPos(arrowPos);
	m_Arrow->Update();
}

void TutorialMarker::Draw(void)
{
	if (!m_Arrow || !m_Visible) return;

	Shader_Begin();
	m_Arrow->Draw();
}

void TutorialMarker::SetPos(const XMFLOAT3& pos)
{
	m_BasePos = pos;
}

// =================================================================
// TutorialBusters クラスメンバ関数の実装
// =================================================================

TutorialBusters::TutorialBusters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
	: AnimSprite3D(pos, scale, rot, pass),
	m_State(TB_IDLE),
	m_Icon(nullptr)
{
	m_Icon = new Billboard();
	m_Icon->Initialize({ 0.0f, 0.0f, 0.0f }, { 0.7f, 0.7f }, { 0.0f, 0.0f, 0.0f }, true);
}

TutorialBusters::~TutorialBusters()
{
	if (m_Icon)
	{
		delete m_Icon;
		m_Icon = nullptr;
	}
}

void TutorialBusters::Update(void)
{
	// アニメーション更新
	const float dt = 1.0f / 60.0f;
	this->UpdateAnimation(dt);

	// 数字キーでアニメーション再生
	for (unsigned int i = 0; i < 10; ++i)
	{
		if (Keyboard_IsKeyDownTrigger((Keyboard_Keys)(KK_D0 + i)))
		{
			if (i < GetAnimationCount())
			{
				PlayAnimationByIndex(i, true);
				hal::dout << "Playing animation: " << GetAnimationName(i) << std::endl;
			}
		}
	}

	// アイコンの位置を頭上に合わせる
	if (m_Icon)
	{
		XMFLOAT3 iconPos = m_Position;
		iconPos.y += 3.25f;
		m_Icon->SetPos(iconPos);
		m_Icon->Update();
	}
}

void TutorialBusters::Draw(void)
{
	// バスターズ本体を描画
	AnimSprite3D::Draw();

	// ビルボードアイコンを描画（通常シェーダーに戻す）
	if (m_Icon)
	{
		Shader_Begin();
		m_Icon->Draw();
	}
}

void TutorialBusters::SetState(TUTORIAL_BUSTERS_STATE state)
{
	m_State = state;

	switch (m_State)
	{
	case TB_IDLE:
		this->ResetColor();
		if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::NONE);
		break;

	case TB_SUSPICION:
		this->SetColor(1.0f, 1.0f, 0.0f, 1.0f); // 黄
		if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::QUESTION);
		break;

	case TB_CHASE:
		this->SetColor(1.0f, 0.0f, 0.0f, 1.0f); // 赤
		if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::ALERT);
		break;

	case TB_STUN:
		this->SetColor(0.0f, 0.0f, 1.0f, 1.0f); // 青
		if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::STUN);
		break;
	}
}

void TutorialObject_Initialize(void)
{
	g_pEnban = new Sprite3D(
		{ -5.0f, 0.5f, 17.0f },
		{ 4.0f, 1.0f, 4.0f },
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\enban.fbx"
	);

	g_EnbanTouched   = false;
	g_PianoPossessed = false;

	if (g_pTutorialBusters)
	{
		delete g_pTutorialBusters;
		g_pTutorialBusters = nullptr;
	}

	g_pTutorialBusters = new TutorialBusters(
		{ 0.0f, PATROL_HEIGHT, 0.0f },
		{ 0.15f, 0.15f, 0.15f },
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\bustars_nocolor.fbx"
	);

	// 目的地マーカーを円盤の上に配置
	if (g_pTutorialMarker)
	{
		delete g_pTutorialMarker;
		g_pTutorialMarker = nullptr;
	}
	g_pTutorialMarker = new TutorialMarker();
	g_pTutorialMarker->Initialize({ -5.0f, 0.5f, 17.0f });
}

void TutorialObject_Update(void)
{
	if (g_pTutorialBusters)
	{
		g_pTutorialBusters->Update();
	}

	// マーカーは表示中のみ Update
	if (g_pTutorialMarker)
	{
		g_pTutorialMarker->Update();
	}

	if (!UI_Tutorial_IsWaiting()) return;

	Ghost* pGhost = GetGhost();
	if (!pGhost) return;

	if (g_pEnban && !g_EnbanTouched)
	{
		XMFLOAT3 gPos = pGhost->GetPos();
		XMFLOAT3 ePos = g_pEnban->GetPos();
		float dx = gPos.x - ePos.x;
		float dz = gPos.z - ePos.z;

		float enbanRadius = g_pEnban->GetScale().x * 0.2f;
		if (sqrtf(dx * dx + dz * dz) <= enbanRadius)
		{
			g_EnbanTouched = true;
		}
	}

	if (!g_PianoPossessed)
	{
		if (pGhost->GetState() == GS_TRANSFORM)
		{
			int inRangeNum = pGhost->GetInRangeNum();
			Furniture* pFurniture = GetFurniture(inRangeNum);
			if (pFurniture && pFurniture->GetBlockID() == 62)
			{
				g_PianoPossessed = true;
			}
		}
	}
}

void TutorialObject_Draw(void)
{
	if (Field_GetCurrentFloor() != 2) return;

	if (g_pTutorialBusters)
	{
		g_pTutorialBusters->Draw();
	}

	// 目的地マーカー描画
	if (g_pTutorialMarker)
	{
		g_pTutorialMarker->Draw();
	}

	// 円盤は表示フラグが立っているときだけ描画
	if (g_pEnban && g_EnbanVisible)
	{
		g_pEnban->Draw();
	}
}

void TutorialObject_Finalize(void)
{
	delete g_pEnban;
	g_pEnban         = nullptr;
	g_EnbanTouched   = false;
	g_EnbanVisible   = false;
	g_PianoPossessed = false;

	if (g_pTutorialBusters)
	{
		delete g_pTutorialBusters;
		g_pTutorialBusters = nullptr;
	}

	if (g_pTutorialMarker)
	{
		delete g_pTutorialMarker;
		g_pTutorialMarker = nullptr;
	}
}

bool* TutorialObject_GetEnbanTouchedPtr(void)
{
	return &g_EnbanTouched;
}

void TutorialObject_SetEnbanVisible(bool visible)
{
	g_EnbanVisible = visible;
}

bool* TutorialObject_GetPianoPossessedPtr(void)
{
	return &g_PianoPossessed;
}

TutorialBusters* GetTutorialBusters(void)
{
	return g_pTutorialBusters;
}

TutorialMarker* GetTutorialMarker(void)
{
	return g_pTutorialMarker;
}
