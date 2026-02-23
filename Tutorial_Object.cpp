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
static Sprite3D* g_pEnban = nullptr;
static bool      g_EnbanTouched = false;
static bool      g_PianoPossessed = false;

// =================================================================
// グローバル変数
// =================================================================
static TutorialBusters* g_pTutorialBusters = nullptr;

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

	g_EnbanTouched = false;
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
}

void TutorialObject_Update(void)
{



	if (g_pTutorialBusters)
	{
		g_pTutorialBusters->Update();
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

		if (sqrtf(dx * dx + dz * dz) <= 0.8f)
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
			if (pFurniture && pFurniture->GetBlockID() == 62) // 62 is Piano
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

	if (!g_pEnban) return;
	g_pEnban->Draw();
}

void TutorialObject_Finalize(void)
{
	delete g_pEnban;
	g_pEnban = nullptr;
	g_EnbanTouched = false;
	g_PianoPossessed = false;

	if (g_pTutorialBusters)
	{
		delete g_pTutorialBusters;
		g_pTutorialBusters = nullptr;
	}
}

bool* TutorialObject_GetEnbanTouchedPtr(void)
{
	return &g_EnbanTouched;
}

bool* TutorialObject_GetPianoPossessedPtr(void)
{
	return &g_PianoPossessed;
}

TutorialBusters* GetTutorialBusters(void)
{
	return g_pTutorialBusters;
}
