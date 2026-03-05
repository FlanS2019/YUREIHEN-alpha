#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "sprite3d.h"
#include "define.h"
#include "light.h"
#include <vector>
using namespace DirectX;

enum GHOST_STATE
{
	GS_MOVING,			// 移動
	GS_FURNITURE_FOUND,	// 家具発見
	GS_TRANSFORM,		// 変身中
	GS_SCARE,			// 驚かし中
	GS_CAUGHT,
};

// Ghost クラス
class Ghost : public Sprite3D
{
public:
	XMFLOAT3 m_Velocity;		// Ghost の速度ベクトル
	int m_InRangeFurnitureNum;	// 範囲内にいる家具の番号（いないなら-1）
	int m_InvincibleTimer;		// 無敵タイマー
	GHOST_STATE m_State;		// Ghost の状態
	float m_DetectionTimer;		// 発見状態のタイマー（1秒につきマイナス1するため）
	float m_FloorCooldown;		// 階段移動のクールタイム
	bool m_IsTransformed;		// 変身しているか
	bool m_IsDetectedByBuster;	// bustarに発見されたか
	bool m_IsDraw;				// 描画フラグ
	bool m_HasIncreasedMultiplier; // 倍率を加算したか

	XMFLOAT3 m_PreTransformPos;   // 変身直前のゴーストのワールド座標

	bool m_IsIlluminated;     // 現在ライトに照らされているか
	bool m_PrevIsIlluminated; // 前フレームで照らされていたか

	int m_EscapeTapCount;      // SPACEキーを連打した回数
	int m_CaughtPenaltyTimer;  // 毎秒ペナルティを与えるためのタイマー
	int m_ScareCooldown;       // 驚かしアクションのクールタイム

	void SetIsIlluminated(bool isIlluminated) { m_IsIlluminated = isIlluminated; }

	std::vector<int> m_InRangeFurnitureList; // 範囲内にある家具のリスト
	int m_SelectedFurnitureListIndex;        // リストの中で現在選んでいる番号

	Sprite3D* m_pRangeCircle;
	PointLight* m_pLight;


	Ghost(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
		: Sprite3D(pos, scale, rot, pass),
		m_Velocity(0.0f, 0.0f, 0.0f),
		m_InRangeFurnitureNum(-1),
		m_IsTransformed(false),
		m_IsDetectedByBuster(false),
		m_InvincibleTimer(0),
		m_FloorCooldown(),
		m_State(GS_MOVING),
		m_IsDraw(true),
		m_HasIncreasedMultiplier(false),
		m_IsIlluminated(false),
		m_PrevIsIlluminated(false),
		m_PreTransformPos(0.0f, 0.0f, 0.0f),
		m_EscapeTapCount(0),
		m_CaughtPenaltyTimer(0),
		m_ScareCooldown(0),
		m_pRangeCircle(nullptr),
		m_pLight(nullptr)
	{
	}

	virtual ~Ghost()
	{
		if (m_pRangeCircle)
		{
			delete m_pRangeCircle;
			m_pRangeCircle = nullptr;
		}

		if (m_pLight)
		{
			delete m_pLight;
			m_pLight = nullptr;
		}
	}

	// Sprite3DのDrawをオーバーライド
	void Draw(void) override
	{
		if (m_IsDraw)
		{
			Sprite3D::Draw();
		}
	}

	// ゲッター
	XMFLOAT3 GetVelocity(void) const { return m_Velocity; }
	int GetInRangeNum(void) const { return m_InRangeFurnitureNum; }
	bool GetIsTransformed(void) const { return m_IsTransformed; }
	bool GetIsDetectedByBuster(void) const { return m_IsDetectedByBuster; }
	GHOST_STATE GetState(void) const { return m_State; }
	bool IsInvincible() const { return m_InvincibleTimer > 0; }

	// セッター
	void SetVelocity(const XMFLOAT3& velocity) { m_Velocity = velocity; }
	void SetInRangeNum(int num) { m_InRangeFurnitureNum = num; }
	void SetIsTransformed(bool isTransformed) { m_IsTransformed = isTransformed; }
	void SetIsDetectedByBuster(bool isDetected) { m_IsDetectedByBuster = isDetected; }
	void SetState(GHOST_STATE state) { m_State = state; }
	void SetIsDraw(bool isDraw) { m_IsDraw = isDraw; }
	void SetInvincible(int frames) { m_InvincibleTimer = frames; }

	// 公開メソッド
	void FurnitureSearch(void);	// 家具検知と色変更
	void Transforming(void);	// 変身処理
	void Move(void);            // 移動処理
	void FloorMove(void);		// 階段移動処理
	void ScareStart(void);		// 驚かせ処理
	void ResetPos(void);		// 状態リセット


	// 定数アクセサ
	static float GetDetectionRange(void) { return FURNITURE_DETECTION_RANGE; }
	static float GetGhostPosY(void) { return GHOST_POS_Y; }
};

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Ghost_Update(void);
void Ghost_Draw(void);
void Ghost_Finalize(void);
void Ghost_SetLight(void);
void Ghost_ForceExitTransform(void);  // 変身を強制解除してGS_MOVINGに戻す

// Ghostのゲッター
Ghost* GetGhost(void);
XMFLOAT3 GetGhostStartPos(void);
XMFLOAT3 GetGhostStartPos(int floor);
