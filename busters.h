#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "sprite3d.h"
#include "ghost.h"
#include "component.h"
#include "define.h"
#include "billboard.h"
#include "light.h"
#include <vector>
using namespace DirectX;

enum BUSTERS_STATE
{
	BUSTERS_SEARCH,    // 家具を探して移動（探索）
	BUSTERS_SUSPICION, // 怪しんで近づく（警戒）
	BUSTERS_CHASE,      // 幽霊を見つけて追跡（確定）
	BUSTERS_STUN,		//気絶中
	BUSTERS_LURED		// 誘引中

};

class Busters : public Sprite3D, public Jump
{
private:
	BUSTERS_STATE m_State;

	int m_TargetFurnitureIndex;
	int m_WaitTimer;
	int m_DetectionGraceTimer;
	int m_KeepStateTimer;
	int m_ReactionCooldown;
	int m_PathUpdateTimer;

	std::vector<XMFLOAT3> m_PathList;

	XMFLOAT3 m_Velocity;
	float m_MoveSpeed;
	float m_DistanceToGhost;

	Billboard* m_Icon;
	PointLight* m_pHeadlight;
	XMFLOAT3 m_LureTargetPos;
	bool m_HasLureTarget;
	int m_LureStayTimer;
	bool m_IsGhostDiscover;
	bool m_IsTutorial;

	XMFLOAT3 m_LastPathCalcGhostPos;
	XMFLOAT3 m_PrevPos; // 前フレームの座標
	int m_StuckTimer;   // 動いていない時間を計測

public:
	Busters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass);
	~Busters();

	void Update(void);
	void Draw(void) override;
	void CheckState(void);
	void MoveTo(XMFLOAT3 targetPos);
	void OnScared(void);
	void OnLured(XMFLOAT3 targetPos);
	void OnStopped(void);
	
	PointLight* GetHeadlight(void) const { return m_pHeadlight; }
	void SetIsGhostDiscover(bool discover);
	bool IsTargetInFOV(const XMFLOAT3& targetPos, float range);
	void SetTutorial(bool tutorial) { m_IsTutorial = tutorial; }
	bool IsTutorial(void) const { return m_IsTutorial; }
};

void Busters_Initialize(void);
void Busters_Update(void);
void Busters_Draw(void);
void Busters_Finalize(void);

Busters* GetBusters(void);
void BustersScare(void);
void BustersLured(const XMFLOAT3& pos, float radius);
void BustersStopped(void);
void Busters_CheckGaugeEvent(void);

void Busters_SetLight(void);
