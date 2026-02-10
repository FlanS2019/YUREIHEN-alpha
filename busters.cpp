#include "busters.h"
#include "billboard.h"
#include "Camera.h"
#include "debug_ostream.h"
#include "define.h"
#include "fade.h"
#include "ghost.h"
#include "shader.h"
#include "keyboard.h"
#include "sprite3d.h"

#include "field.h"
#include "furniture.h"
#include <stdlib.h>
#include <algorithm>
#include <vector>

#include "Floor1.h"
#include "Floor2.h"
#include "Floor3.h"

#include "UI.h"
#include "UI_scarecombo.h"
#include "scene.h"

static std::vector<Busters*> g_BustersList[MAP_FLOORS];

// =================================================================
// Busters クラスメンバ関数の実装
// =================================================================

Busters::Busters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
	: Sprite3D(pos, scale, rot, pass),
	Jump(0.01f, 0.2f, PATROL_HEIGHT),
	m_State(BUSTERS_SEARCH),
	m_TargetFurnitureIndex(-1),
	m_WaitTimer(0),
	m_DetectionGraceTimer(0),
	m_Velocity(0.0f, 0.0f, 0.0f),
	m_MoveSpeed(BUSTERS_MOVE_SPEED_SEARCH),
	m_DistanceToGhost(0.0f),
	m_ReactionCooldown(0),
	m_KeepStateTimer(0),
	m_Icon(nullptr),
	m_pHeadlight(nullptr),
	m_LureTargetPos(pos),
	m_HasLureTarget(false),
	m_LureStayTimer(0),
	m_IsGhostDiscover(false)

{
	m_Icon = new Billboard();
	m_LastPathCalcGhostPos = { -999.0f, 0.0f, -999.0f }; 
	m_Icon->Initialize({ 0.0f, 0.0f, 0.0f }, { 0.7f, 0.7f }, { 0.0f, 0.0f, 0.0f }, true);
	m_PathUpdateTimer = rand() % 15;
	m_PrevPos = pos;
	m_StuckTimer = 0;

	// ヘッドライト初期化
	m_pHeadlight = new PointLight(
		TRUE,
		XMFLOAT4(pos.x, pos.y + 1.6f, pos.z, 1.0f),	// 頭部位置
		XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f),				// 方向（ダミー）
		XMFLOAT4(1.0f, 1.0f, 0.95f, 1.0f),				// 白～微黄色
		10.0f,											// 範囲
		2.0f											// 強度
	);
}

Busters::~Busters()
{
	if (m_Icon)
	{
		delete m_Icon;
		m_Icon = nullptr;
	}

	if (m_pHeadlight)
	{
		delete m_pHeadlight;
		m_pHeadlight = nullptr;
	}
}

