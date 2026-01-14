#include "furniture.h"
#include "Camera.h"
#include "shader.h"
#include "ghost.h"
#include "keyboard.h"
#include "define.h"
#include "Floor1.h"
#include <cmath>   // sinf用

Furniture* g_Furniture[FURNITURE_NUM]{};

static int g_FurnitureCount = 0;
// =========================================================
// 家具の配置 (Initialize)
// =========================================================
void Furniture_Initialize(void)
{
	// 1. カウントをリセット
	g_FurnitureCount = 0;

	for (int z = 0; z < MAP_LENGTH; z++)
	{
		for (int x = 0; x < MAP_WIDTH; x++)
		{

			int id = Floor1[1][z][x];

			// ワールド座標に変換
			float wx = (float)x - MAP_WIDTH / 2.0f;
			float wz = MAP_LENGTH / 2.0f - (float)z;

			switch (id)
			{

				case 50: 

					// 椅子を生成 (手動配置と同じ関数を使う)
					CreateFurniture(
						{ wx, 0.0f, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, 0.0f, 0.0f },      // 回転(固定)
						"asset\\model\\rockingchair.fbx", // モデル
						ACTION_SCARE                // アクション
					);
				break;

				case 51: 
					// 木を生成 (手動配置と同じ関数を使う)
					CreateFurniture(
						{ wx, 0.0f, wz },          // 場所
						{ 2.0f, 2.0f, 2.0f },      // サイズ
						{ 0.0f, 0.0f, 0.0f },      // 回転(固定)
						"asset\\model\\tree.fbx",  // モデル
						ACTION_LURE                 // アクション
					);
				break;

				case 52: 
					// 樹を生成 (手動配置と同じ関数を使う)
					CreateFurniture(
						{ wx, 0.0f, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, 0.0f, 0.0f },      // 回転(固定)
						"asset\\model\\aaaa.fbx",  // モデル
						ACTION_STOP                 // アクション
					);
			}
		}
	}

	// =========================================================
	// 手動配置
	// =========================================================

	// 1: ロッキングチェア
	CreateFurniture(
		{ -5.0f, 0.0f, -5.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 45.0f, 0.0f },
		"asset\\model\\rockingchair.fbx", ACTION_SCARE
	);

	CreateFurniture(
		{ 5.0f, 0.0f, -5.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, -45.0f, 0.0f },
		"asset\\model\\rockingchair.fbx", ACTION_SCARE
	);

	// 2: 木
	CreateFurniture(
		{ -6.0f, 0.0f, 6.0f }, { 1.5f, 1.5f, 1.5f }, { 0.0f, 0.0f, 0.0f },
		"asset\\model\\tree.fbx", ACTION_LURE
	);

	CreateFurniture(
		{ 6.0f, 0.0f, 6.0f }, { 1.5f, 1.5f, 1.5f }, { 0.0f, 45.0f, 0.0f },
		"asset\\model\\tree.fbx", ACTION_LURE
	);

	// 3: 樹（停止アクション）
	CreateFurniture(
		{ -6.0f, 0.0f, -2.0f }, { 0.5f, 0.5f, 0.5f }, { 0.0f, 90.0f, 0.0f },
		"asset\\model\\aaaa.fbx", ACTION_STOP
	);

	CreateFurniture(
		{ 6.0f, 0.0f, -2.0f }, { 1.5f, 1.5f, 1.5f }, { 0.0f, 135.0f, 0.0f },
		"asset\\model\\tree.fbx", ACTION_STOP
	);
}

// =========================================================
// 家具クラスのメソッド実装
// =========================================================

void Furniture::Update(void)
{
	// 1. Ghostとの距離計算
	Ghost* pGhost = GetGhost();
	if (pGhost)
	{
		XMFLOAT3 ghostPos = pGhost->GetPos();
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
		case ACTION_SCARE: // 驚かせ -> ジャンプ
			JumpUpdate(*(Transform3D*)this);
			break;

		case ACTION_LURE: // 誘引する -> 揺れ
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

		case ACTION_STOP: // 停止中 -> 回転
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

void CreateFurniture(XMFLOAT3 pos, XMFLOAT3 scale, XMFLOAT3 rot, const char* modelPath, FURNITURE_ACTION action)
{
	// 配列がいっぱいなら何もしない（エラー防止）
	if (g_FurnitureCount >= FURNITURE_NUM) return;

	// 配列の「次の空いている場所」に家具を作る
	g_Furniture[g_FurnitureCount] = new Furniture(pos, scale, rot, modelPath, action);

	// 地面の高さをセット（共通設定をここに移動）
	g_Furniture[g_FurnitureCount]->SetGroundLevel(0.0f);

	// カウントを進める
	g_FurnitureCount++;
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