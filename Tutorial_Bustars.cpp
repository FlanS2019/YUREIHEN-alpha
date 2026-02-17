#include "Tutorial_Bustars.h"
#include "billboard.h"
#include "shader.h"
#include "define.h"

// =================================================================
// グローバル変数
// =================================================================
static TutorialBusters* g_pTutorialBusters = nullptr;

// =================================================================
// TutorialBusters クラスメンバ関数の実装
// =================================================================

TutorialBusters::TutorialBusters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
	: Sprite3D(pos, scale, rot, pass),
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
	Sprite3D::Draw();

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

// =================================================================
// グローバル関数
// =================================================================

void TutorialBusters_Initialize(const XMFLOAT3& pos)
{
	// 既存のインスタンスがあれば解放
	if (g_pTutorialBusters)
	{
		delete g_pTutorialBusters;
		g_pTutorialBusters = nullptr;
	}

	g_pTutorialBusters = new TutorialBusters(
		pos,
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\Busters_karikansei_3.fbx"
	);
}

void TutorialBusters_Update(void)
{
	if (g_pTutorialBusters)
	{
		g_pTutorialBusters->Update();
	}
}

void TutorialBusters_Draw(void)
{
	if (g_pTutorialBusters)
	{
		g_pTutorialBusters->Draw();
	}
}

void TutorialBusters_Finalize(void)
{
	if (g_pTutorialBusters)
	{
		delete g_pTutorialBusters;
		g_pTutorialBusters = nullptr;
	}
}

TutorialBusters* GetTutorialBusters(void)
{
	return g_pTutorialBusters;
}