void Busters::Update(void)
{
	JumpUpdate(*(Transform3D*)this);

	if (m_DetectionGraceTimer > 0)
	{
		m_DetectionGraceTimer--;
	}

	if (m_ReactionCooldown > 0)
	{
		m_ReactionCooldown--;
	}

	// ヘッドライトの位置を頭部に追従させる
	if (m_pHeadlight)
	{
		XMFLOAT3 headPos = m_Position;
		headPos.y += 2.0f;	// 頭部の高さ

		float rotY = GetRot().y;
		float radY = XMConvertToRadians(rotY);

		// 方向計算 (モデルに合わせて調整)
		float dirX = sinf(radY);
		float dirZ = cosf(radY);

		// 位置オフセット
		headPos.x += dirX * 0.8f;
		headPos.z += dirZ * 0.8f;

		m_pHeadlight->SetPosition(headPos.x, headPos.y, headPos.z);

		m_pHeadlight->SetDirection(XMFLOAT4(dirX, -0.3f, dirZ, 0.0f));

		float range = 10.0f;
		XMFLOAT4 color = { 1.0f, 1.0f, 0.95f, 1.0f }; // 固定色（白）

		switch (m_State)
		{
		case BUSTERS_SEARCH:    // 探索
			range = 15.0f;
			break;

		case BUSTERS_SUSPICION: // 警戒
			range = BUSTERS_SUSPICION_RANGE;
			break;

		case BUSTERS_CHASE:     // 追跡
			range = BUSTERS_PATROL_RANGH;
			break;

		case BUSTERS_LURED:     // 誘引
			range = BUSTERS_SUSPICION_RANGE;
			break;

		case BUSTERS_STUN:      // 気絶
			range = 5.0f;
			break;
		}

		m_pHeadlight->SetRange(range);
		m_pHeadlight->SetDiffuse(color);
	}

	// アイコンの状態更新
	if (m_Icon)
	{
		switch (m_State)
		{
		case BUSTERS_SEARCH:

			if (m_WaitTimer > 0)
			{
				// 調査中（tyousa.png）
				m_Icon->SetIcon(BILLBOARD_ICON::CHECK);

			}
			else
			{
				// 探索中（tansaku.png）
				m_Icon->SetIcon(BILLBOARD_ICON::SEARCH);
			}
		break;		case BUSTERS_SUSPICION: m_Icon->SetIcon(BILLBOARD_ICON::QUESTION); break;
		case BUSTERS_LURED:		m_Icon->SetIcon(BILLBOARD_ICON::QUESTION); break;
		case BUSTERS_CHASE:     m_Icon->SetIcon(BILLBOARD_ICON::ALERT); break;
		case BUSTERS_STUN:      m_Icon->SetIcon(BILLBOARD_ICON::STUN); break;

		}

		XMFLOAT3 iconPos = m_Position;
		iconPos.y += 3.25f;
		m_Icon->SetPos(iconPos);
		m_Icon->Update();
	}

	if (m_WaitTimer > 0)
	{
		m_WaitTimer--;
		if (m_State == BUSTERS_SUSPICION || m_State == BUSTERS_CHASE)
		{
			if (m_State == BUSTERS_SEARCH && m_TargetFurnitureIndex != -1)
			{
				Furniture* target = GetFurniture(m_TargetFurnitureIndex);
				if (target)
				{
					float dx = target->GetPos().x - m_Position.x;
					float dz = target->GetPos().z - m_Position.z;
					float deg = XMConvertToDegrees(atan2f(dx, dz));
					SetRotY(deg);
				}
			}

			Ghost* ghost = GetGhost();
			if (ghost)
			{
				float dx = ghost->GetPos().x - m_Position.x;
				float dz = ghost->GetPos().z - m_Position.z;
				float deg = XMConvertToDegrees(atan2f(dx, dz));
				SetRotY(deg);
			}
		}
		return;
	}

	CheckState();

	XMFLOAT3 nextStepPos = m_Position;

	switch (m_State)
	{
	case BUSTERS_SEARCH: // 探索
		if (m_TargetFurnitureIndex == -1)
		{
			m_TargetFurnitureIndex = rand() % FURNITURE_NUM;
			Furniture* targetFurniture = GetFurniture(m_TargetFurnitureIndex);
			if (targetFurniture)
			{
				XMFLOAT3 targetPos = targetFurniture->GetPos();

				// 現在地から家具への方向ベクトル
				float dx = targetPos.x - m_Position.x;
				float dz = targetPos.z - m_Position.z;
				float dist = sqrtf(dx * dx + dz * dz);

				if (dist > 0.0f)
				{
					// 0.8m手前を目的地にする（家具の半径＋バスターズの半径分）
					float offset = 0.6f;
					targetPos.x -= (dx / dist) * offset;
					targetPos.z -= (dz / dist) * offset;
				}

				m_PathList = Field_FindPath(m_Position, targetPos);

				if (m_PathList.empty() && targetFurniture)
				{
					m_PathList.push_back(targetFurniture->GetPos());
				}

				if (m_PathList.empty())
				{
					m_TargetFurnitureIndex = -1;
					m_WaitTimer = 300;
				}
			}
			else
			{
				m_TargetFurnitureIndex = -1;
			}
		}
		m_MoveSpeed = BUSTERS_MOVE_SPEED_SEARCH;
		break;

	case BUSTERS_LURED: // 誘引
		m_MoveSpeed = BUSTERS_MOVE_SPEED_SUSPICION;
		if (!m_HasLureTarget)
		{
			if (m_LureStayTimer > 0)
			{
				m_LureStayTimer--;
				return;
			}
			m_State = BUSTERS_SEARCH;
			m_TargetFurnitureIndex = -1;
			m_WaitTimer = 0;
			m_PathList.clear();
			break;
		}
		nextStepPos = m_LureTargetPos;
		break;

	case BUSTERS_SUSPICION: // 警戒
	case BUSTERS_CHASE:     // 追跡
		if (GetGhost())
		{
			float dx = GetGhost()->GetPos().x - m_Position.x;
			float dz = GetGhost()->GetPos().z - m_Position.z;
			float dist = sqrtf(dx * dx + dz * dz);
			bool hasWall = Field_CheckWallBetween(m_Position, GetGhost()->GetPos());

			// 壁がなく、かつ距離が近い場合
			if (!hasWall && dist < 0.5f)
			{
				m_PathList.clear();
				nextStepPos = GetGhost()->GetPos();
			}
			else
			{

				m_PathUpdateTimer++;

				// 現在のゴーストの位置
				XMFLOAT3 ghostPos = GetGhost()->GetPos();

				// ターゲット（ゴースト）が前回計算時からどれくらい動いたか？
				float dxG = ghostPos.x - m_LastPathCalcGhostPos.x;
				float dzG = ghostPos.z - m_LastPathCalcGhostPos.z;
				float distGhostMovedSq = dxG * dxG + dzG * dzG;

				// 自分とゴーストの距離に応じて、更新頻度（インターバル）を変える
				int updateInterval = 15;
				if (dist > 20.0f) updateInterval = 60;      // 20m以上離れていれば1秒に1回
				else if (dist > 10.0f) updateInterval = 30; // 10m以上なら0.5秒に1回

				// 計算するかどうかの判定
				// 「経路がない」または「インターバル経過 かつ 敵が一定以上動いた」場合のみ
				bool shouldUpdate = false;
				if (m_PathList.empty())
				{
					shouldUpdate = true;
				}
				else if (m_PathUpdateTimer > updateInterval && distGhostMovedSq > 0.5f * 0.5f)
				{
					shouldUpdate = true;
				}

				if (shouldUpdate)
				{
					m_PathUpdateTimer = 0; // タイマーリセット
					m_LastPathCalcGhostPos = ghostPos; // 計算位置を記憶

					m_PathList = Field_FindPath(m_Position, ghostPos);

					if (!m_PathList.empty()) {
						std::reverse(m_PathList.begin(), m_PathList.end());

						if (m_PathList.size() > 0) m_PathList.erase(m_PathList.begin());
					}
				}
			}
		}

		m_MoveSpeed = (m_State == BUSTERS_CHASE) ? BUSTERS_MOVE_SPEED_CHASE : BUSTERS_MOVE_SPEED_SUSPICION;
		break;
	}

	// 経路移動
	if (!m_PathList.empty())
	{
		nextStepPos = m_PathList[0];
		float dx = nextStepPos.x - m_Position.x;
		float dz = nextStepPos.z - m_Position.z;
		float distSq = dx * dx + dz * dz;

		float arriveThreshold = 0.5f;
		if (m_PathList.size() == 1) arriveThreshold = 1.0f;

		if (distSq < arriveThreshold * arriveThreshold)
		{
			m_PathList.erase(m_PathList.begin());
			if (m_PathList.empty())
			{
				if (m_State == BUSTERS_SEARCH || m_State == BUSTERS_LURED)
				{
					m_State = BUSTERS_SEARCH;
					m_TargetFurnitureIndex = -1;
					m_WaitTimer = 300;
				}
			}
		}
	}

	if (m_State == BUSTERS_LURED && m_HasLureTarget)
	{
		float dx = m_LureTargetPos.x - m_Position.x;
		float dz = m_LureTargetPos.z - m_Position.z;
		float distSq = dx * dx + dz * dz;
		if (distSq <= 0.04f)
		{
			m_HasLureTarget = false;
			m_LureStayTimer = BUSTERS_LURE_STAY_FRAMES;
		}
	}

	MoveTo(nextStepPos);

	if (m_State == BUSTERS_SEARCH && m_TargetFurnitureIndex != -1)
	{
		Furniture* target = GetFurniture(m_TargetFurnitureIndex);
		if (target)
		{
			// フラグを立てる
			target->SetIsTargeted(true);
		}
	}

	if (m_State == BUSTERS_SEARCH && !m_PathList.empty())
	{
		float dx = m_Position.x - m_PrevPos.x;
		float dz = m_Position.z - m_PrevPos.z;
		float movedDistSq = dx * dx + dz * dz;

		// ほとんど動いていない (0.01cm以下)
		if (movedDistSq < 0.0001f)
		{
			m_StuckTimer++;
			// 60フレーム（約1秒）以上動けなかったらスタックとみなす
			if (m_StuckTimer > 60)
			{
				m_PathList.clear(); // 経路破棄して再計算へ
				m_StuckTimer = 0;
				m_TargetFurnitureIndex = -1; // ターゲットを変える（諦める）
			}
		}
		else
		{
			m_StuckTimer = 0;
		}
	}
	m_PrevPos = m_Position;

	// 捕獲判定
	if (m_State == BUSTERS_CHASE && GetGhost())
	{
		XMFLOAT3 gPos = GetGhost()->GetPos();
		float dx = gPos.x - m_Position.x;
		float dy = gPos.y - m_Position.y;
		float dz = gPos.z - m_Position.z;
		float dist = sqrtf(dx * dx + dy * dy + dz * dz);

		if (dist < 1.0f)
		{
			StartFade(SCENE_ANM_LOSE);
		}
	}
}void

