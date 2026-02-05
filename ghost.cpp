#include "ghost.h"
using namespace DirectX;
#include "sprite.h"
#include "sprite3d.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "field.h"
#include "mouse.h"
#include "debug_ostream.h"
#include "camera.h"
#include "furniture.h"
#include "busters.h"
#include "UI.h"
#include "UI_scarecombo.h"
#include "define.h"
#include "sound.h"
#include <algorithm>
#include "shader.h"

Ghost* g_Ghost = NULL;
SoundData* g_pScareSound = nullptr;

// 現在の驚かし範囲（半径）を計算する関数
static float GetCurrentScareRange()
{
	int combo = UI_ScareCombo_GetNumber();
	float range = SCARE_RANGE * (float)combo / 5.0f;
	if (range < 3.5f) range = 3.5f;
	return range;
}

// 円のサイズと位置を更新するヘルパー関数
static void UpdateRangeCircleState()
{
	if (!g_Ghost || !g_Ghost->m_pRangeCircle) return;

	// コンボ数に合わせてサイズ（スケール）を更新
	float currentRange = GetCurrentScareRange();
	// 円モデルの直径 = 半径 * 2
	g_Ghost->m_pRangeCircle->SetSize({ currentRange * 2.0f, 0.1f, currentRange * 2.0f });

	// 位置を家具に合わせる
	Furniture* pFurniture = GetFurniture(g_Ghost->GetInRangeNum());
	if (pFurniture)
	{
		XMFLOAT3 circlePos = pFurniture->GetPos();
		float groundY = pFurniture->GetGroundLevel();
		circlePos.y = groundY + 0.01f;
		g_Ghost->m_pRangeCircle->SetPos(circlePos);
	}
}

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_Ghost = new Ghost(
		{ -3.0f, Ghost::GetGhostPosY(), -10.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 180.0f, 0.0f },
		"asset\\model\\ghost.fbx"
	);

	// 検出範囲を表示する円を初期化
	if (g_Ghost)
	{
		g_Ghost->m_pLight = new PointLight(
			TRUE,
			XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f),		// 位置 (Point Light)
			XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f),		// 方向 (Dummy)
			XMFLOAT4(1.0f, 1.0f, 0.9f, 1.0f),		// 拡散光
			6.0f,									// 減衰距離 (Range)
			0.8f									// 強度 (Intensity)
		);

		XMFLOAT3 circlePos = g_Ghost->GetPos();
		circlePos.y = 0.01f;

		// 初期サイズ
		float initRange = 2.0f;

		g_Ghost->m_pRangeCircle = new Sprite3D(
			circlePos,
			{ initRange * 2.0f, 0.1f, initRange * 2.0f },
			{ 0.0f, 0.0f, 0.0f },
			"asset\\model\\circle.fbx"
		);

		if (g_Ghost->m_pRangeCircle)
		{
			g_Ghost->m_pRangeCircle->SetColor(0.0f, 1.0f, 0.0f, 0.5f);
		}
	}
}

