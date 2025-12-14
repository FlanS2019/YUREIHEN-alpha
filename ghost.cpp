#include "ghost.h"
#include "main.h"
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

Ghost* g_Ghost = NULL;

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_Ghost = new Ghost(
		{ -3.0f, Ghost::GetGhostPosY(), -10.0f },	// 位置 選ばれた初期位置
		{ 1.0f, 1.0f, 1.0f },						// スケール
		{ 0.0f, 180.0f, 0.0f },						// 回転（度）
		"asset\\model\\ghost.fbx"					// モデルパス
	);
}

void Ghost_Update(void)
{
	if (!g_Ghost) return;

	switch (g_Ghost->GetState())
	{
	case GS_MOVING:
		g_Ghost->SetIsDraw(true);		// 描画有効化
		g_Ghost->Move();				// 移動処理
		g_Ghost->FurnitureSearch();		// 近くの家具検知と色変更
		g_Ghost->FloorMove();			// 階段移動処理
		break;
	case GS_FURNITURE_FOUND:
		g_Ghost->SetIsDraw(true);		// 描画有効化
		g_Ghost->Move();				// 移動処理
		g_Ghost->FurnitureSearch();		// 近くの家具検知と色変更
		g_Ghost->FloorMove();			// 階段移動処理

		// 変身開始処理
		if (Keyboard_IsKeyDownTrigger(KK_E))
		{
			g_Ghost->SetState(GS_TRANSFORM);
		}

		break;
	case GS_TRANSFORM:
		g_Ghost->SetIsDraw(false);		// 描画無効化
		g_Ghost->Transforming();		// 変身処理

		// spaceで驚かしアクション
		if (Keyboard_IsKeyDownTrigger(KK_SPACE))
		{
			g_Ghost->SetState(GS_SCARE);

			// 家具とプレイヤーをジャンプさせる(ここで呼ぶのキモい)
			g_Ghost->ScareStart();
		}

		// 変身解除
		if (Keyboard_IsKeyDownTrigger(KK_E))
		{
			g_Ghost->ResetPos();
			g_Ghost->SetState(GS_MOVING);

		}
		break;
	case GS_SCARE:
		g_Ghost->SetIsDraw(false);		// 描画無効化
		g_Ghost->Transforming();		// 変身処理
		// 家具のジャンプが終わったらTransFormに戻る
		if (FurnitureScareEnded(g_Ghost->GetInRangeNum()))
		{
			g_Ghost->SetState(GS_TRANSFORM);
		}
		break;
	default:
		break;
	}

	// カメラの注視対象をGhost位置に設定
	Camera_SetTargetPos(g_Ghost->GetPos());

	// ステート別デバッグ出力
	//hal::dout << "Ghost State: " << g_Ghost->GetState() << std::endl;
}

void Ghost_Draw(void)
{
	g_Ghost->Draw();
}

void Ghost_Finalize(void)
{
	if (g_Ghost)
	{
		delete g_Ghost;
		g_Ghost = NULL;
	}
}

// ========== Ghost クラスメソッドの実装 ==========

void Ghost::Transforming(void)
{
	// Ghostを家具の位置に合わせる
	Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
	if (pFurniture)
	{
		SetPos(pFurniture->GetPos());
	}

	// Ghost（Furniture） と buster の距離を計算
	XMFLOAT3 busterPos = GetBusters()->GetPos();
	XMFLOAT3 ghostPos = GetPos();
	XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
	XMVECTOR busterVec = XMLoadFloat3(&busterPos);
	XMVECTOR distVec = XMVectorSubtract(busterVec, ghostVec);
	float distance = XMVectorGetX(XMVector3Length(distVec));

	// 発見距離以下なら発見処理
	if (distance <= SCARE_RANGE)
	{
		// 検知範囲に入っているなら色を変える
		GetBusters()->SetIsGhostDiscover(true);
	}
	else
	{
		GetBusters()->SetIsGhostDiscover(false);
	}
}

