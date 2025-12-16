#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "ghost.h"
#include "sprite3d.h"
#include "component.h"
#include "define.h"

using namespace DirectX;

// アクションの種類
enum FURNITURE_ACTION
{
	ACTION_SCARE,	// 既存の驚かし（ジャンプ）
	ACTION_LURE,	// おびき寄せ（振動）
	ACTION_STOP,	// 足止め（回転）
};

// Furniture クラス
class Furniture : public Sprite3D, public Jump
{
protected:
	float m_DistanceToGhost;

	// アクション制御用
	FURNITURE_ACTION m_ActionType;
	bool m_IsActing;
	float m_ActionTimer;
	XMFLOAT3 m_BasePos; // 振動アニメーション用

public:
	// コンストラクタに actionType を追加
	Furniture(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass, FURNITURE_ACTION actionType = ACTION_SCARE)
		: Sprite3D(pos, scale, rot, pass),
		Jump(0.01f, 0.2f, pos.y), // 地面の高さを初期Y座標とする
		m_DistanceToGhost(0.0f),
		m_ActionType(actionType),
		m_IsActing(false),
		m_ActionTimer(0.0f),
		m_BasePos(pos)
	{
	}

	~Furniture() = default;

	void Update(void);

	// アクション開始
	void StartAction(void);

	// ゲッター
	float GetDistanceToGhost(void) const { return m_DistanceToGhost; }
	FURNITURE_ACTION GetActionType(void) const { return m_ActionType; }
	bool GetIsActing(void) const { return m_IsActing || GetIsJumping(); }
};

void Furniture_Initialize(void);
void Furniture_Update(void);
void Furniture_Draw(void);
void Furniture_Finalize(void);
Furniture* GetFurniture(int index);
bool FurnitureScareStart(int index);
bool FurnitureScareEnded(int index);