void Ghost_Update(void)
{
	if (!g_Ghost) return;

	switch (g_Ghost->GetState())
	{
	case GS_MOVING:
		g_Ghost->SetIsDraw(true);
		g_Ghost->Move();
		g_Ghost->FurnitureSearch();
		g_Ghost->FloorMove();
		break;

	case GS_FURNITURE_FOUND:
		g_Ghost->SetIsDraw(true);
		g_Ghost->Move();
		g_Ghost->FurnitureSearch();
		g_Ghost->FloorMove();

		if (Keyboard_IsKeyDownTrigger(KK_SPACE))
		{

			UpdateRangeCircleState();

			g_Ghost->SetState(GS_TRANSFORM);
			g_Ghost->m_HasIncreasedMultiplier = false;
		}
		break;

	case GS_TRANSFORM:
		g_Ghost->SetIsDraw(false);
		g_Ghost->Transforming();

		if (g_Ghost->m_pRangeCircle)
		{
			Furniture* pFurniture = GetFurniture(g_Ghost->GetInRangeNum());
			if (pFurniture)
			{
				XMFLOAT3 circlePos = pFurniture->GetPos();
				float groundY = pFurniture->GetGroundLevel();
				circlePos.y = groundY + 0.01f;
				g_Ghost->m_pRangeCircle->SetPos(circlePos);
			}
		}

		if (Keyboard_IsKeyDownTrigger(KK_SPACE))
		{
			g_Ghost->SetState(GS_SCARE);
			g_Ghost->ScareStart();
		}

		if (Keyboard_IsKeyDownTrigger(KK_E))
		{

			UpdateRangeCircleState();

			g_Ghost->ResetPos();
			g_Ghost->SetState(GS_MOVING);
		}
		break;

	case GS_SCARE:
		g_Ghost->SetIsDraw(false);
		g_Ghost->Transforming();

		// 家具のジャンプが終わったら移動状態に戻る
		if (FurnitureScareEnded(g_Ghost->GetInRangeNum()))
		{
			UpdateRangeCircleState();

			g_Ghost->ResetPos();
			g_Ghost->SetState(GS_MOVING);
		}
		break;

	default:
		break;
	}

	// P キーでデバッグ出力
	if (Keyboard_IsKeyDownTrigger(KK_P))
	{
		if (g_Ghost)
		{
			XMFLOAT3 pos = g_Ghost->GetPos();
			hal::dout << "Ghost Position: X=" << pos.x << ", Y=" << pos.y << ", Z=" << pos.z << std::endl;
		}
	}

	if (g_Ghost)
	{
		Camera_SetTargetPos(g_Ghost->GetPos());
	}
}

void Ghost_Draw(void)
{
	if (g_Ghost)
	{
		g_Ghost->Draw();
	}

	if (g_Ghost && g_Ghost->m_pRangeCircle &&
		(g_Ghost->GetState() == GS_TRANSFORM || g_Ghost->GetState() == GS_SCARE))
	{
		g_Ghost->m_pRangeCircle->Draw();
	}
}

void Ghost_Finalize(void)
{
	if (g_pScareSound)
	{
		UnloadSound(g_pScareSound);
		g_pScareSound = nullptr;
	}

	if (g_Ghost)
	{
		delete g_Ghost;
		g_Ghost = NULL;
	}
}

void Ghost_SetLight(void)
{
	if (!g_Ghost || !g_Ghost->m_pLight) return;

	// 毎フレーム Ghost の最新位置をライトに反映
	XMFLOAT3 ghostPos = g_Ghost->GetPos();
	g_Ghost->m_pLight->SetPosition(ghostPos.x, ghostPos.y, ghostPos.z);

	// シェーダーにライト情報を設定
	Shader_SetPointLight(g_Ghost->m_pLight);
}

// ========== Ghost クラスメソッドの実装 ==========

void Ghost::Transforming(void)
{
	Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
	if (pFurniture)
	{
		SetPos(pFurniture->GetPos());
	}

	Busters* pBuster = GetBusters();
	if (pBuster)
	{
		XMFLOAT3 busterPos = pBuster->GetPos();
		XMFLOAT3 ghostPos = GetPos();
		XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
		XMVECTOR busterVec = XMLoadFloat3(&busterPos);
		XMVECTOR distVec = XMVectorSubtract(busterVec, ghostVec);
		float distance = XMVectorGetX(XMVector3Length(distVec));

		float currentRange = GetCurrentScareRange();

		if (distance <= currentRange)
		{
			pBuster->SetIsGhostDiscover(true);
		}
		else
		{
			pBuster->SetIsGhostDiscover(false);
		}
	}
}

