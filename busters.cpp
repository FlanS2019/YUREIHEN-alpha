#include "busters.h"
#include "billboard.h"
#include "Camera.h"
#include "debug_ostream.h"
#include "define.h"
#include "fade.h"
#include "ghost.h"
#include "shader.h"
#include "keyboard.h"
#include "sprite3d.h"

#include "field.h"
#include "furniture.h"
#include <stdlib.h>

#include "UI.h"
#include "scene.h"



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
	m_DistanceToGhost(0.0f),
	m_Icon(nullptr)
{
	m_Icon = new Billboard();

	m_Icon->Initialize({ 0.0f, 0.0f, 0.0f }, { 0.7f, 0.7f }, { 0.0f, 0.0f, 0.0f }, true);
}

Busters::~Busters()
{
	if (m_Icon)
	{
		delete m_Icon;
		m_Icon = nullptr;
	}
}

void Busters::Update(void)
{
	JumpUpdate(*(Transform3D*)this);

	// アイコンの状態更新
	if (m_Icon)
	{
		// 状態に合わせてアイコンを一発切り替え
		switch (m_State)
		{
		case BUSTERS_SEARCH:    // 探索中
			m_Icon->SetIcon(BILLBOARD_ICON::NONE); // 何も出さない
			break;

		case BUSTERS_SUSPICION: // 警戒中（？）
			m_Icon->SetIcon(BILLBOARD_ICON::QUESTION);
			break;

		case BUSTERS_CHASE:     // 追跡中（！）
			m_Icon->SetIcon(BILLBOARD_ICON::ALERT);
			break;

		case BUSTERS_STUN:      // ★追加: 気絶中ならSTUNアイコン
			m_Icon->SetIcon(BILLBOARD_ICON::STUN);
			break;
		}

		// 位置合わせ（頭上）
		XMFLOAT3 iconPos = m_Position;

		iconPos.y += 3.25f; 
		
		m_Icon->SetPos(iconPos);
		m_Icon->Update();
	}

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
				if (m_PathList.empty())
				{
					m_TargetFurnitureIndex = -1;
					m_WaitTimer = 60; // 経路が見つからない場合は1秒間検索を控える
				}
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
			m_WaitTimer = 300;		//家具の調査時間(60f=1秒)
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

	auto checkWallCollision = [&](float nx, float nz) -> bool {
		float r = 0.4f; // 当たり判定半径
		return (Field_IsWall(nx + r, m_Position.y, nz + r) ||
			Field_IsWall(nx + r, m_Position.y, nz - r) ||
			Field_IsWall(nx - r, m_Position.y, nz + r) ||
			Field_IsWall(nx - r, m_Position.y, nz - r));
		};

	// --- X軸移動 ---
	float nextX = m_Position.x + dx * m_MoveSpeed;
	if (!checkWallCollision(nextX, m_Position.z)) // 関数を呼ぶだけ！
	{
		m_Position.x = nextX;
	}

	// --- Z軸移動 ---
	float nextZ = m_Position.z + dz * m_MoveSpeed;
	if (!checkWallCollision(m_Position.x, nextZ)) // 同じ関数を再利用！
	{
		m_Position.z = nextZ;
	}
}

void Busters::OnScared(void)
{
	JumpStart();
	m_TargetFurnitureIndex = -1;
	m_WaitTimer = 120;
	this->SetColor(0.0f, 0.0f, 1.0f, 1.0f); // 青

	m_State = BUSTERS_STUN;
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

	m_State = BUSTERS_STUN;
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
	g_BustersList[0] = new Busters({ 0.0f, PATROL_HEIGHT, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\Busters_karikansei_3.fbx");
	if (g_BustersList[0]) g_BustersList[0]->SetGroundLevel(PATROL_HEIGHT);

	g_BustersList[1] = new Busters({ -10.0f, PATROL_HEIGHT, 10.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\Busters_karikansei_3.fbx");
	if (g_BustersList[1]) g_BustersList[1]->SetGroundLevel(PATROL_HEIGHT);

	g_BustersList[2] = new Busters({ 10.0f, PATROL_HEIGHT, -10.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\Busters_karikansei_3.fbx");
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
// ゲージMAX時の処理
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

		// 1. ゲージをリセット
		UI_ResetScareGauge();

		//0.0以下で敗北になるのでとりあえず1を足す
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

void Busters::Draw(void)
{
	// 1. バスターズ（ボーンあり）を描画

	Sprite3D::Draw();

	if (m_Icon)
	{
		// 2. ここで通常のシェーダー（ボーン無し）に戻す
		Shader_Begin();

		// 3. ビルボード描画
		m_Icon->Draw();
	}
}