#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "ghost.h"
#include "sprite3d.h"
#include "component.h"
#include "define.h"
using namespace DirectX;

// �A�N�V�����̎��
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

	// �A�N�V��������p
	FURNITURE_ACTION m_ActionType;
	bool m_IsActing;
	float m_ActionTimer;
	float m_CooldownTimer; // クールタイムタイマー
	XMFLOAT3 m_BasePos; // �U���A�j���[�V�����p

public:
	// �R���X�g���N�^�� actionType ��ǉ�
	Furniture(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass, FURNITURE_ACTION actionType = ACTION_SCARE)
		: Sprite3D(pos, scale, rot, pass),
		Jump(0.01f, 0.2f, pos.y), // �n�ʂ̍���������Y���W�Ƃ���
		m_DistanceToGhost(0.0f),
		m_ActionType(actionType),
		m_IsActing(false),
		m_ActionTimer(0.0f),
		m_CooldownTimer(0.0f),
		m_BasePos(pos)
	{
	}

	~Furniture() = default;

	void Update(void);

	// �A�N�V�����J�n
	void StartAction(void);

	// �Q�b�^�[
	float GetDistanceToGhost(void) const { return m_DistanceToGhost; }
	FURNITURE_ACTION GetActionType(void) const { return m_ActionType; }
	bool GetIsActing(void) const { return m_IsActing || GetIsJumping(); }
	bool IsCoolingDown(void) const { return m_CooldownTimer > 0.0f; }
};

void Furniture_Initialize(void);
void Furniture_Update(void);
void Furniture_Draw(void);
void Furniture_Finalize(void);
void CreateFurniture(XMFLOAT3 pos, XMFLOAT3 scale, XMFLOAT3 rot, const char* modelPath, FURNITURE_ACTION action);
Furniture* GetFurniture(int index);
bool FurnitureScareStart(int index);
bool FurnitureScareEnded(int index);