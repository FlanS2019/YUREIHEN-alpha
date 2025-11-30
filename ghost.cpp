#include "ghost.h"
#include "main.h"
#include "sprite.h"
#include "sprite3d.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "camera.h"
#include "furniture.h"
#include "UI.h"

Ghost* g_Ghost = NULL;

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_Ghost = new Ghost(
		{ 0.0f, Ghost::GetGhostPosY(), 0.0f },	//位置　終わってる初期化
		{ 1.0f, 1.0f, 1.0f },					//スケール
		{ 0.0f, 0.0f, 0.0f },					//回転（度）
		"asset\\model\\ghost.fbx"				//モデルパス
	);
}

void Ghost_Update(void)
{
	if (!g_Ghost) return;

	// 家具検知と色変更
	g_Ghost->UpdateFurnitureDetection();

	// 変身状態の切り替えと、変身中の処理
	g_Ghost->UpdateInput();

	// 移動処理
	g_Ghost->UpdateMovement();

	// カメラの注視対象をGhost位置に設定
	Camera_SetTargetPos(g_Ghost->GetPos());
}

void Ghost_Draw(void)
{
	if (g_Ghost && !g_Ghost->GetIsTransformed())
	{
		g_Ghost->Draw();
	}
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

void Ghost::UpdateFurnitureDetection(void)
{
	// Ghost近くのFurnitureを探索して色を変更
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		Furniture* pFurniture = GetFurniture(i);
		if (pFurniture)
		{
			// Ghost と Furniture の距離を計算
			XMFLOAT3 furniturePos = pFurniture->GetPos();
			XMFLOAT3 ghostPos = GetPos();
			XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
			XMVECTOR furnitureVec = XMLoadFloat3(&furniturePos);
			XMVECTOR distVec = XMVectorSubtract(furnitureVec, ghostVec);
			float distance = XMVectorGetX(XMVector3Length(distVec));

			// 距離が検出範囲内なら黄色、範囲外なら元の色に戻す
			if (distance <= FURNITURE_DETECTION_RANGE)
			{
				pFurniture->SetColor(1.0f, 1.0f, 0.0f, 1.0f);  // 黄色
				m_InRangeNum = i;
			}
			else
			{
				pFurniture->ResetColor();  // 元の色に戻す
				m_InRangeNum = -1;
			}
		}
	}
}

void Ghost::UpdateInput(void)
{
	// Eキーで変身アクション
	if (Keyboard_IsKeyDownTrigger(KK_E))
	{
		// 変身中（変身解除）
		if (m_IsTransformed)
		{
			m_IsTransformed = false;
			m_Velocity = { 0.0f, 0.0f, 0.0f };	// 速度をリセット
			SetPosY(GHOST_POS_Y); // Ghostを初期位置に戻す
		}
		// 検知範囲にいる場合
		else if (m_InRangeNum != -1)
		{
			m_IsTransformed = true;
		}
	}

	// 変身しているとき
	if (m_IsTransformed)
	{
		// Ghostを家具の位置に合わせる
		Furniture* pFurniture = GetFurniture(m_InRangeNum);
		if (pFurniture)
		{
			SetPos(pFurniture->GetPos());
		}

		// 恐怖アクション　スペースキーでジャンプ
		if (Keyboard_IsKeyDownTrigger(KK_SPACE))
		{
			Furniture* pFurniture = GetFurniture(m_InRangeNum);
			if (pFurniture)
			{
				pFurniture->Jump();
				// 恐怖ゲージ加算（デバッグ用。本来は怖がらせられたら……）
				AddScareGauge();
			}
		}
	}
}

void Ghost::UpdateMovement(void)
{
	// 変身していないときのみ移動可能
	if (m_IsTransformed)
		return;

	// カメラのヨー角を取得
	float cameraYaw = Camera_GetYaw();
	float yawRad = XMConvertToRadians(cameraYaw);

	// カメラの向きに基づいた方向ベクトル
	float forwardX = sinf(yawRad);
	float forwardZ = cosf(yawRad);
	float rightX = cosf(yawRad);
	float rightZ = -sinf(yawRad);

	// 入力による加速度ベクトル
	XMVECTOR accelVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	// W: 前方移動（カメラの向き）
	if (Keyboard_IsKeyDown(KK_W))
	{
		accelVec = XMVectorAdd(accelVec, XMVectorSet(forwardX * ACCELERATION, 0.0f, forwardZ * ACCELERATION, 0.0f));
	}
	// S: 後方移動
	if (Keyboard_IsKeyDown(KK_S))
	{
		accelVec = XMVectorAdd(accelVec, XMVectorSet(-forwardX * ACCELERATION, 0.0f, -forwardZ * ACCELERATION, 0.0f));
	}
	// D: 右移動
	if (Keyboard_IsKeyDown(KK_D))
	{
		accelVec = XMVectorAdd(accelVec, XMVectorSet(rightX * ACCELERATION, 0.0f, rightZ * ACCELERATION, 0.0f));
	}
	// A: 左移動
	if (Keyboard_IsKeyDown(KK_A))
	{
		accelVec = XMVectorAdd(accelVec, XMVectorSet(-rightX * ACCELERATION, 0.0f, -rightZ * ACCELERATION, 0.0f));
	}

	// 速度を更新（加速度を適用）
	XMVECTOR velocityVec = XMLoadFloat3(&m_Velocity);
	velocityVec = XMVectorAdd(velocityVec, accelVec);

	// 速度の大きさを制限
	float speed = XMVectorGetX(XMVector3Length(velocityVec));
	if (speed > MAX_SPEED)
	{
		velocityVec = XMVectorScale(velocityVec, MAX_SPEED / speed);
	}

	// 入力がない場合は減速
	if (XMVectorGetX(accelVec) == 0.0f && XMVectorGetY(accelVec) == 0.0f && XMVectorGetZ(accelVec) == 0.0f)
	{
		velocityVec = XMVectorScale(velocityVec, DECELERATION);
	}

	XMStoreFloat3(&m_Velocity, velocityVec);

	// 水平方向（XZ）の移動ベクトルから Ghost の向きを決定
	float moveVecX = m_Velocity.x;
	float moveVecZ = m_Velocity.z;

	// 水平方向の速度が0でない場合、その方向を向く
	if (moveVecX != 0.0f || moveVecZ != 0.0f)
	{
		float moveAngle = atan2f(moveVecX, moveVecZ);
		float moveYaw = XMConvertToDegrees(moveAngle);
		SetRot({ 0.0f, moveYaw - 180.0f, 0.0f });
	}

	// Ghost位置を更新
	XMFLOAT3 ghostPos = GetPos();
	XMVECTOR posVec = XMLoadFloat3(&ghostPos);
	posVec = XMVectorAdd(posVec, velocityVec);
	XMFLOAT3 newPos;
	XMStoreFloat3(&newPos, posVec);
	SetPos(newPos);
}

void Ghost::ResetState(void)
{
	m_Velocity = { 0.0f, 0.0f, 0.0f };
	m_InRangeNum = -1;
	m_IsTransformed = false;
}