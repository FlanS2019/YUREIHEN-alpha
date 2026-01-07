#include "furniture.h"
#include "Camera.h"
#include "shader.h"
#include "ghost.h"
#include "keyboard.h"
#include "define.h"
#include <cmath>   // sinf用

Furniture* g_Furniture[FURNITURE_NUM]{};

// =========================================================
// 家具の配置 (Initialize)
// =========================================================
void Furniture_Initialize(void)
{
	// 1: ロッキングチェア -> SCARE (驚かし/ジャンプ)
	g_Furniture[0] = new Furniture(
		{ -5.0f, 0.0f, -5.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 45.0f, 0.0f },
		"asset\\model\\rockingchair.fbx",
		ACTION_SCARE // 驚かし
	);
	
	g_Furniture[1] = new Furniture(
		{ 5.0f, 0.0f, -5.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, -45.0f, 0.0f },
		"asset\\model\\rockingchair.fbx",
		ACTION_SCARE // 驚かし
	);

	// 2: 木 -> LURE (おびき寄せ/振動)
	g_Furniture[2] = new Furniture(
		{ -6.0f, 0.0f, 6.0f }, { 1.5f, 1.5f, 1.5f }, { 0.0f, 0.0f, 0.0f },
		"asset\\model\\tree.fbx",
		ACTION_LURE // おびき寄せ
	);

	g_Furniture[3] = new Furniture(
		{ 6.0f, 0.0f, 6.0f }, { 1.5f, 1.5f, 1.5f }, { 0.0f, 45.0f, 0.0f },
		"asset\\model\\tree.fbx",
		ACTION_LURE // おびき寄せ
	);

	// 3: 樹 -> STOP (回転・停止)
	g_Furniture[4] = new Furniture(
		{ -6.0f, 0.0f, -2.0f }, { 0.5f, 0.5f, 0.5f }, { 0.0f, 90.0f, 0.0f },
		"asset\\model\\aaaa.fbx",
		ACTION_STOP // 足止め
	);

	g_Furniture[5] = new Furniture(
		{ 6.0f, 0.0f, -2.0f }, { 1.5f, 1.5f, 1.5f }, { 0.0f, 135.0f, 0.0f },
		"asset\\model\\tree.fbx",
		ACTION_STOP // 足止め
	);

	// 共通設定
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i]) g_Furniture[i]->SetGroundLevel(0.0f);
	}
}

// =========================================================
// 家具クラスのメソッド実装
// =========================================================

void Furniture::Update(void)
{
	// 1. Ghostとの距離計算
	if (GetGhost())
	{
		XMFLOAT3 ghostPos = GetGhost()->GetPos();
		XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
		XMVECTOR furnitureVec = XMLoadFloat3(&m_Position);
		XMVECTOR distVec = XMVectorSubtract(furnitureVec, ghostVec);
		m_DistanceToGhost = XMVectorGetX(XMVector3Length(distVec));
	}

	// 2. アクション実行時の処理 (ビジュアル変化)
	if (m_IsActing || GetIsJumping())
	{

		switch (m_ActionType)
		{
		case ACTION_SCARE: // 驚かせ -> ジャンプの動き
			JumpUpdate(*(Transform3D*)this);
			break;

		case ACTION_LURE: // 誘惑する -> 揺れ動きのもの
		{
			m_ActionTimer += 1.0f;
			// 揺れ計算
			float shakeAmount = 0.1f;
			float offsetX = sinf(m_ActionTimer * 2.0f) * shakeAmount;

			// 減衰
			float decay = 1.0f - (m_ActionTimer / 60.0f);
			if (decay < 0.0f) decay = 0.0f;
			offsetX *= decay;

			SetPosX(m_BasePos.x + offsetX);

			if (m_ActionTimer >= 60.0f)
			{
				m_IsActing = false;
				SetPos(m_BasePos);
			}
		}
		break;

		case ACTION_STOP: // 停止中 -> 回転するのもの
		{
			m_ActionTimer += 1.0f;
			// 回転計算
			float speed = 30.0f;
			AddRotY(speed);

			if (m_ActionTimer >= 60.0f)
			{
				m_IsActing = false;
			}
		}
		break;
		}
	}
}

void Furniture::StartAction(void)
{
	if (GetIsActing()) return; // 二重実行を抑止

	if (m_ActionType == ACTION_SCARE)
	{
		JumpStart(); // ジャンプフラグON
	}
	else
	{
		m_IsActing = true; // 行う、アクション開始フラグON
		m_ActionTimer = 0.0f;
	}
}

// =========================================================
// グローバル関数の実装
// =========================================================

// Game_Update から呼ばれ全ての家具の更新処理
void Furniture_Update(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i]) g_Furniture[i]->Update();
	}
}

void Furniture_Draw(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i]) g_Furniture[i]->Draw();
	}
}

void Furniture_Finalize(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i]) { delete g_Furniture[i]; g_Furniture[i] = nullptr; }
	}
}

Furniture* GetFurniture(int index)
{
	if (index >= 0 && index < FURNITURE_NUM) return g_Furniture[index];
	return nullptr;
}

bool FurnitureScareStart(int index)
{
	if (index >= 0 && index < FURNITURE_NUM && g_Furniture[index])
	{
		g_Furniture[index]->StartAction();
		return true;
	}
	return false;
}

bool FurnitureScareEnded(int index)
{
	if (index >= 0 && index < FURNITURE_NUM && g_Furniture[index])
	{
		return !g_Furniture[index]->GetIsActing();
	}
	return false;
}