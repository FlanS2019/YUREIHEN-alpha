#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "ghost.h"
#include "sprite3d.h"
#include "component.h"
#include "define.h"
#include "billboard.h"
#include "light.h"
#include <string>
using namespace DirectX;

// アクションの種類
enum FURNITURE_ACTION
{
	ACTION_SCARE,	//驚かせ
	ACTION_LURE,	//引き寄せ
	ACTION_STOP,	//妨害
	ACTION_NONE,	//なにもなし
};

// Furniture クラス - 色を変更でき、ジャンプ機能を持つ
class Furniture : public Sprite3D, public Jump
{
protected:
	//ghostとの距離を保持する変数
	float m_DistanceToGhost;

	// アクション用パラメータ
	FURNITURE_ACTION m_ActionType;
	bool m_IsActing;
	float m_ActionTimer;
	float m_CooldownTimer; // クールタイムタイマー
	XMFLOAT3 m_BasePos; // ベースポジション
	int m_BlockID;     // ブロックIDを保持

	Billboard m_Billboard; // 近接時に表示するアイコン

	// ターゲットフラグ
	bool m_IsTargeted;    // バスターズ用
	bool m_IsGhostTarget; // 幽霊用

	// 壁掛けライト用ポイントライト
	bool m_IsWallLight;
	PointLight* m_pPointLight;

public:
	// コンストラクタ
	Furniture(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass, FURNITURE_ACTION actionType = ACTION_SCARE, int blockID = 0)
		: Sprite3D(pos, scale, rot, pass),
		Jump(0.01f, 0.2f, pos.y), // 地面の高さyで初期化
		m_DistanceToGhost(0.0f),
		m_ActionType(actionType),
		m_IsActing(false),
		m_ActionTimer(0.0f),
		m_CooldownTimer(0.0f),
		m_BasePos(pos),
		m_BlockID(blockID),
		m_IsTargeted(false),
		m_IsGhostTarget(false),
		m_IsWallLight(false),
		m_pPointLight(nullptr)
	{
		// ビルボードの初期化 (サイズを大きくし、位置は後でUpdateで調整する)
		m_Billboard.Initialize({ pos.x, pos.y + 1.0f, pos.z }, { 1.0f, 1.0f }, { 0,0,0 });
		// ライティング無効化を有効にして、アイコンを常に明るく表示
		m_Billboard.SetIgnoreLighting(true);

		// 壁掛けライト（ID:70）の場合、ポイントライトを生成
		if (blockID == 70)
		{
			m_IsWallLight = true;
			m_pPointLight = new PointLight(
				TRUE,
				XMFLOAT4(pos.x, pos.y + 0.5f, pos.z, 1.0f),
				XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f),
				XMFLOAT4(1.0f, 0.85f, 0.6f, 1.0f), // 暖色系の光
				5.0f,  // 範囲
				1.5f   // 強度
			);
		}
	}

	~Furniture()
	{
		if (m_pPointLight)
		{
			delete m_pPointLight;
			m_pPointLight = nullptr;
		}
	}

	void Update(void);
	void Draw(void) override;

	// アクション開始
	void StartAction(void);

	// ゲッター
	float GetDistanceToGhost(void) const { return m_DistanceToGhost; }
	FURNITURE_ACTION GetActionType(void) const { return m_ActionType; }
	bool GetIsActing(void) const { return m_IsActing || GetIsJumping(); }
	bool IsCoolingDown(void) const { return m_CooldownTimer > 0.0f; }
	float GetCooldownTimer(void) const { return m_CooldownTimer; }
	int GetBlockID(void) const { return m_BlockID; }
	void SetBasePos(const XMFLOAT3& pos) { m_BasePos = pos; }

	// セッターとゲッター（ターゲット関連）
	void SetIsTargeted(bool targeted) { m_IsTargeted = targeted; }
	bool GetIsTargeted(void) const { return m_IsTargeted; }

	// 幽霊用のセッターとゲッター
	void SetIsGhostTarget(bool targeted) { m_IsGhostTarget = targeted; }
	bool GetIsGhostTarget(void) const { return m_IsGhostTarget; }

	// 壁掛けライト関連
	bool IsWallLight(void) const { return m_IsWallLight; }
	PointLight* GetPointLight(void) const { return m_pPointLight; }
};

void Furniture_Initialize(void);
void Furniture_Update(void);
void Furniture_Draw(void);
void Furniture_Finalize(void);
std::string GetBlockNameJa(int id);
void CreateFurniture(XMFLOAT3 pos, XMFLOAT3 scale, XMFLOAT3 rot, const char* modelPath, FURNITURE_ACTION action, int blockID);
Furniture* GetFurniture(int index);
bool FurnitureScareStart(int index);
bool FurnitureScareEnded(int index);
bool IsFurnitureBlock(int id);
void Furniture_SetLight(void);