Busters::CheckState(void)
{
	if (m_State == BUSTERS_LURED)
	{
		return;
	}

	Ghost* ghost = GetGhost();
	if (!ghost) return;
	if (m_DetectionGraceTimer > 0) return;

	// 変身中は見つからない処理
	if (ghost->GetState() == GS_TRANSFORM || ghost->GetState() == GS_SCARE)
	{
		// 見失う
		if (m_State != BUSTERS_SEARCH && m_State != BUSTERS_LURED)
		{
			m_State = BUSTERS_SEARCH;
			this->ResetColor();
			ghost->SetIsDetectedByBuster(false);
			m_TargetFurnitureIndex = -1;
			m_PathList.clear();
			m_WaitTimer = 60;
		}
		return;
	}

	// 壁チェック
	bool hasWall = Field_CheckWallBetween(m_Position, ghost->GetPos());


	// 判定範囲の決定（ヒステリシス付き）
	float chaseRange = BUSTERS_PATROL_RANGH;
	if (m_State == BUSTERS_CHASE) chaseRange *= 1.2f;

	float suspicionRange = BUSTERS_SUSPICION_RANGE;
	if (m_State == BUSTERS_SUSPICION) suspicionRange *= 1.2f;

	// 視野判定を実行
	bool inChaseArea = IsTargetInFOV(ghost->GetPos(), chaseRange);
	bool inSuspicionArea = IsTargetInFOV(ghost->GetPos(), suspicionRange);


	// --- 状態遷移ロジック ---

	// 優先度高：追跡範囲 (壁なし かつ 視野内)
	if (!hasWall && inChaseArea)
	{
		m_KeepStateTimer = KEEP_STATE_TIME; // 見失い防止タイマー更新

		if (m_State != BUSTERS_CHASE)
		{
			m_State = BUSTERS_CHASE;
			this->SetColor(1.0f, 0.0f, 0.0f, 1.0f); // 赤
			ghost->SetIsDetectedByBuster(true);

			// 初回発見時の硬直処理
			if (m_ReactionCooldown <= 0)
			{
				m_WaitTimer = WAIT_TIMER_DEFAULT;
				m_ReactionCooldown = WAIT_TIMER_COOLDOWN;
			}
			else
			{
				m_WaitTimer = 0;
			}
		}

		// 接近時のゲージ減少処理
		if (m_DistanceToGhost < BUSTERS_PATROL_RANGH)
		{
			float reduceAmount = (1.0f - (m_DistanceToGhost / BUSTERS_PATROL_RANGH)) * -BUSTER_GAUGE_REDUCTION;
			AddScareGauge(reduceAmount);
		}
	}
	// 優先度中：警戒範囲 (壁なし かつ 視野内)
	else if (!hasWall && inSuspicionArea)
	{
		m_KeepStateTimer = KEEP_STATE_TIME;

		if (m_State != BUSTERS_SUSPICION)
		{
			m_State = BUSTERS_SUSPICION;
			this->SetColor(1.0f, 1.0f, 0.0f, 1.0f); // 黄
			ghost->SetIsDetectedByBuster(false);

			if (m_ReactionCooldown <= 0)
			{
				m_WaitTimer = WAIT_TIMER_DEFAULT;
				m_ReactionCooldown = WAIT_TIMER_COOLDOWN;
			}
			else
			{
				m_WaitTimer = 0;
			}
		}
	}
	// 範囲外 or 壁あり or 後ろ側
	else
	{
		// 見失い猶予中
		if (m_KeepStateTimer > 0)
		{
			m_KeepStateTimer--;
			return; // 状態維持
		}

		// 完全に見失った -> 探索に戻る
		if (m_State != BUSTERS_SEARCH)
		{
			m_State = BUSTERS_SEARCH;
			this->ResetColor();
			ghost->SetIsDetectedByBuster(false);
			m_TargetFurnitureIndex = -1;
			m_PathList.clear(); // 経路リセット
			m_WaitTimer = 30;
		}
	}
}

