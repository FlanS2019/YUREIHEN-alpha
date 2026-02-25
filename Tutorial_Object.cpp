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
#include "light.h"

// ==========================================
// 円盤（enban）Sprite3D
// ==========================================
static Sprite3D* g_pEnban      = nullptr;
static bool      g_EnbanTouched   = false;
static bool      g_EnbanVisible   = false;
static bool      g_PianoPossessed = false;
static bool      g_BustersVisible = false;
static bool      g_BustersStunned = false; // バスターズをスタンさせたフラグ

// =================================================================
// グローバル変数
// =================================================================
static TutorialBusters* g_pTutorialBusters = nullptr;
static TutorialMarker*  g_pTutorialMarker  = nullptr;

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
	m_Visible  = false;

	m_Arrow = new Billboard();
	m_Arrow->Initialize(
		{ m_BasePos.x, m_BasePos.y + TUTORIAL_MARKER_BASE_HEIGHT, m_BasePos.z },
		{ TUTORIAL_MARKER_SIZE, TUTORIAL_MARKER_SIZE },
		{ 0.0f, 0.0f, 0.0f },
		true
	);
	m_Arrow->SetIcon(BILLBOARD_ICON::DESTINATION);
}

void TutorialMarker::Update(void)
{
	if (!m_Arrow || !m_Visible) return;

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
	: AnimSprite3D(pos, scale, rot, pass)
	, Jump(0.01f, 0.2f, PATROL_HEIGHT)
	, m_State(TB_IDLE)
	, m_Icon(nullptr)
	, m_pHeadlight(nullptr)
	, m_MoveSpeed(BUSTERS_MOVE_SPEED_SEARCH)
	, m_WaitTimer(0)
	, m_KeepStateTimer(0)
	, m_HasTarget(false)
	, m_TargetPos(0.0f, 0.0f, 0.0f)
{
	m_Icon = new Billboard();
	m_Icon->Initialize({ 0.0f, 0.0f, 0.0f }, { 0.7f, 0.7f }, { 0.0f, 0.0f, 0.0f }, true);

	m_pHeadlight = new PointLight(
		TRUE,
		XMFLOAT4(pos.x, pos.y + 1.6f, pos.z, 1.0f),
		XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f),
		XMFLOAT4(1.0f, 1.0f, 0.95f, 1.0f),
		10.0f,
		2.0f
	);
}

TutorialBusters::~TutorialBusters()
{
	if (m_Icon)
	{
		delete m_Icon;
		m_Icon = nullptr;
	}
	if (m_pHeadlight)
	{
		delete m_pHeadlight;
		m_pHeadlight = nullptr;
	}
}

// -------------------------------------------------------
// 視野判定（Busters::IsTargetInFOV と同ロジック）
// -------------------------------------------------------
bool TutorialBusters::IsTargetInFOV(const XMFLOAT3& targetPos, float range) const
{
	float dx = targetPos.x - m_Position.x;
	float dz = targetPos.z - m_Position.z;
	float distSq = dx * dx + dz * dz;

	if (distSq > range * range) return false;

	float rotRad = XMConvertToRadians(GetRot().y + 180.0f);
	XMVECTOR forwardVec = XMVectorSet(sinf(rotRad), 0.0f, cosf(rotRad), 0.0f);

	XMVECTOR dirVec = XMVector3Normalize(XMVectorSet(dx, 0.0f, dz, 0.0f));
	float dot = XMVectorGetX(XMVector3Dot(forwardVec, dirVec));
	float limitCos = cosf(XMConvertToRadians(BUSTERS_FOV_ANGLE / 2.0f));

	return dot >= limitCos;
}

