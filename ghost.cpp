#include "ghost.h"
#include "main.h"
#include "sprite.h"
#include "sprite3d.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "camera.h"

Sprite3D* g_Ghost = NULL;
XMFLOAT3 g_GhostPos = { 0.0f, 1.3f, 0.0f };
XMFLOAT3 g_GhostVelocity = { 0.0f, 0.0f, 0.0f };  // Ghost の速度ベクトル

// 移動パラメータ定数
#define MOVEMENT_SPEED (0.1f)
#define ACCELERATION (0.005f)  // 加速度
#define DECELERATION (0.98f)  // 減速係数（1.0に近いほど遅く減速）
#define MAX_SPEED (0.10f)     // 最大速度

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// ②変数を初期化
	g_Ghost = new Sprite3D(
		g_GhostPos,			//位置
		{ 1.0f, 1.0f, 1.0f },			//スケール
		{ 0.0f, 0.0f, 0.0f },			//回転（度）
		"asset\\model\\ghost.fbx"		//モデルパス
	);
	
	// 速度を初期化
	g_GhostVelocity = { 0.0f, 0.0f, 0.0f };
}

void Ghost_Update(void)
{
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
	//// Space: 上昇
	//if (Keyboard_IsKeyDown(KK_SPACE))
	//{
	//	accelVec = XMVectorAdd(accelVec, XMVectorSet(0.0f, ACCELERATION, 0.0f, 0.0f));
	//}
	//// Shift: 下降
	//if (Keyboard_IsKeyDown(KK_LEFTSHIFT))
	//{
	//	accelVec = XMVectorAdd(accelVec, XMVectorSet(0.0f, -ACCELERATION, 0.0f, 0.0f));
	//}
	
	// 速度を更新（加速度を適用）
	XMVECTOR velocityVec = XMLoadFloat3(&g_GhostVelocity);
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
	
	XMStoreFloat3(&g_GhostVelocity, velocityVec);
	
	// 水平方向（XZ）の移動ベクトルから Ghost の向きを決定
	float moveVecX = g_GhostVelocity.x;
	float moveVecZ = g_GhostVelocity.z;
	
	// 水平方向の速度が0でない場合、その方向を向く
	if (moveVecX != 0.0f || moveVecZ != 0.0f)
	{
		float moveAngle = atan2f(moveVecX, moveVecZ);
		float moveYaw = XMConvertToDegrees(moveAngle);
		g_Ghost->SetRot({ 0.0f, moveYaw - 180.0f, 0.0f });
	}
	
	// Ghost位置を更新
	XMVECTOR posVec = XMLoadFloat3(&g_GhostPos);
	posVec = XMVectorAdd(posVec, velocityVec);
	XMStoreFloat3(&g_GhostPos, posVec);
	
	// Sprite3Dの位置を更新
	if (g_Ghost)
	{
		g_Ghost->SetPos(g_GhostPos);
	}

	// カメラの注視対象をGhost位置に設定
	Camera_SetTargetPos(g_Ghost->GetPos());
}

void Ghost_Draw(void)
{
	// ④モデル描画
	g_Ghost->Draw();
}

void Ghost_Finalize(void)
{
	// ⑤終了処理
	delete g_Ghost;
}