void Busters::MoveTo(XMFLOAT3 targetPos)
{
	if (GetIsJumping()) return;

	float dx = targetPos.x - m_Position.x;
	float dz = targetPos.z - m_Position.z;

	if (fabsf(dx) < m_MoveSpeed && fabsf(dz) < m_MoveSpeed) return;

	float len = sqrtf(dx * dx + dz * dz);
	if (len > 0)
	{
		dx /= len;
		dz /= len;
	}

	// 向き変更
	float angle = atan2f(dx, dz);
	float deg = XMConvertToDegrees(angle);
	SetRotY(deg + 180.0f);

	// 当たり判定付き移動（スライド移動）
	auto checkWall = [&](float nx, float nz) -> bool {
		float r = 0.2f;
		return (Field_IsWall(nx + r, m_Position.y, nz + r) ||
			Field_IsWall(nx + r, m_Position.y, nz - r) ||
			Field_IsWall(nx - r, m_Position.y, nz + r) ||
			Field_IsWall(nx - r, m_Position.y, nz - r));
		};

	float nextX = m_Position.x + dx * m_MoveSpeed;
	if (!checkWall(nextX, m_Position.z)) m_Position.x = nextX;

	float nextZ = m_Position.z + dz * m_MoveSpeed;
	if (!checkWall(m_Position.x, nextZ)) m_Position.z = nextZ;
}

