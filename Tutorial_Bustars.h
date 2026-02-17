#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "sprite3d.h"
#include "billboard.h"
using namespace DirectX;

// =================================================================
// チュートリアル用バスターズの状態
// =================================================================
enum TUTORIAL_BUSTERS_STATE
{
	TB_IDLE,		// 待機中（何もしない）
	TB_SUSPICION,	// 警戒中（？アイコン）
	TB_CHASE,		// 追跡中（！アイコン）
	TB_STUN,		// 気絶中（星アイコン）
};

// =================================================================
// チュートリアル用バスターズクラス
// 最低限の状態表示（色変え）とビルボードアイコンのみ
// =================================================================
class TutorialBusters : public Sprite3D
{
private:
	TUTORIAL_BUSTERS_STATE m_State;
	Billboard* m_Icon;

public:
	TutorialBusters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass);
	~TutorialBusters();

	void Update(void);
	void Draw(void) override;

	// 状態を設定（色・アイコンも自動で切り替わる）
	void SetState(TUTORIAL_BUSTERS_STATE state);
	TUTORIAL_BUSTERS_STATE GetState(void) const { return m_State; }
};

// =================================================================
// グローバル関数
// =================================================================
void TutorialBusters_Initialize(const XMFLOAT3& pos);
void TutorialBusters_Update(void);
void TutorialBusters_Draw(void);
void TutorialBusters_Finalize(void);

TutorialBusters* GetTutorialBusters(void);
