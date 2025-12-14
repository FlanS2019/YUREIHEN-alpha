#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "sprite3d.h"
#include "define.h"

using namespace DirectX;

enum GHOST_STATE
{
	GS_MOVING,			// �ړ�
	GS_FURNITURE_FOUND,	// �Ƌ��
	GS_TRANSFORM,		// �ϐg��
	GS_SCARE,			// ��������
};

// Ghost �N���X
class Ghost : public Sprite3D
{
private:
	XMFLOAT3 m_Velocity;		// Ghost�̑��x�x�N�g��
	int m_InRangeFurnitureNum;	// �͈͓�ɂ���Ƌ�̔ԍ��i�Ȃ����-1�j
	GHOST_STATE m_State;		// Ghost�̏��
	float m_DetectionTimer;		// ���m��Ԃ̃^�C�}�[�i1�b�ɂ��}�C�i�X1���邽�߁j
	float m_FloorCooldown;		// �K�i�ړ��̃N�[���^�C��
	bool m_IsTransformed;		// �ϐg���Ă��邩
	bool m_IsDetectedByBuster;	// bustar�ɔ������ꂽ��
	bool m_IsDraw;				// �`��t���O

public:
	Ghost(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
		: Sprite3D(pos, scale, rot, pass),
		m_Velocity(0.0f, 0.0f, 0.0f),
		m_InRangeFurnitureNum(-1),
		m_IsTransformed(false),
		m_IsDetectedByBuster(false),
		m_DetectionTimer(0.0f),
		m_FloorCooldown(),
		m_State(GS_MOVING),
		m_IsDraw(true)
	{
	}

	~Ghost() = default;

	// Sprite3D��Draw��I�[�o�[���C�h
	void Draw(void) override
	{
		if (m_IsDraw)
		{
			Sprite3D::Draw();
		}
	}

	// �Q�b�^�[
	XMFLOAT3 GetVelocity(void) const { return m_Velocity; }
	int GetInRangeNum(void) const { return m_InRangeFurnitureNum; }
	bool GetIsTransformed(void) const { return m_IsTransformed; }
	bool GetIsDetectedByBuster(void) const { return m_IsDetectedByBuster; }
	GHOST_STATE GetState(void) const { return m_State; }


	// �Z�b�^�[
	void SetVelocity(const XMFLOAT3& velocity) { m_Velocity = velocity; }
	void SetInRangeNum(int num) { m_InRangeFurnitureNum = num; }
	void SetIsTransformed(bool isTransformed) { m_IsTransformed = isTransformed; }
	void SetIsDetectedByBuster(bool isDetected) { m_IsDetectedByBuster = isDetected; }
	void SetState(GHOST_STATE state) { m_State = state; }
	void SetIsDraw(bool isDraw) { m_IsDraw = isDraw; }


	// ���J���\�b�h
	void FurnitureSearch(void);	// �Ƌ�m�ƐF�ύX
	void Transforming(void);	// �ϐg����
	void Move(void);            // �ړ�����
	void FloorMove(void);		// �K�i�ړ�����
	void ScareStart(void);		// �������J�n
	void ResetPos(void);		// ��ԃ��Z�b�g

	// �萔�A�N�Z�T
	static float GetDetectionRange(void) { return FURNITURE_DETECTION_RANGE; }
	static float GetGhostPosY(void) { return GHOST_POS_Y; }
};

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Ghost_Update(void);
void Ghost_Draw(void);
void Ghost_Finalize(void);

// Ghost�̃Q�b�^�[
Ghost* GetGhost(void);