void Ghost::ScareStart(void)
{
	FurnitureScareStart(m_InRangeFurnitureNum);

	// Ghost（Furniture） と buster の距離を計算
	XMFLOAT3 busterPos = GetBusters()->GetPos();
	XMFLOAT3 ghostPos = GetPos();
	XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
	XMVECTOR busterVec = XMLoadFloat3(&busterPos);
	XMVECTOR distVec = XMVectorSubtract(busterVec, ghostVec);
	float distance = XMVectorGetX(XMVector3Length(distVec));

	// 発見距離以下なら発見処理
	if (distance <= SCARE_RANGE)
	{
		BustersScare();								// 
		ScareComboUP();								// 恐怖コンボを上げる
		AddScareGauge(1.0f * UI_ScareCombo_GetNumber());	// 恐怖ゲージ加算
	}
}

void Ghost::FurnitureSearch(void)
{
	float tempDistance = 999999.0f;
	int tempInRangeNum = -1;

	// Ghost最近傍のFurnitureを探す
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		Furniture* pFurniture = GetFurniture(i);
		if (pFurniture)
		{
			pFurniture->ResetColor();  // 元の色に戻す

			// 家具検出範囲内で一時保存された家具との距離が近い場合
			if (pFurniture->GetDistanceToGhost() <= FURNITURE_DETECTION_RANGE &&
				pFurniture->GetDistanceToGhost() < tempDistance)
			{
				tempDistance = pFurniture->GetDistanceToGhost();
				tempInRangeNum = i;
			}
		}
	}

	// 最も近い家具を変身対象とする
	if (tempInRangeNum != -1)
	{
		m_InRangeFurnitureNum = tempInRangeNum;

		// 検出範囲内の家具がある場合、その家具の色を変える
		Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
		if (pFurniture)
		{
			pFurniture->SetColor(1.0f, 1.0f, 0.0f, 1.0f);  // 黄色
			this->SetState(GS_FURNITURE_FOUND);
		}
	}
	else
	{
		this->SetState(GS_MOVING);
	}
}

void Ghost::Move(void)
{
	// 変身しているときのみ移動不可
	if (m_IsTransformed)
		return;

	// --- 入力と加速処理 (既存はそのまま) ---
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

	// 速度制限
	float speed = XMVectorGetX(XMVector3Length(velocityVec));
	if (speed > GHOST_MAX_SPEED)
	{
		velocityVec = XMVectorScale(velocityVec, GHOST_MAX_SPEED / speed);
	}

	// 減速
	if (XMVectorGetX(accelVec) == 0.0f && XMVectorGetY(accelVec) == 0.0f && XMVectorGetZ(accelVec) == 0.0f)
	{
		velocityVec = XMVectorScale(velocityVec, GHOST_DECELERATION);
	}

	XMStoreFloat3(&m_Velocity, velocityVec);

	// 向きの変更
	float moveVecX = m_Velocity.x;
	float moveVecZ = m_Velocity.z;

	if (moveVecX != 0.0f || moveVecZ != 0.0f)
	{
		float moveAngle = atan2f(moveVecX, moveVecZ);
		float moveYaw = XMConvertToDegrees(moveAngle);
		SetRot({ 0.0f, moveYaw - 180.0f, 0.0f });
	}

	// Ghost位置を更新
	// =========================================================
	// 壁当たり判定 (Collision)
	// =========================================================

	// 当たり判定の半径 
	float r = 0.4f;

	// --- X軸の移動と判定 ---
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

	// --- Z軸の移動と判定 ---
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
	// 1. クールダウンタイマーの更新
	if (m_FloorCooldown > 0.0f)
	{
		m_FloorCooldown -= 1.0f / 60.0f;
	}

	// =========================================================
	// 階段検知と移動処理 (既存コード)
	// =========================================================
	if (m_FloorCooldown <= 0.0f)
	{
		FIELD_TYPE blockType = Field_GetBlockType(m_Position.x, m_Position.z);

		if (blockType == FIELD_STAIRS_UP || blockType == FIELD_STAIRS_DOWN)
		{
			// 色変え
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
}

// ghostのゲッター
Ghost* GetGhost(void)
{
	return g_Ghost;
}
