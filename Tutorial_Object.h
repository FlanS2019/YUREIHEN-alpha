/*==============================================================================
   チュートリアル用3Dモデル管理 [Tutorial_Object.h]
==============================================================================*/
#pragma once

#include <DirectXMath.h>
#include "sprite3d.h"
#include "anim_sprite3d.h"
#include "billboard.h"
#include "define.h"
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
class TutorialBusters : public AnimSprite3D
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
// 目的地マーカークラス
// 指定ワールド座標の頭上でビルボード（下向き矢印）がぴょんぴょんする
// =================================================================
class TutorialMarker
{
private:
	XMFLOAT3 m_BasePos;     // マーカーのワールド座標（地面位置）
	Billboard* m_Arrow;     // 下向き矢印ビルボード
	float m_BobTimer;       // バウンスアニメーション用タイマー
	bool  m_Visible;        // 表示フラグ

	// 定数は define.h のマクロを使用
	// TUTORIAL_MARKER_SIZE / TUTORIAL_MARKER_BOB_AMP
	// TUTORIAL_MARKER_BOB_SPEED / TUTORIAL_MARKER_BASE_HEIGHT

public:
	TutorialMarker();
	~TutorialMarker();

	void Initialize(const XMFLOAT3& pos);
	void Update(void);
	void Draw(void);

	void SetPos(const XMFLOAT3& pos);
	void SetVisible(bool visible) { m_Visible = visible; }
	bool GetVisible(void) const   { return m_Visible; }
};

// チュートリアル用オブジェクト（円盤等）の初期化・更新・描画・終了
void TutorialObject_Initialize(void);
void TutorialObject_Update(void);
void TutorialObject_Draw(void);
void TutorialObject_Finalize(void);

// 円盤接触フラグのポインタを返す
bool* TutorialObject_GetEnbanTouchedPtr(void);

// 円盤の表示フラグを設定する
void TutorialObject_SetEnbanVisible(bool visible);

// バスターズの表示フラグを設定する
void TutorialObject_SetBustersVisible(bool visible);

// ピアノ憑依フラグのポインタを返す
bool* TutorialObject_GetPianoPossessedPtr(void);

TutorialBusters* GetTutorialBusters(void);

// 目的地マーカーのゲッター
TutorialMarker* GetTutorialMarker(void);
