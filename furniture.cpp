#include "furniture.h"
#include "Camera.h"
#include "shader.h"
#include "ghost.h"
#include "keyboard.h"
#include "define.h"
#include "Floor1.h"
#include "Floor2.h"
#include "Floor3.h"
#include "field.h"
#include <cmath>   // sinf用

Furniture* g_Furniture[FURNITURE_NUM]{};

static int g_FurnitureCount = 0;

int GetFurnitureBlockID(int floor, int y, int z, int x)
{
	// 範囲チェック
	if (y < 0 || y >= MAP_HEIGHT || z < 0 || z >= MAP_LENGTH || x < 0 || x >= MAP_WIDTH) return 0;

	switch (floor)
	{
	case 0: return Floor1[y][z][x];
	case 1: return Floor2[y][z][x];
	case 2: return Floor3[y][z][x];
	default: return 0;
	}
}

// =========================================================
// 家具の配置 (Initialize)
// =========================================================
void Furniture_Initialize(void)
{
	Furniture_Finalize();

	// 1. カウントをリセット
	g_FurnitureCount = 0;

	int currentFloor = Field_GetCurrentFloor();

	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int z = 0; z < MAP_LENGTH; z++)
		{
			for (int x = 0; x < MAP_WIDTH; x++)
			{

				int id = GetFurnitureBlockID(currentFloor, 1, z, x);

				// ワールド座標に変換
				float wx = (float)x - MAP_WIDTH / 2.0f;
				float wz = MAP_LENGTH / 2.0f - (float)z;
				float wy = (float)y - 1.0f;

				float rotY = Field_CalculateRotationFromMarker(wx, wy, wz);

				switch (id)
				{
				case 50:

					// キャビネット
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 51:
					// 高い本棚
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\tall_bookshelf.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 52:
					// ソファー
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\sofa.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 53:
					// シャンデリア
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 54:
					// ロッキングチェア
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\rockingchair.fbx", // モデル
						ACTION_LURE                // アクション
					);
					break;
				case 55:
					// 蓄音機
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\phonograph.fbx", // モデル
						ACTION_STOP               // アクション
					);
					break;
				case 56:
					// カーペット
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_STOP                // アクション
					);
					break;
				case 57:
					// 暖炉
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\danro.fbx", // モデル
						ACTION_LURE                // アクション
					);
					break;
				case 58:
					// バスタブ
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\bathtub.fbx", // モデル
						ACTION_LURE                // アクション
					);
					break;
				case 59:
					// キッチン
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\kitchen.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 60:
					// 便器
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_LURE                // アクション
					);
					break;
				case 61:
					// ベッド
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_STOP               // アクション
					);
					break;
				case 62:
					// ピアノ
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 63:
					// シンク
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 64:
					// 鏡
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\mirror.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 65:
					// ハンティングトロフィー
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 66:
					// 偽扉
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_SCARE                // アクション
					);
					break;
				case 67:
					// 振り子時計
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_LURE               // アクション
					);
					break;
				case 68:
					// 椅子
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_NONE                // アクション
					);
					break;
				case 69:
					// ダイニングテーブル
					CreateFurniture(
						{ wx, wy, wz },          // 場所
						{ 1.0f, 1.0f, 1.0f },      // サイズ
						{ 0.0f, rotY, 0.0f },      // 回転
						"asset\\model\\cube.fbx", // モデル
						ACTION_NONE                // アクション
					);
					break;
				}
			}
		}
	}

	// =========================================================
	// 手動配置（基本使わない）
	// =========================================================

	//// 1: ロッキングチェア
	//CreateFurniture(
	//	{ -5.0f, 0.0f, -5.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 45.0f, 0.0f },
	//	"asset\\model\\rockingchair.fbx", ACTION_SCARE
	//);
}

// =========================================================
// 家具クラスのメソッド実装
// =========================================================

void Furniture::Update(void)
{
	// 0. クールタイムの更新
	if (m_CooldownTimer > 0.0f)
	{
		m_CooldownTimer -= 1.0f / 60.0f;
		if (m_CooldownTimer <= 0.0f)
		{
			m_CooldownTimer = 0.0f;
			ResetColor(); // クールタイム終了で色を戻す
		}
		else
		{
			SetColor(0.0f, 0.0f, 1.0f, 1.0f); // クールタイム中は青色
		}
	}

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
	if (GetIsActing() || IsCoolingDown()) return; // 二重実行およびクールタイム中の実行を抑止

	if (m_ActionType == ACTION_SCARE)
	{
		JumpStart(); // ジャンプフラグON
	}
	else
	{
		m_IsActing = true; // 行う、アクション開始フラグON
		m_ActionTimer = 0.0f;
	}

	m_CooldownTimer = 10.0f; // 10秒間のクールタイムを設定
}

void CreateFurniture(XMFLOAT3 pos, XMFLOAT3 scale, XMFLOAT3 rot, const char* modelPath, FURNITURE_ACTION action)
{
	// 配列がいっぱいなら何もしない（エラー防止）
	if (g_FurnitureCount >= FURNITURE_NUM) return;

	// 配列の「次の空いている場所」に家具を作る
	g_Furniture[g_FurnitureCount] = new Furniture(pos, scale, rot, modelPath, action);

	// 地面の高さをセット
	g_Furniture[g_FurnitureCount]->SetGroundLevel(pos.y);

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