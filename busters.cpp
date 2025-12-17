#include "busters.h"
#include "Camera.h"
#include "shader.h"
#include "keyboard.h"
#include "sprite3d.h"
#include "debug_ostream.h"
#include "define.h"
#include "field.h"
#include "furniture.h"
#include <stdlib.h>

#include "UI.h"     // UI操作用
#include "scene.h"  // SCENE定数用
#include "fade.h"   // StartFade用
#include "ghost.h"  // Ghost操作用

static Busters* g_BustersList[MAP_FLOORS];

// =================================================================
// Busters クラスメンバ関数の実装
// =================================================================

Busters::Busters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
	: Sprite3D(pos, scale, rot, pass),
	Jump(0.01f, 0.2f, PATROL_HEIGHT),
	m_State(BUSTERS_SEARCH),
	m_TargetFurnitureIndex(-1),
	m_WaitTimer(0),
	m_Velocity(0.0f, 0.0f, 0.0f),
	m_MoveSpeed(0.03f),
	m_DistanceToGhost(0.0f)
{
	srand((unsigned int)GetTickCount64());
}

void Busters::Update(void)
{
	JumpUpdate(*(Transform3D*)this);

	if (m_WaitTimer > 0)
	{
		m_WaitTimer--;
		return;
	}

	CheckState();

	XMFLOAT3 nextStepPos = m_Position;

	switch (m_State)
	{
	case BUSTERS_SEARCH: // 探索
		if (m_TargetFurnitureIndex == -1)
		{
			m_TargetFurnitureIndex = rand() % FURNITURE_NUM;
			Furniture* targetFurniture = GetFurniture(m_TargetFurnitureIndex);
			if (targetFurniture)
			{
				m_PathList = Field_FindPath(m_Position, targetFurniture->GetPos());
				if (m_PathList.empty()) m_TargetFurnitureIndex = -1;
			}
			else
			{
				m_TargetFurnitureIndex = -1;
			}
		}

		if (!m_PathList.empty())
		{
			XMFLOAT3 targetNode = m_PathList.back();
			targetNode.y = m_Position.y;
			nextStepPos = targetNode;
			XMVECTOR myPosV = XMLoadFloat3(&m_Position);
			XMVECTOR targetV = XMLoadFloat3(&targetNode);
			if (XMVectorGetX(XMVector3Length(XMVectorSubtract(targetV, myPosV))) < 0.5f)
			{
				m_PathList.pop_back();
			}
		}
		else if (m_TargetFurnitureIndex != -1)
		{
			m_TargetFurnitureIndex = -1;
			m_WaitTimer = 60;
		}

		m_MoveSpeed = 0.03f;
		break;

	case BUSTERS_SUSPICION: // 警戒
		m_PathList.clear();
		m_TargetFurnitureIndex = -1;
		if (GetGhost())
		{
			nextStepPos = GetGhost()->GetPos();
			nextStepPos.y = m_Position.y;
		}
		m_MoveSpeed = 0.06f;
		break;

	case BUSTERS_CHASE: // 追跡
		if (GetGhost())
		{
			nextStepPos = GetGhost()->GetPos();
			nextStepPos.y = m_Position.y;
		}
		m_PathList.clear();
		m_TargetFurnitureIndex = -1;
		m_MoveSpeed = 0.09f;
		break;
	}

	MoveTo(nextStepPos);
}

void Busters::CheckState(void)
{
	Ghost* ghost = GetGhost();
	if (!ghost) return;
	if (m_WaitTimer > 0) return;

	XMFLOAT3 ghostPos = ghost->GetPos();
	XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
	XMVECTOR myVec = XMLoadFloat3(&m_Position);
	m_DistanceToGhost = XMVectorGetX(XMVector3Length(XMVectorSubtract(ghostVec, myVec)));

	// 変身中
	if (ghost->GetState() == GS_TRANSFORM || ghost->GetState() == GS_SCARE)
	{
		if (m_State == BUSTERS_SUSPICION && m_DistanceToGhost < 2.0f)
		{
			m_State = BUSTERS_SEARCH;
			this->ResetColor();
			m_WaitTimer = 60;
		}
		else if (m_State != BUSTERS_SEARCH && m_State != BUSTERS_SUSPICION)
		{
			m_State = BUSTERS_SEARCH;
			this->ResetColor();
			ghost->SetIsDetectedByBuster(false);
			m_WaitTimer = 60;
		}
		return;
	}

	bool hasWall = Field_CheckWallBetween(m_Position, ghostPos);

	if (!hasWall && m_DistanceToGhost < BUSTERS_PATROL_RANGH)
	{
		if (m_State != BUSTERS_CHASE)
		{
			m_State = BUSTERS_CHASE;
			this->SetColor(1.0f, 0.0f, 0.0f, 1.0f); // 赤
			ghost->SetIsDetectedByBuster(true);
		}
	}
	else if (!hasWall && m_DistanceToGhost < BUSTERS_SUSPICION_RANGE)
	{
		if (m_State != BUSTERS_SUSPICION)
		{
			m_State = BUSTERS_SUSPICION;
			this->SetColor(1.0f, 1.0f, 0.0f, 1.0f); // 黄
			ghost->SetIsDetectedByBuster(false);
		}
	}
	else
	{
		if (m_State != BUSTERS_SEARCH)
		{
			m_State = BUSTERS_SEARCH;
			this->ResetColor();
			ghost->SetIsDetectedByBuster(false);
			m_TargetFurnitureIndex = -1;
			m_WaitTimer = 30;
		}
	}
}