void Busters::OnScared(void)
{
	JumpStart();
	m_TargetFurnitureIndex = -1;
	m_WaitTimer = 120;
	this->SetColor(0.0f, 0.0f, 1.0f, 1.0f); // 青

	m_State = BUSTERS_STUN;
}

void Busters::OnLured(XMFLOAT3 targetPos)
{
    m_State = BUSTERS_LURED;
    this->SetColor(0.0f, 1.0f, 1.0f, 1.0f); // シアン
    m_WaitTimer = 0;
    m_DetectionGraceTimer = BUSTERS_LURE_STAY_FRAMES;
    m_PathList.clear();
    m_HasLureTarget = true;
    m_LureTargetPos = targetPos;
    m_LureStayTimer = 0;
}

void Busters::OnStopped(void)
{
	m_WaitTimer = 300;
	this->SetColor(0.5f, 0.0f, 0.5f, 1.0f); // 紫

	m_State = BUSTERS_STUN;
}

void Busters::SetIsGhostDiscover(bool discover)
{
    if (m_State != BUSTERS_SEARCH)
    {
        if (m_IsGhostDiscover)
        {
            m_IsGhostDiscover = false;
        }
        return;
    }

    if (discover)
    {
        m_IsGhostDiscover = true;
        this->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else if (m_IsGhostDiscover)
    {
        m_IsGhostDiscover = false;
        this->ResetColor();
    }
}

bool Busters::IsTargetInFOV(const XMFLOAT3& targetPos, float range)
{
	// 距離チェック
	XMVECTOR myPosVec = XMLoadFloat3(&m_Position);
	XMVECTOR targetPosVec = XMLoadFloat3(&targetPos);
	XMVECTOR dirVec = XMVectorSubtract(targetPosVec, myPosVec);
	float distSq = XMVectorGetX(XMVector3LengthSq(dirVec)); // 距離の2乗

	if (distSq > range * range) return false; // 範囲外

	// 角度チェック (内積)
	// バスターズの正面ベクトル
	// MoveToで `atan2 + 180` しているので、正面は `rotY + 180` の方向
	float rotRad = XMConvertToRadians(GetRot().y + 180.0f);
	XMVECTOR forwardVec = XMVectorSet(sinf(rotRad), 0.0f, cosf(rotRad), 0.0f);

	// ターゲットへの方向ベクトル（XZ平面）
	dirVec = XMVectorSet(targetPos.x - m_Position.x, 0.0f, targetPos.z - m_Position.z, 0.0f);
	dirVec = XMVector3Normalize(dirVec);

	// 内積計算
	float dot = XMVectorGetX(XMVector3Dot(forwardVec, dirVec));

	// 閾値計算 (視野角の半分)
	float limitCos = cosf(XMConvertToRadians(BUSTERS_FOV_ANGLE / 2.0f));

	// 内積が閾値より大きければ視野内（cosは角度が小さいほど1に近い）
	if (dot >= limitCos)
	{
		return true;
	}

	return false; // 視野角の外
}

XMFLOAT3 GetRandomBusterPos(int floor)
{
	int attempts = 0;
	// 最大100回トライして、壁じゃない場所を探す
	while (attempts < 100)
	{
		int gx = 0;
		int gz = rand() % MAP_LENGTH; // Zは全域ランダム

		if (floor == 2)
		{
			// 3階(Index 2)の場合: 真ん中(MAP_WIDTH/2)より左側(0に近い方)に制限
			gx = rand() % (MAP_WIDTH / 2);
		}
		else
		{
			// それ以外: 全域ランダム
			gx = rand() % MAP_WIDTH;
		}

		// 壁判定 (Y=1 のレイヤーを確認)
		int blockID = 0;
		switch (floor)
		{
		case 0: blockID = Floor1[1][gz][gx]; break;
		case 1: blockID = Floor2[1][gz][gx]; break;
		case 2: blockID = Floor3[1][gz][gx]; break;
		}

		// IDが0 (空気) ならスポーンOK
		if (blockID == 0)
		{
			// グリッド座標 -> ワールド座標 変換
			// field.cpp の計算式に合わせる: (x - W/2, z - H/2)
			float wx = (float)gx - MAP_WIDTH / 2.0f;
			float wz = MAP_LENGTH / 2.0f - (float)gz;

			return { wx, PATROL_HEIGHT, wz };
		}
		attempts++;
	}

	// 見つからなかった場合の安全策 (原点)
	return { 0.0f, PATROL_HEIGHT, 0.0f };
}

void DrawDebugFan(const XMFLOAT3& center, float rotY, float fovAngle, float range, const XMFLOAT4& color)
{
	// 簡易的な頂点構造体
	struct DebugVertex {
		XMFLOAT3 pos;
		XMFLOAT4 col;
	};

	ID3D11Device* pDevice = Direct3D_GetDevice();
	ID3D11DeviceContext* pContext = Direct3D_GetDeviceContext();

	// ラインの頂点を作成
	std::vector<DebugVertex> vertices;
	XMFLOAT3 startPos = { center.x, center.y + 0.1f, center.z }; // 少し浮かせる

	// 角度計算 (Degree -> Radian)
	float radY = XMConvertToRadians(rotY);
	float halfFovRad = XMConvertToRadians(fovAngle / 2.0f);

	// 扇形の解像度（分割数）
	const int segments = 10;

	// 左端から右端までラインを引く
	for (int i = 0; i <= segments; i++)
	{
		float progress = (float)i / segments; // 0.0 ～ 1.0
		float currentAngle = (radY + XM_PI) - halfFovRad + (halfFovRad * 2.0f * progress);
		// ※ +XM_PI (180度) はモデルの向き（MoveToで+180している仕様）に合わせる補正

		// 終点計算
		float dx = sinf(currentAngle);
		float dz = cosf(currentAngle);
		XMFLOAT3 endPos = {
			startPos.x + dx * range,
			startPos.y,
			startPos.z + dz * range
		};

		// 中心から外周への線
		if (i == 0 || i == segments) // 両端のみ描画（全部描くとくどいため）
		{
			vertices.push_back({ startPos, color });
			vertices.push_back({ endPos, color });
		}

		// 外周の弧を描画（ひとつ前の点と結ぶ）
		if (i > 0)
		{
			float prevAngle = (radY + XM_PI) - halfFovRad + (halfFovRad * 2.0f * ((float)(i - 1) / segments));
			XMFLOAT3 prevEndPos = {
				startPos.x + sinf(prevAngle) * range,
				startPos.y,
				startPos.z + cosf(prevAngle) * range
			};
			vertices.push_back({ prevEndPos, color });
			vertices.push_back({ endPos, color });
		}
	}

	// 頂点バッファ作成・描画（一時的）
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(DebugVertex) * (UINT)vertices.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices.data();

	ID3D11Buffer* pVertexBuffer = nullptr;
	if (SUCCEEDED(pDevice->CreateBuffer(&bd, &initData, &pVertexBuffer)))
	{
		// シェーダー設定（既存の3D描画用設定を利用すると仮定）
		// 本来は専用シェーダーを使うべきですが、ここでは色情報を含む頂点として描画
		UINT stride = sizeof(DebugVertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		// 描画（マテリアルカラーを無視させるためシェーダー設定が必要な場合あり）
		// ここでは描画発行のみ
		pContext->Draw((UINT)vertices.size(), 0);

		pVertexBuffer->Release();
	}
}
// =================================================================
// グローバル関数
// =================================================================

void Busters_Initialize(void)
{

	srand((unsigned int)time(NULL));

	// 既存リストのクリア
	for (int i = 0; i < MAP_FLOORS; i++)
	{
		for (Busters* buster : g_BustersList[i]) {
			delete buster;
		}
		g_BustersList[i].clear();
	}

	// 1階 (Floor 0)
	{
		XMFLOAT3 pos = GetRandomBusterPos(0);
		Busters* b = new Busters(pos, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\Busters_karikansei_3.fbx");
		if (b) {
			b->SetGroundLevel(PATROL_HEIGHT);
			g_BustersList[0].push_back(b);
		}
	}

	// 2階 (Floor 1)
	{
		XMFLOAT3 pos = GetRandomBusterPos(1);
		Busters* b = new Busters(pos, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\Busters_karikansei_3.fbx");
		if (b) {
			b->SetGroundLevel(PATROL_HEIGHT);
			g_BustersList[1].push_back(b);
		}
	}

	// 3階 (Floor 2)
	{
		XMFLOAT3 pos = GetRandomBusterPos(2);
		Busters* b = new Busters(pos, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\Busters_karikansei_3.fbx");
		if (b) {
			b->SetGroundLevel(PATROL_HEIGHT);
			g_BustersList[2].push_back(b);
		}
	}
}

void Busters_Update(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
	{
		for (Busters* buster : g_BustersList[currentFloor])
		{
			buster->Update();
		}
	}
}

void Busters_Draw(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
	{
		for (Busters* buster : g_BustersList[currentFloor])
		{
			buster->Draw();
		}
	}
}

void Busters_Finalize(void)
{
	for (int i = 0; i < MAP_FLOORS; i++)
	{
		for (Busters* buster : g_BustersList[i])
		{
			delete buster;
		}
		g_BustersList[i].clear();
	}
}

Busters* GetBusters(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
	{
		if (!g_BustersList[currentFloor].empty()) {
			return g_BustersList[currentFloor][0];
		}
	}
	return NULL;
}

void BustersScare(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
	{
		for (Busters* buster : g_BustersList[currentFloor]) {
			buster->OnScared();
		}
	}
}

void BustersLured(const XMFLOAT3& pos, float radius)
{
    if (radius <= 0.0f)
    {
        return;
    }

    float radiusSq = radius * radius;
    int currentFloor = Field_GetCurrentFloor();
    if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
    {
        for (Busters* buster : g_BustersList[currentFloor])
        {
            XMFLOAT3 busterPos = buster->GetPos();
            float dx = busterPos.x - pos.x;
            float dz = busterPos.z - pos.z;
            float distSq = dx * dx + dz * dz;
            if (distSq <= radiusSq)
            {
                buster->OnLured(pos);
            }
        }
    }
}

void BustersStopped(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
	{
		for (Busters* buster : g_BustersList[currentFloor]) {
			buster->OnStopped();
		}
	}
}

// =================================================================
// ゲージMAX時の処理
// =================================================================
void Busters_CheckGaugeEvent(void)
{
	if (!UI_IsScareGaugeMax()) return;

	int currentFloor = Field_GetCurrentFloor();

	if (currentFloor == END_FLOOR - 1)
	{
		// -------------------------------------------------
		// クリア階（3階）の場合 -> ゲーム勝利
		// -------------------------------------------------
		StartFade(SCENE_ANM_WIN);
	}
	else if (currentFloor > 0)
	{
		// -------------------------------------------------
		// 2階以上の場合 -> 下の階へ逃げる
		// -------------------------------------------------

		// ゲージをリセット
		UI_ResetScareGauge();

		// 0.0以下で敗北になるのでとりあえず回復
		AddScareGauge(BUSTERS_DEFOURT_GAUGE);

		int nextFloor = currentFloor - 1;

		// 階層に応じて増やす人数を決める
		int addCount = 0;
		if (nextFloor == 1) addCount = 1; // 2階へ行くとき： +1人
		if (nextFloor == 0) addCount = 2; // 1階へ行くとき： +2人

		for (Busters* buster : g_BustersList[currentFloor])
		{
			// 下の階のランダムな位置へ移動
			XMFLOAT3 newPos = GetRandomBusterPos(nextFloor);
			buster->SetPos(newPos);

			// 下の階のリストに追加
			g_BustersList[nextFloor].push_back(buster);
		}
		// 現在の階のバスターズを空に
		g_BustersList[currentFloor].clear();

		for (int i = 0; i < addCount; i++)
		{
			if (nextFloor >= 0 && nextFloor < MAP_FLOORS)
			{

				float offsetX = (float)(rand() % 400 - 200) / 100.0f;
				float offsetZ = (float)(rand() % 400 - 200) / 100.0f;

				Busters* newBuster = new Busters(
					{ offsetX, PATROL_HEIGHT, offsetZ },
					{ 1.0f, 1.0f, 1.0f },
					{ 0.0f, 0.0f, 0.0f },
					"asset\\model\\Busters_karikansei_3.fbx"
				);

				if (newBuster) {
					newBuster->SetGroundLevel(PATROL_HEIGHT);
					g_BustersList[nextFloor].push_back(newBuster);
				}
			}
		}

		// 幽霊も下の階へ自動移動
		Ghost* ghost = GetGhost();
		if (ghost)
		{
			// 現在の幽霊の位置をそのまま保持して下の階に移動
			XMFLOAT3 ghostPos = ghost->GetPos();
			Field_ChangeFloor(nextFloor);
			ghost->SetPos(ghostPos);
		}

		// 下の階へ移動
		//Field_ChangeFloor(nextFloor);
	}
	else
	{
		// -------------------------------------------------
		// 1階の場合 -> 逃げ場なし（プレイヤーの勝利）
		// -------------------------------------------------
		StartFade(SCENE_ANM_WIN);
	}
}
void Busters::Draw(void)
{
	// バスターズ（ボーンあり）を描画

	Sprite3D::Draw();

	if (m_Icon)
	{
		// ここで通常のシェーダー（ボーン無し）に戻す
		Shader_Begin();

		// ビルボード描画
		m_Icon->Draw();
	}

}

// =================================================================
// バスターズのライト設定
// =================================================================
void Busters_SetLight(void)
{
    int currentFloor = Field_GetCurrentFloor();
    if (currentFloor < 0 || currentFloor >= MAP_FLOORS)
    {
        return;
    }

    for (Busters* pBuster : g_BustersList[currentFloor])
    {
        if (!pBuster)
        {
            continue;
        }

        PointLight* pHeadlight = pBuster->GetHeadlight();
        if (!pHeadlight)
        {
            continue;
        }

        Shader_AddPointLight(pHeadlight);
    }
}