void Ghost::ScareStart(void)
{
	FurnitureScareStart(m_InRangeFurnitureNum);

	Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
	if (!pFurniture) return;

	Busters* pBuster = GetBusters();
	if (!pBuster) return;

	XMFLOAT3 busterPos = pBuster->GetPos();

	if (g_pScareSound)
	{
		PlaySound(g_pScareSound, false);
	}

	XMFLOAT3 ghostPos = GetPos();
	XMVECTOR distVec = XMVectorSubtract(XMLoadFloat3(&busterPos), XMLoadFloat3(&ghostPos));
	float distance = XMVectorGetX(XMVector3Length(distVec));

	FURNITURE_ACTION action = pFurniture->GetActionType();

	float currentRange = GetCurrentScareRange();

	switch (action)
	{
	case ACTION_SCARE:

		if (distance <= currentRange)
		{
			BustersScare();
			if (!m_HasIncreasedMultiplier)
			{
				ScareComboUP();
				m_HasIncreasedMultiplier = true;
			}
			AddScareGauge(SCORE_SCARE * UI_ScareCombo_GetNumber());
			Busters_CheckGaugeEvent();
		}
		break;

	case ACTION_LURE:

		if (distance <= currentRange * 2.0f)
		{
			BustersLured(ghostPos);
			if (!m_HasIncreasedMultiplier)
			{
				ScareComboUP();
				m_HasIncreasedMultiplier = true;
			}
			AddScareGauge(SCORE_LURE * UI_ScareCombo_GetNumber());
		}
		break;

	case ACTION_STOP:

		if (distance <= currentRange)
		{
			BustersStopped();
			if (!m_HasIncreasedMultiplier)
			{
				ScareComboUP();
				m_HasIncreasedMultiplier = true;
			}
			AddScareGauge(SCORE_STOP * UI_ScareCombo_GetNumber());
		}
		break;
	}
}

void Ghost::FurnitureSearch(void)
{
	float tempDistance = 999999.0f;
	int tempInRangeNum = -1;

	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		Furniture* pFurniture = GetFurniture(i);
		if (pFurniture)
		{
			if (pFurniture->IsCoolingDown()) continue;
			pFurniture->ResetColor();
			if (pFurniture->GetActionType() == ACTION_NONE) continue;

			if (pFurniture->GetDistanceToGhost() <= FURNITURE_DETECTION_RANGE &&
				pFurniture->GetDistanceToGhost() < tempDistance)
			{
				tempDistance = pFurniture->GetDistanceToGhost();
				tempInRangeNum = i;
			}
		}
	}

	if (tempInRangeNum != -1)
	{
		m_InRangeFurnitureNum = tempInRangeNum;
		Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
		if (pFurniture)
		{
			pFurniture->SetColor(1.0f, 1.0f, 0.0f, 1.0f);
			this->SetState(GS_FURNITURE_FOUND);
		}
	}
	else
	{
		m_InRangeFurnitureNum = -1;
		this->SetState(GS_MOVING);
	}
}