// -------------------------------------------------------
// 状態遷移チェック（Busters::CheckState の簡略版）
// -------------------------------------------------------
void TutorialBusters::CheckState(void)
{
	if (m_State == TB_STUN) return;

	Ghost* ghost = GetGhost();
	if (!ghost) return;

	// 変身中・驚かせ中は見つからない
	if (ghost->GetState() == GS_TRANSFORM || ghost->GetState() == GS_SCARE)
	{
		if (m_State == TB_CHASE)
			SetState(TB_SUSPICION);
		return;
	}

	bool hasWall = Field_CheckWallBetween(m_Position, ghost->GetPos());

	float chaseRange     = BUSTERS_PATROL_RANGH;
	float suspicionRange = BUSTERS_SUSPICION_RANGE;
	if (m_State == TB_CHASE)     chaseRange     *= 1.2f;
	if (m_State == TB_SUSPICION) suspicionRange *= 1.2f;

	bool inChase     = !hasWall && IsTargetInFOV(ghost->GetPos(), chaseRange);
	bool inSuspicion = !hasWall && IsTargetInFOV(ghost->GetPos(), suspicionRange);

	if (inChase)
	{
		m_KeepStateTimer = KEEP_STATE_TIME;
		if (m_State != TB_CHASE)
		{
			SetState(TB_CHASE);
			if (m_WaitTimer <= 0)
				m_WaitTimer = WAIT_TIMER_DEFAULT;
		}
	}
	else if (inSuspicion)
	{
		m_KeepStateTimer = KEEP_STATE_TIME;
		if (m_State != TB_SUSPICION)
		{
			SetState(TB_SUSPICION);
			if (m_WaitTimer <= 0)
				m_WaitTimer = WAIT_TIMER_DEFAULT;
		}
	}
	else
	{
		if (m_KeepStateTimer > 0)
		{
			m_KeepStateTimer--;
		}
		else
		{
			// 調査対象がある場合は IDLE に戻さず SUSPICION のまま維持
			if (m_State != TB_IDLE && !m_HasTarget)
			{
				SetState(TB_IDLE);
			}
			else if (m_State == TB_CHASE && m_HasTarget)
			{
				// 追跡を外れたら調査移動に戻す
				SetState(TB_SUSPICION);
			}
		}
	}
}

// -------------------------------------------------------
// 直線移動（壁判定なし・経路探索なし）
// -------------------------------------------------------
void TutorialBusters::MoveTo(const XMFLOAT3& targetPos)
{
	float dx = targetPos.x - m_Position.x;
	float dz = targetPos.z - m_Position.z;
	float len = sqrtf(dx * dx + dz * dz);

	if (len < m_MoveSpeed) return;

	dx /= len;
	dz /= len;

	// 向き更新
	float deg = XMConvertToDegrees(atan2f(dx, dz));
	SetRotY(deg + 180.0f);

	m_Position.x += dx * m_MoveSpeed;
	m_Position.z += dz * m_MoveSpeed;
}

// -------------------------------------------------------
// ヘッドライト位置・向き更新
// -------------------------------------------------------
void TutorialBusters::UpdateHeadlight(void)
{
	if (!m_pHeadlight) return;

	XMFLOAT3 headPos = m_Position;
	headPos.y += 2.0f;

	float rotRad = XMConvertToRadians(GetRot().y);
	float dirX = sinf(rotRad);
	float dirZ = cosf(rotRad);
	headPos.x += dirX * 0.8f;
	headPos.z += dirZ * 0.8f;

	m_pHeadlight->SetPosition(headPos.x, headPos.y, headPos.z);
	m_pHeadlight->SetDirection(XMFLOAT4(dirX, -0.3f, dirZ, 0.0f));

	float range = 15.0f;
	switch (m_State)
	{
	case TB_SUSPICION: range = BUSTERS_SUSPICION_RANGE; break;
	case TB_CHASE:     range = BUSTERS_PATROL_RANGH;    break;
	case TB_STUN:      range = 5.0f;                    break;
	default:           range = 15.0f;                   break;
	}
	m_pHeadlight->SetRange(range);
}

