#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "sprite3d.h"
#include "ghost.h"
#include "component.h"
#include "define.h"
#include <vector>

using namespace DirectX;

enum BUSTERS_STATE
{
	BUSTERS_SEARCH,    // 家具を探して移動（探索）
	BUSTERS_SUSPICION, // 怪しんで近づく（警戒）
	BUSTERS_CHASE      // 幽霊を見つけて追跡（確定）
};

class Busters : public Sprite3D, public Jump
{
private:
	BUSTERS_STATE m_State;

	int m_TargetFurnitureIndex;
	int m_WaitTimer;

	std::vector<XMFLOAT3> m_PathList;

	XMFLOAT3 m_Velocity;
	float m_MoveSpeed;
	float m_DistanceToGhost;

public:
	Busters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass);
	~Busters() = default;

	void Update(void);
	void CheckState(void);
	void MoveTo(XMFLOAT3 targetPos);
	void OnScared(void);
	void OnLured(XMFLOAT3 targetPos); // おびき寄せられる
	void OnStopped(void);             // 足止めされる

	void SetIsGhostDiscover(bool discover);
};

void Busters_Initialize(void);
void Busters_Update(void);
void Busters_Draw(void);
void Busters_Finalize(void);

Busters* GetBusters(void);
void BustersScare(void);
void BustersLured(XMFLOAT3 pos);
void BustersStopped(void);