void Ghost::Move(void)
{
	if (m_IsTransformed)
		return;

	float cameraYaw = Camera_GetYaw();
	float yawRad = XMConvertToRadians(cameraYaw);
	float forwardX = sinf(yawRad);
	float forwardZ = cosf(yawRad);
	float rightX = cosf(yawRad);
	float rightZ = -sinf(yawRad);

	XMVECTOR accelVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	if (Keyboard_IsKeyDown(KK_W)) accelVec = XMVectorAdd(accelVec, XMVectorSet(forwardX * GHOST_ACCELERATION, 0.0f, forwardZ * GHOST_ACCELERATION, 0.0f));
	if (Keyboard_IsKeyDown(KK_S)) accelVec = XMVectorAdd(accelVec, XMVectorSet(-forwardX * GHOST_ACCELERATION, 0.0f, -forwardZ * GHOST_ACCELERATION, 0.0f));
	if (Keyboard_IsKeyDown(KK_D)) accelVec = XMVectorAdd(accelVec, XMVectorSet(rightX * GHOST_ACCELERATION, 0.0f, rightZ * GHOST_ACCELERATION, 0.0f));
	if (Keyboard_IsKeyDown(KK_A)) accelVec = XMVectorAdd(accelVec, XMVectorSet(-rightX * GHOST_ACCELERATION, 0.0f, -rightZ * GHOST_ACCELERATION, 0.0f));

	XMVECTOR velocityVec = XMLoadFloat3(&m_Velocity);
	velocityVec = XMVectorAdd(velocityVec, accelVec);

	float speed = XMVectorGetX(XMVector3Length(velocityVec));
	if (speed > GHOST_MAX_SPEED)
	{
		velocityVec = XMVectorScale(velocityVec, GHOST_MAX_SPEED / speed);
	}

	if (XMVectorGetX(accelVec) == 0.0f && XMVectorGetY(accelVec) == 0.0f && XMVectorGetZ(accelVec) == 0.0f)
	{
		velocityVec = XMVectorScale(velocityVec, GHOST_DECELERATION);
	}

	XMStoreFloat3(&m_Velocity, velocityVec);

	float moveVecX = m_Velocity.x;
	float moveVecZ = m_Velocity.z;

	if (moveVecX != 0.0f || moveVecZ != 0.0f)
	{
		float moveAngle = atan2f(moveVecX, moveVecZ);
		float moveYaw = XMConvertToDegrees(moveAngle);
		SetRot({ 0.0f, moveYaw - 180.0f, 0.0f });
	}

	float r = 0.4f;

	float nextX = m_Position.x + m_Velocity.x;
	bool hitX = false;

	if (Field_IsOuterWall(nextX + r, m_Position.z + r) ||
		Field_IsOuterWall(nextX + r, m_Position.z - r) ||
		Field_IsOuterWall(nextX - r, m_Position.z + r) ||
		Field_IsOuterWall(nextX - r, m_Position.z - r))
	{
		hitX = true;
	}

	if (hitX) m_Velocity.x = 0.0f;
	else m_Position.x = nextX;

	float nextZ = m_Position.z + m_Velocity.z;
	bool hitZ = false;

	if (Field_IsOuterWall(m_Position.x + r, nextZ + r) ||
		Field_IsOuterWall(m_Position.x + r, nextZ - r) ||
		Field_IsOuterWall(m_Position.x - r, nextZ + r) ||
		Field_IsOuterWall(m_Position.x - r, nextZ - r))
	{
		hitZ = true;
	}

	if (hitZ) m_Velocity.z = 0.0f;
	else m_Position.z = nextZ;

	SetPos(m_Position);
}

void Ghost::FloorMove(void)
{

	if (m_FloorCooldown > 0.0f)
	{
		m_FloorCooldown -= 1.0f / 60.0f;
	}

	if (m_FloorCooldown <= 0.0f)
	{
		FIELD_TYPE blockType = Field_GetBlockType(m_Position.x, m_Position.z);

		if (blockType == FIELD_STAIRS_UP || blockType == FIELD_STAIRS_DOWN)
		{
			if (m_FloorCooldown > 0.0f) SetColor(1.0f, 0.5f, 0.5f, 1.0f);
			else SetColor(0.7f, 1.0f, 0.7f, 1.0f);

			bool isClicked = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

			if (isClicked)
			{
				if (blockType == FIELD_STAIRS_UP)
				{
					int currentFloor = Field_GetCurrentFloor();
					if (currentFloor < MAP_FLOORS - 1)
					{
						Field_ChangeFloor(currentFloor + 1);
						m_Position.z += 1.2f;
						SetPos(m_Position);
						m_FloorCooldown = FLOOR_COOLDOWN_TIME;
					}
				}
				else if (blockType == FIELD_STAIRS_DOWN)
				{
					int currentFloor = Field_GetCurrentFloor();
					if (currentFloor > 0)
					{
						Field_ChangeFloor(currentFloor - 1);
						m_Position.z -= 1.2f;
						SetPos(m_Position);
						m_FloorCooldown = FLOOR_COOLDOWN_TIME;
					}
				}
			}
		}
		else
		{
			ResetColor();
		}
	}
}

void Ghost::ResetPos(void)
{
	m_Velocity = { 0.0f, 0.0f, 0.0f };
	m_Position = { m_Position.x, GHOST_POS_Y, m_Position.z };
	m_InRangeFurnitureNum = -1;
	m_IsTransformed = false;
	m_HasIncreasedMultiplier = false;
}

Ghost* GetGhost(void)
{
	return g_Ghost;
}