// -------------------------------------------------------
// Update
// -------------------------------------------------------
void TutorialBusters::Update(void)
{
	const float dt = 1.0f / 60.0f;
	this->UpdateAnimation(dt);

	JumpUpdate(*(Transform3D*)this);

	// 硬直タイマー
	if (m_WaitTimer > 0)
	{
		m_WaitTimer--;
		Ghost* ghost = GetGhost();
		if (m_HasTarget && m_State == TB_SUSPICION)
		{
			// 調査中はピアノの方向を向き続ける
			float dx = m_TargetPos.x - m_Position.x;
			float dz = m_TargetPos.z - m_Position.z;
			if (dx * dx + dz * dz > 0.001f)
				SetRotY(XMConvertToDegrees(atan2f(dx, dz)) + 180.0f);
		}
		else if (ghost && (m_State == TB_SUSPICION || m_State == TB_CHASE))
		{
			float dx = ghost->GetPos().x - m_Position.x;
			float dz = ghost->GetPos().z - m_Position.z;
			SetRotY(XMConvertToDegrees(atan2f(dx, dz)) + 180.0f);
		}
	}
	else
	{
		// 状態チェック（ゴースト検知）
		CheckState();

		Ghost* ghost = GetGhost();

		if (m_State == TB_CHASE || m_State == TB_SUSPICION)
		{
			if (m_HasTarget && m_State == TB_SUSPICION)
			{
				// 調査対象（ピアノ）へ向かっているとき
				float dx = m_TargetPos.x - m_Position.x;
				float dz = m_TargetPos.z - m_Position.z;
				float distSq = dx * dx + dz * dz;

				if (distSq <= 2.0f * 2.0f)
				{
					// ピアノ前に到着 → 調査開始（立ち止まって CHECK アイコン）
					m_WaitTimer = 180;
					if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::CHECK);
					// ターゲット方向を向く
					if (distSq > 0.001f)
					{
						SetRotY(XMConvertToDegrees(atan2f(dx, dz)) + 180.0f);
					}
				}
				else
				{
					// まだ遠い → 近づく
					m_MoveSpeed = BUSTERS_MOVE_SPEED_SUSPICION;
					MoveTo(m_TargetPos);
				}
			}
			else if (ghost)
			{
				// ゴーストを発見している → ゴーストへ直線移動
				m_MoveSpeed = (m_State == TB_CHASE)
					? BUSTERS_MOVE_SPEED_CHASE
					: BUSTERS_MOVE_SPEED_SUSPICION;
				MoveTo(ghost->GetPos());
			}
		}
		else if (m_HasTarget && m_State == TB_IDLE)
		{
			// 調査対象（ピアノ）へ警戒速度で向かう
			m_MoveSpeed = BUSTERS_MOVE_SPEED_SUSPICION;
			SetState(TB_SUSPICION);
			MoveTo(m_TargetPos);
		}
	}

	// ヘッドライト更新
	UpdateHeadlight();

	// アイコン更新
	if (m_Icon)
	{
		switch (m_State)
		{
		case TB_IDLE:      m_Icon->SetIcon(BILLBOARD_ICON::NONE);     break;
		case TB_SUSPICION: m_Icon->SetIcon(BILLBOARD_ICON::QUESTION); break;
		case TB_CHASE:     m_Icon->SetIcon(BILLBOARD_ICON::ALERT);    break;
		case TB_STUN:      m_Icon->SetIcon(BILLBOARD_ICON::STUN);     break;
		}

		XMFLOAT3 iconPos = m_Position;
		iconPos.y += 3.25f;
		m_Icon->SetPos(iconPos);
		m_Icon->Update();
	}
}

void TutorialBusters::Draw(void)
{
	AnimSprite3D::Draw();

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

// 驚かせられた
void TutorialBusters::OnScared(void)
{
	JumpStart();
	SetState(TB_STUN);
	m_WaitTimer      = 180; // 3秒間スタン
	m_KeepStateTimer = 0;
}

// =================================================================
// グローバル関数
// =================================================================

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
	g_BustersStunned = false; // 追加：バスターズスタンフラグのリセット

	if (g_pTutorialBusters)
	{
		delete g_pTutorialBusters;
		g_pTutorialBusters = nullptr;
	}

	g_pTutorialBusters = new TutorialBusters(
		{ 0.0f, PATROL_HEIGHT, 0.0f },
		{ 0.12f, 0.12f, 0.12f },
		{ 0.0f, 180.0f, 0.0f },
		"asset\\model\\busters_v3.fbx"
	);

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

	// バスターズがスタンしたかチェック
	if (!g_BustersStunned && g_pTutorialBusters)
	{
		if (g_pTutorialBusters->GetState() == TB_STUN)
		{
			g_BustersStunned = true;
		}
	}
}

void TutorialObject_Draw(void)
{
	if (Field_GetCurrentFloor() != 2) return;

	if (g_pTutorialBusters && g_BustersVisible)
	{
		g_pTutorialBusters->Draw();
	}

	if (g_pTutorialMarker)
	{
		g_pTutorialMarker->Draw();
	}

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
	g_BustersVisible = false;
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

void TutorialObject_SetBustersVisible(bool visible)
{
	g_BustersVisible = visible;
}

bool* TutorialObject_GetPianoPossessedPtr(void)
{
	return &g_PianoPossessed;
}

bool* TutorialObject_GetBustersStunnedPtr(void)
{
	return &g_BustersStunned;
}

FlagWithDelay TutorialObject_GetBustersStunnedPtr(int delayFrames)
{
	return { &g_BustersStunned, delayFrames };
}

TutorialBusters* GetTutorialBusters(void)
{
	return g_pTutorialBusters;
}

TutorialMarker* GetTutorialMarker(void)
{
	return g_pTutorialMarker;
}

void TutorialBusters_SetLight(void)
{
	if (!g_pTutorialBusters || !g_BustersVisible) return;

	PointLight* pLight = g_pTutorialBusters->GetHeadlight();
	if (pLight)
	{
		Shader_AddPointLight(pLight);
	}
}
