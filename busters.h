#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "sprite3d.h"
#include "anim_sprite3d.h"
#include "ghost.h"
#include "component.h"
#include "define.h"
#include "billboard.h"
#include "light.h"
#include <vector>
using namespace DirectX;

enum BUSTERS_STATE
{
	BUSTERS_SEARCH,         // 家具を探して移動（探索）
	BUSTERS_WAIT_RESELECT,  // スタック・経路失敗時の目的地再抽選待機
	BUSTERS_SUSPICION,      // 怪しんで近づく（警戒）
	BUSTERS_CHASE,          // 幽霊を見つけて追跡（確定）
	BUSTERS_STUN,           // 気絶中
	BUSTERS_LURED,          // 誘引中
	BUSTERS_RUN_TO_STAIRS,  // 階降下アニメーション：階段まで走る
};

class Busters : public AnimSprite3D, public Jump
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
	std::vector<int> m_IgnoredDoorIndices;

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

	// 階降下アニメーション用
	XMFLOAT3 m_StairsTargetPos;  // 目標の階段位置
	bool m_RunToStairsDone;       // 階段到着済みフラグ

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
	void StartRunToStairs(XMFLOAT3 stairsPos); // 階降下アニメ開始
	bool IsRunToStairsDone(void) const { return m_RunToStairsDone; }
	
	BUSTERS_STATE GetState(void) const { return m_State; }
	void ReduceWaitTimer(int amount) { m_WaitTimer -= amount; if (m_WaitTimer < 0) m_WaitTimer = 0; }

	PointLight* GetHeadlight(void) const { return m_pHeadlight; }
	void SetIsGhostDiscover(bool discover);
	bool IsTargetInFOV(const XMFLOAT3& targetPos, float range);
	void SetTutorial(bool tutorial) { m_IsTutorial = tutorial; }
	bool IsTutorial(void) const { return m_IsTutorial; }
	void AddIgnoreRelayDoor(int furnitureIndex); // 対象のドアを中継地点候補から外す
	void ClearIgnoreRelayDoors(void);            // 除外状態をすべてリセットする
	bool IsIgnoredRelayDoor(int furnitureIndex); // そのドアが除外されているか確認する

};

void Busters_Initialize(void);
void Busters_Update(void);
void Busters_Draw(void);
void Busters_Finalize(void);

Busters* GetBusters(void);
void BustersScare(void);
void BustersLured(const XMFLOAT3& pos, float radius);
void BustersStopped(void);

// ゲージMAX時の処理：trueならフロア降下アニメーション開始、falseなら勝利フェード済み
bool Busters_CheckGaugeEvent(void);

// 階段到着済みのバスターズを1体削除する（到着した順に削除）
// 戻り値: true=削除した / false=到着済みバスターズなし
bool Busters_DeleteFirstArrived(void);

void Busters_SetLight(void);

int Busters_GetCurrentFloorCount(void);
bool Busters_IsAnyInRange(const XMFLOAT3& pos, float range); // 指定座標からrange以内に1人でもいるか
Busters* GetBustersByIndex(int index); // インデックス指定でバスターズを取得

// フロア降下アニメーション用
void Busters_StartFloorExitAnim(void); // 全バスターズにID5/6階段まで走るよう指示（ランダム割り当て）
bool Busters_IsFloorExitAnimDone(void);               // 全バスターズが階段到着済みか
void Busters_DoFloorTransition(void);                 // 実際のフロア移行処理を実行
void Busters_DeleteCurrentFloor(void);                // 現在フロアのバスターズを全削除
void Busters_SpawnOnFloor(int floorIndex);            // 指定フロアにバスターズを生成
void Busters_SpawnAt(const XMFLOAT3& pos, int floorIndex); // 指定座標・フロアにバスターズを1体生成