void Busters::MoveTo(XMFLOAT3 targetPos)
{
	if (GetIsJumping()) return;

	float dx = targetPos.x - m_Position.x;
	float dz = targetPos.z - m_Position.z;

	if (fabsf(dx) < 0.1f && fabsf(dz) < 0.1f) return;

	float len = sqrtf(dx * dx + dz * dz);
	if (len > 0)
	{
		dx /= len;
		dz /= len;
	}

	float angle = atan2f(dx, dz);
	float deg = XMConvertToDegrees(angle);
	SetRotY(deg + 180.0f);

	float r = 0.4f;
	float nextX = m_Position.x + dx * m_MoveSpeed;
	bool hitX = false;
	if (Field_IsWall(nextX + r, m_Position.y, m_Position.z + r) ||
		Field_IsWall(nextX + r, m_Position.y, m_Position.z - r) ||
		Field_IsWall(nextX - r, m_Position.y, m_Position.z + r) ||
		Field_IsWall(nextX - r, m_Position.y, m_Position.z - r))
	{
		hitX = true;
	}
	if (!hitX) m_Position.x = nextX;

	float nextZ = m_Position.z + dz * m_MoveSpeed;
	bool hitZ = false;
	if (Field_IsWall(m_Position.x + r, m_Position.y, nextZ + r) ||
		Field_IsWall(m_Position.x + r, m_Position.y, nextZ - r) ||
		Field_IsWall(m_Position.x - r, m_Position.y, nextZ + r) ||
		Field_IsWall(m_Position.x - r, m_Position.y, nextZ - r))
	{
		hitZ = true;
	}
	if (!hitZ) m_Position.z = nextZ;
}

void Busters::OnScared(void)
{
	JumpStart();
	m_TargetFurnitureIndex = -1;
	m_WaitTimer = 120;
	this->SetColor(0.0f, 0.0f, 1.0f, 1.0f); // 青
}

void Busters::OnLured(XMFLOAT3 targetPos)
{
	m_State = BUSTERS_SUSPICION;
	this->SetColor(0.0f, 1.0f, 1.0f, 1.0f); // シアン
	m_WaitTimer = 60;
	m_PathList.clear();
}

void Busters::OnStopped(void)
{
	m_WaitTimer = 300;
	this->SetColor(0.5f, 0.0f, 0.5f, 1.0f); // 紫
}

void Busters::SetIsGhostDiscover(bool discover)
{
	if (m_WaitTimer > 0) return;
	if (discover) this->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
	else this->ResetColor();
}

// =================================================================
// グローバル関数
// =================================================================

void Busters_Initialize(void)
{
	g_BustersList[0] = new Busters({ 0.0f, PATROL_HEIGHT, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\buster.fbx");
	if (g_BustersList[0]) g_BustersList[0]->SetGroundLevel(PATROL_HEIGHT);

	g_BustersList[1] = new Busters({ -10.0f, PATROL_HEIGHT, 10.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\buster.fbx");
	if (g_BustersList[1]) g_BustersList[1]->SetGroundLevel(PATROL_HEIGHT);

	g_BustersList[2] = new Busters({ 10.0f, PATROL_HEIGHT, -10.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\buster.fbx");
	if (g_BustersList[2]) g_BustersList[2]->SetGroundLevel(PATROL_HEIGHT);
}

void Busters_Update(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS && g_BustersList[currentFloor])
		g_BustersList[currentFloor]->Update();
}

void Busters_Draw(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS && g_BustersList[currentFloor])
		g_BustersList[currentFloor]->Draw();
}

void Busters_Finalize(void)
{
	for (int i = 0; i < MAP_FLOORS; i++)
	{
		if (g_BustersList[i]) { delete g_BustersList[i]; g_BustersList[i] = NULL; }
	}
}

Busters* GetBusters(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS) return g_BustersList[currentFloor];
	return NULL;
}

void BustersScare(void)
{
	Busters* target = GetBusters();
	if (target) target->OnScared();
}

void BustersLured(XMFLOAT3 pos)
{
	Busters* target = GetBusters();
	if (target) target->OnLured(pos);
}

void BustersStopped(void)
{
	Busters* target = GetBusters();
	if (target) target->OnStopped();
}

// =================================================================
// ゲージMAX時の処理 (修正版: デバッグ削除 & 敗北回避)
// =================================================================
void Busters_CheckGaugeEvent(void)
{
	if (!UI_IsScareGaugeMax()) return;

	int currentFloor = Field_GetCurrentFloor();

	if (currentFloor > 0)
	{
		// -------------------------------------------------
		// 2階以上の場合 -> 下の階へ逃げる
		// -------------------------------------------------

		// 1. ゲージをリセット (通常は0になります)
		UI_ResetScareGauge();

		AddScareGauge(1.0f);

		// 2. 下の階へ移動
		Field_ChangeFloor(currentFloor - 1);

		//// 3. プレイヤー(Ghost)も追って移動
		//if (GetGhost())
		//{
		//	GetGhost()->ResetPos();
		//	GetGhost()->SetPos({ 0.0f, Ghost::GetGhostPosY(), 0.0f });
		//}
	}
	else
	{
		// -------------------------------------------------
		// 1階の場合 -> 逃げ場なし（プレイヤーの勝利）
		// -------------------------------------------------
		StartFade(SCENE_ANM_WIN);
	}
}