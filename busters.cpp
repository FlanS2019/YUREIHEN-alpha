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
#include <cfloat>
#include <algorithm>
#include <vector>

#include "Floor1.h"
#include "Floor2.h"
#include "Floor3.h"

#include "UI.h"
#include "UI_scarecombo.h"
#include "scene.h"

static std::vector<Busters*> g_BustersList[MAP_FLOORS];

// 前方宣言
bool CanPassLine(const XMFLOAT3& start, const XMFLOAT3& end, float radius, int ignoreFurnitureIndex = -1);

// =================================================================
// パススムージング: 直線で到達可能なノードをスキップして滑らかにする
// =================================================================
static std::vector<XMFLOAT3> SmoothPath(const std::vector<XMFLOAT3>& path, float bodyRadius, int ignoreFurnIdx = -1)
{
	if (path.size() <= 2) return path;

	std::vector<XMFLOAT3> smoothed;
	smoothed.push_back(path[0]);

	size_t current = 0;
	while (current < path.size() - 1)
	{
		size_t farthest = current + 1;
		// 現在のノードから、直線で到達できる最も遠いノードを探す
		for (size_t i = current + 2; i < path.size(); i++)
		{
			if (CanPassLine(path[current], path[i], bodyRadius, ignoreFurnIdx))
			{
				farthest = i;
			}
		}
		smoothed.push_back(path[farthest]);
		current = farthest;
	}

	return smoothed;
}

// =================================================================
// Busters クラスメンバ関数の実装
// =================================================================

Busters::Busters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
	: AnimSprite3D(pos, scale, rot, pass),
	Jump(0.01f, 0.2f, BUSTERS_HEIGHT),
	m_State(BUSTERS_SEARCH),
	m_TargetFurnitureIndex(-1),
	m_WaitTimer(0),
	m_DetectionGraceTimer(0),
	m_MoveSpeed(BUSTERS_MOVE_SPEED_SEARCH),
	m_DistanceToGhost(0.0f),
	m_ReactionCooldown(0),
	m_KeepStateTimer(0),
	m_Icon(nullptr),
	m_pHeadlight(nullptr),
	m_LureTargetPos(pos),
	m_HasLureTarget(false),
	m_LureStayTimer(0),
	m_IsGhostDiscover(false),
	m_IsTutorial(false),
	m_StairsTargetPos(pos),
	m_RunToStairsDone(false)
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
	// アニメーション更新
	const float dt = 1.0f / 60.0f;
	this->UpdateAnimation(dt);

	// 状態に応じたモーション切り替え
	switch (m_State)
	{
	case BUSTERS_SEARCH:
		if (m_WaitTimer > 0)
			this->PlayAnimationByName("chousa", true);
		else
			this->PlayAnimationByName("walk", true);
		break;
	case BUSTERS_WAIT_RESELECT:
		this->PlayAnimationByName("walk", true);
		break;
	case BUSTERS_SUSPICION:
		this->PlayAnimationByName("walk", true);
		break;
	case BUSTERS_CHASE:
		// hakkenが再生中なら終了を待ち、終わったらhakkendashへ＋追跡開始
		if (m_AnimState.currentAnimName == "hakken")
		{
			if (!this->IsAnimationPlaying())
			{
				this->PlayAnimationByName("hakkendash", true);
				m_WaitTimer = 0; // hakken終了 → 即追跡開始
			}
		}
		else if (m_AnimState.currentAnimName != "hakkendash")
		{
			// CHASE状態に入った直後：まずhakkenを非ループで再生
			this->PlayAnimationByName("hakken", false);
		}
		break;
	case BUSTERS_STUN:
		this->PlayAnimationByName("kizetsu", true);
		break;
	case BUSTERS_LURED:
		this->PlayAnimationByName("walk", true);
		break;
	case BUSTERS_RUN_TO_STAIRS:
		this->PlayAnimationByName("hakkendash", true);
		break;
	}

	// ========== デバッグモード: 角度確認用 ==========
	// 左右矢印キーで回転、移動・検知は停止
	if (DEBUG_BUSTERS_ROTATION)
	{
		float currentRot = GetRot().y;
		
		// 左矢印: 角度を減らす
		if (Keyboard_IsKeyDown(KK_LEFT))
		{
			currentRot -= 2.0f;
			SetRotY(currentRot);
		}
		// 右矢印: 角度を増やす
		if (Keyboard_IsKeyDown(KK_RIGHT))
		{
			currentRot += 2.0f;
			SetRotY(currentRot);
		}
		
		// 角度を-180〜180に正規化
		while (currentRot > 180.0f) currentRot -= 360.0f;
		while (currentRot < -180.0f) currentRot += 360.0f;
		
		// デバッグログ出力（毎フレーム）
		float radY = XMConvertToRadians(currentRot);
		float radY90 = XMConvertToRadians(currentRot + 90.0f);
		
		hal::dout << "[DEBUG ROT] GetRot().y=" << currentRot 
		          << " | cos/sin(rot)=(" << cosf(radY) << ", " << sinf(radY) << ")"
		          << " | cos/sin(rot+90)=(" << cosf(radY90) << ", " << sinf(radY90) << ")"
		          << std::endl;
		
		// ヘッドライト更新のみ行う
		if (m_pHeadlight)
		{
			XMFLOAT3 headPos = m_Position;
			headPos.y += 2.0f;
			float dirRadY = XMConvertToRadians(currentRot);
			float dirX = -sinf(dirRadY);
			float dirZ = -cosf(dirRadY);
			headPos.x += dirX * 0.8f;
			headPos.z += dirZ * 0.8f;
			m_pHeadlight->SetPosition(headPos.x, headPos.y, headPos.z);
			m_pHeadlight->SetDirection(XMFLOAT4(dirX, -0.3f, dirZ, 0.0f));
			m_pHeadlight->SetRange(15.0f);
			
			// ライト方向もデバッグログに出力
			hal::dout << "  LightDir: (" << dirX << ", -0.3, " << dirZ << ")" << std::endl;
		}
		
		// アイコン更新
		if (m_Icon)
		{
			m_Icon->SetIcon(BILLBOARD_ICON::SEARCH);
			XMFLOAT3 iconPos = m_Position;
			iconPos.y += 3.25f;
			m_Icon->SetPos(iconPos);
			m_Icon->Update();
		}
		
		return;  // 通常処理をスキップ
	}
	// ========== デバッグモード終了 ==========

	JumpUpdate(*(Transform3D*)this);

	if (m_DetectionGraceTimer > 0)
	{
		m_DetectionGraceTimer--;
	}

	if (m_ReactionCooldown > 0)
	{
		m_ReactionCooldown--;
	}

	Ghost* ghost = GetGhost();
	// 幽霊が無敵でなく、かつ変身中(家具の中)でない場合のみライトを当てる
	if (ghost && !ghost->IsInvincible() &&
		ghost->GetState() != GS_TRANSFORM && ghost->GetState() != GS_SCARE)
	{
		bool hasWall = Field_CheckWallBetween(m_Position, ghost->GetPos());
		bool inFOV = IsTargetInFOV(ghost->GetPos(), BUSTERS_SUSPICION_RANGE);

		if (!hasWall && inFOV)
		{
			ghost->SetIsIlluminated(true);
		}
	}

	// ヘッドライトの位置を頭部に追従させる
	if (m_pHeadlight)
	{
		XMFLOAT3 headPos = m_Position;
		headPos.y += 2.0f;	// 頭部の高さ

		float rotY = GetRot().y;
		float radY = XMConvertToRadians(rotY);

		// 方向計算（モデルの正面がZ軸負方向のため反転）
		float dirX = -sinf(radY);
		float dirZ = -cosf(radY);

		// 位置オフセット
		headPos.x += dirX * 0.8f;
		headPos.z += dirZ * 0.8f;

		m_pHeadlight->SetPosition(headPos.x, headPos.y, headPos.z);

		m_pHeadlight->SetDirection(XMFLOAT4(dirX, -0.3f, dirZ, 0.0f));

		float range = 10.0f;
		XMFLOAT4 color = { 1.0f, 1.0f, 0.95f, 1.0f }; // 固定色（白）

		switch (m_State)
		{
		case BUSTERS_SEARCH:         // 探索
		case BUSTERS_WAIT_RESELECT:  // 再抽選待機
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
			break;
		case BUSTERS_WAIT_RESELECT: m_Icon->SetIcon(BILLBOARD_ICON::SEARCH); break;
		case BUSTERS_SUSPICION:     m_Icon->SetIcon(BILLBOARD_ICON::QUESTION); break;
		case BUSTERS_LURED:         m_Icon->SetIcon(BILLBOARD_ICON::QUESTION); break;
		case BUSTERS_CHASE:         m_Icon->SetIcon(BILLBOARD_ICON::ALERT); break;
		case BUSTERS_STUN:          m_Icon->SetIcon(BILLBOARD_ICON::STUN); break;
		}

		XMFLOAT3 iconPos = m_Position;
		iconPos.y += 3.25f;
		m_Icon->SetPos(iconPos);
		m_Icon->Update();
	}

	// 階段へ走るアニメーション中は最優先で処理（WaitTimerなどに邪魔させない）
	if (m_State == BUSTERS_RUN_TO_STAIRS)
	{
		float dx = m_StairsTargetPos.x - m_Position.x;
		float dz = m_StairsTargetPos.z - m_Position.z;
		float distSq = dx * dx + dz * dz;
		if (distSq <= 0.5f * 0.5f)
		{
			// ID5/6（階段）またはID4（1階出口）のマスの真上にいる場合のみ完了とする
			int blockID = Field_GetRawBlockID(m_Position.x, m_Position.z);
			if (blockID == 5 || blockID == 6 || blockID == 4)
			{
				m_RunToStairsDone = true;
			}
		}
		else
		{
			// 1階出口(ID4)は中心に乗り切れない場合があるため、目標付近でも到達扱いにする
			int targetBlockID = Field_GetRawBlockID(m_StairsTargetPos.x, m_StairsTargetPos.z);
			if (targetBlockID == 4 && distSq <= 1.0f * 1.0f)
			{
				m_RunToStairsDone = true;
			}
		}

		if (m_RunToStairsDone)
		{
			m_PrevPos = m_Position;
			if (m_Icon)
			{
				m_Icon->SetIcon(BILLBOARD_ICON::ALERT);
				XMFLOAT3 iconPos = m_Position;
				iconPos.y += 3.25f;
				m_Icon->SetPos(iconPos);
				m_Icon->Update();
			}
			return;
		}

		{
			m_MoveSpeed = BUSTERS_MOVE_SPEED_CHASE;
			if (!m_PathList.empty())
			{
				MoveTo(m_PathList[0]);
			}
			else
			{
				MoveTo(m_StairsTargetPos);
			}
			// スタック検出：止まっていたらパスを再計算する
			{
				float mdx = m_Position.x - m_PrevPos.x;
				float mdz = m_Position.z - m_PrevPos.z;
				if (mdx * mdx + mdz * mdz < 0.0001f)
				{
					m_StuckTimer++;
				if (m_StuckTimer > 30)
					{
						// パスを再計算して再挑戦
						m_PathList.clear();
						m_PathList = Field_FindPath(m_Position, m_StairsTargetPos);
						if (!m_PathList.empty())
						{
							std::reverse(m_PathList.begin(), m_PathList.end());
							if (!m_PathList.empty()) m_PathList.erase(m_PathList.begin());
							m_PathList = SmoothPath(m_PathList, 0.6f);
						}
						m_StuckTimer = 0;
					}
				}
				else
				{
					m_StuckTimer = 0;
				}
			}
		}
		m_PrevPos = m_Position;
		// アイコン更新
		if (m_Icon)
		{
			m_Icon->SetIcon(BILLBOARD_ICON::ALERT);
			XMFLOAT3 iconPos = m_Position;
			iconPos.y += 3.25f;
			m_Icon->SetPos(iconPos);
			m_Icon->Update();
		}
		return;
	}

	if (m_WaitTimer > 0)
	{
		// CHASE状態でhakken再生中はタイマーを減らない（hakken終了まで硬直維持）
		if (m_State == BUSTERS_CHASE && m_AnimState.currentAnimName == "hakken" && this->IsAnimationPlaying())
		{
			// hakken再生中：タイマーを維持して移動させない
		}
		else
		{
			m_WaitTimer--;
		}

		// 再抽選待機タイマーが終了したら探索ステートへ復帰
		if (m_State == BUSTERS_WAIT_RESELECT && m_WaitTimer <= 0)
		{
			m_State = BUSTERS_SEARCH;
			m_TargetFurnitureIndex = -1;
			m_PathList.clear();
			return;
		}

		if (m_State == BUSTERS_SUSPICION || m_State == BUSTERS_CHASE)
		{
			Ghost* ghost = GetGhost();
			if (ghost)
			{
			float dx = ghost->GetPos().x - m_Position.x;
			float dz = ghost->GetPos().z - m_Position.z;
			float angle = atan2f(-dx, -dz);
			float deg = XMConvertToDegrees(angle);
				
				float currentRot = GetRot().y;
				float angleDiff = deg - currentRot;
				while (angleDiff > 180.0f) angleDiff -= 360.0f;
				while (angleDiff < -180.0f) angleDiff += 360.0f;
				
				float maxRotSpeed = 45.0f;
				if (fabsf(angleDiff) > maxRotSpeed)
				{
				angleDiff = (angleDiff > 0) ? maxRotSpeed : -maxRotSpeed;
				}
				
				SetRotY(currentRot + angleDiff);
			}
		}
		else if (m_State == BUSTERS_SEARCH && m_TargetFurnitureIndex != -1)
		{
			Furniture* target = GetFurniture(m_TargetFurnitureIndex);
			if (target)
			{
				float dx = target->GetPos().x - m_Position.x;
				float dz = target->GetPos().z - m_Position.z;
				float angle = atan2f(-dx, -dz);
				float deg = XMConvertToDegrees(angle);
				
				float currentRot = GetRot().y;
				float angleDiff = deg - currentRot;
				while (angleDiff > 180.0f) angleDiff -= 360.0f;
				while (angleDiff < -180.0f) angleDiff += 360.0f;
				
				float maxRotSpeed = 45.0f;
				if (fabsf(angleDiff) > maxRotSpeed)
				{
				angleDiff = (angleDiff > 0) ? maxRotSpeed : -maxRotSpeed;
				}
				
				SetRotY(currentRot + angleDiff);
			}
		}
		return;
	}

	CheckState();

	XMFLOAT3 nextStepPos = m_Position;

	switch (m_State)
	{
	case BUSTERS_WAIT_RESELECT: // 再抽選待機中はswitch内では何もしない
		m_MoveSpeed = BUSTERS_MOVE_SPEED_SEARCH;
		break;

	case BUSTERS_SEARCH: // 探索

		if (m_TargetFurnitureIndex == -1 || m_PathList.empty())
		{
			if (m_TargetFurnitureIndex == -1)
			{
				Furniture* targetFurniture = nullptr;
				int loopCount = 0;

				while (loopCount < 100)
				{
					m_TargetFurnitureIndex = rand() % FURNITURE_NUM;
					targetFurniture = GetFurniture(m_TargetFurnitureIndex);

					// ドア(13)以外の家具を選ぶ
					if (targetFurniture && targetFurniture->GetBlockID() != 13) break;
					loopCount++;
				}
			}

			Furniture* targetFurniture = GetFurniture(m_TargetFurnitureIndex);
			if (targetFurniture && targetFurniture->GetBlockID() != 13)
			{
				XMFLOAT3 targetPos = targetFurniture->GetPos();
				XMFLOAT3 destination = targetPos;
				int bestDoorIndex = -1;
				bool hasWall = Field_CheckWallBetween(m_Position, targetPos);

				if (hasWall)
				{
				float minDist = 999999.0f;

					for (int i = 0; i < FURNITURE_NUM; i++) {
						Furniture* f = GetFurniture(i);

						if (f && f->GetBlockID() == 13 && !IsIgnoredRelayDoor(i))
						{
							XMFLOAT3 dPos = f->GetPos();

							// 現在地からドアへ、かつドアから目標への直線ルートが可能かチェック
							bool canReachDoor = !Field_CheckWallBetween(m_Position, dPos);
							bool canReachTarget = !Field_CheckWallBetween(dPos, targetPos);

							if (canReachDoor && canReachTarget)
							{
								float d1x = dPos.x - m_Position.x;
								float d1z = dPos.z - m_Position.z;
								float distToDoor = sqrtf(d1x * d1x + d1z * d1z); // 現在地〜ドアの距離

								float d2x = targetPos.x - dPos.x;
								float d2z = targetPos.z - dPos.z;
								float distToTarget = sqrtf(d2x * d2x + d2z * d2z); // ドア〜目標の距離

								float totalDist = distToDoor + distToTarget;

								if (totalDist < minDist) {
									minDist = totalDist;
									bestDoorIndex = i;
								}
							}
						}
					}

					if (bestDoorIndex != -1) {
						// 最適なドアを中継地点に設定する
						destination = GetFurniture(bestDoorIndex)->GetPos();

						// このドアを次から中継地点にしない状態にする
						AddIgnoreRelayDoor(bestDoorIndex);
					}
				}

				float dx = destination.x - m_Position.x;
				float dz = destination.z - m_Position.z;
				float dist = sqrtf(dx * dx + dz * dz);

				bool isTargetDoor = (destination.x != targetPos.x || destination.z != targetPos.z);

				if (dist > 0.0f && !isTargetDoor)
				{
					// 本物の家具を目指す時だけ手前で止まる
					float offset = 0.6f;
					destination.x -= (dx / dist) * offset;
					destination.z -= (dz / dist) * offset;
				}

				// 目的地への経路を生成（destinationを使用してドア経由の経路を作成）
				bool destinationHasWall = Field_CheckWallBetween(m_Position, destination);
				
				if (destinationHasWall && bestDoorIndex == -1)
				{
					// ドアを経由してもアクセス不可の場合は、このターゲットを諦める
					m_TargetFurnitureIndex = -1;
					m_State = BUSTERS_WAIT_RESELECT;
					m_WaitTimer = 60;
				}
				else
				{
					m_PathList = Field_FindPath(m_Position, destination);
					if (m_PathList.empty())
					{
						// 経路が見つからない（壁の中などで行けない）場合は、壁に突っ込まずに潔く諦める
						m_TargetFurnitureIndex = -1;
						m_State = BUSTERS_WAIT_RESELECT;
						m_WaitTimer = 60;
					}
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
			float distSq = dx * dx + dz * dz;
			bool hasWall = Field_CheckWallBetween(m_Position, GetGhost()->GetPos());

			// 壁がなく、かつ距離が近い場合
			if (!hasWall && distSq < 0.25f)
			{
				m_PathList.clear();
				nextStepPos = GetGhost()->GetPos();
			}
			else
			{
				float dist = sqrtf(distSq);

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
				else if (m_PathUpdateTimer > updateInterval * 2 && distGhostMovedSq > 1.0f * 1.0f)
				{
					shouldUpdate = true;
				}

				if (shouldUpdate)
				{
					m_PathUpdateTimer = 0; // タイマーリセット
					m_LastPathCalcGhostPos = ghostPos; // 計算位置を記憶

				// ゴーストへの直線に壁がある場合、ドアを経由する経路を作成
				XMFLOAT3 targetPos = ghostPos;
				int bestDoorIndex = -1;
				if (hasWall)
				{
			float minDist = 999999.0f;

						for (int i = 0; i < FURNITURE_NUM; i++)
						{
							Furniture* f = GetFurniture(i);

							if (f && f->GetBlockID() == 13)
							{
								XMFLOAT3 dPos = f->GetPos();

								// 現在地からドアへ、かつドアからゴーストへのルートが可能かチェック
								bool canReachDoor = !Field_CheckWallBetween(m_Position, dPos);
								bool canReachGhost = !Field_CheckWallBetween(dPos, ghostPos);

								if (canReachDoor && canReachGhost)
								{
									float d1x = dPos.x - m_Position.x;
									float d1z = dPos.z - m_Position.z;

									float d2x = ghostPos.x - dPos.x;
									float d2z = ghostPos.z - dPos.z;

									float totalDist = (sqrtf(d1x * d1x + d1z * d1z)) + (sqrtf(d2x * d2x + d2z * d2z));

									if (totalDist < minDist)
									{
										minDist = totalDist;
										bestDoorIndex = i;
									}
								}
							}
						}

						// ドアが見つかった場合は、そのドアを経由して進む
						if (bestDoorIndex != -1)
						{
							targetPos = GetFurniture(bestDoorIndex)->GetPos();
						}
					}

					m_PathList = Field_FindPath(m_Position, targetPos);

					if (!m_PathList.empty()) {
						std::reverse(m_PathList.begin(), m_PathList.end());

						if (m_PathList.size() > 0) m_PathList.erase(m_PathList.begin());

						// パススムージング：直線で到達できるノードをスキップして滑らかなルートにする
						if (m_PathList.size() > 2)
						{
							m_PathList = SmoothPath(m_PathList, 0.4f);
						}
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

		bool forceArrive = false;
		if (m_State == BUSTERS_SEARCH && m_TargetFurnitureIndex != -1)
		{
			Furniture* target = GetFurniture(m_TargetFurnitureIndex);
			if (target)
			{
				float fdx = target->GetPos().x - m_Position.x;
				float fdz = target->GetPos().z - m_Position.z;

				if (fdx * fdx + fdz * fdz < 1.5f * 1.5f)
				{
					forceArrive = true;
				}
			}
		}

		if (distSq < arriveThreshold * arriveThreshold || forceArrive)
		{
			m_PathList.erase(m_PathList.begin());

			if (m_PathList.empty() || forceArrive)
			{
				if (m_State == BUSTERS_SEARCH || m_State == BUSTERS_LURED)
				{
					bool reached = forceArrive;
					if (!reached && m_TargetFurnitureIndex != -1) {
						Furniture* target = GetFurniture(m_TargetFurnitureIndex);
						if (target) {
							float tdx = target->GetPos().x - m_Position.x;
							float tdz = target->GetPos().z - m_Position.z;
							// 目標家具から1.5m以内にいれば到着とみなす
							if (tdx * tdx + tdz * tdz < 1.5f * 1.5f) {
								reached = true;
							}
						}
					}

					if (reached) {
						// 本当に目標に到着した
						m_State = BUSTERS_SEARCH;
						m_TargetFurnitureIndex = -1; // ターゲット解除
						m_WaitTimer = 300;           // 調査（待機）開始
						m_PathList.clear();
						ClearIgnoreRelayDoors();
						m_StuckTimer = 0; // スタックタイマーもリセット
					}
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

	if ((m_State == BUSTERS_SEARCH || m_State == BUSTERS_WAIT_RESELECT) && m_TargetFurnitureIndex != -1)
	{
		Furniture* target = GetFurniture(m_TargetFurnitureIndex);
		if (target)
		{
			// フラグを立てる
			target->SetIsTargeted(true);
		}
	}

	// スタック検出と回避
	{
		float dx = m_Position.x - m_PrevPos.x;
		float dz = m_Position.z - m_PrevPos.z;
		float movedDistSq = dx * dx + dz * dz;

		if (movedDistSq < 0.0001f)
		{
			m_StuckTimer++;
			// 移動不可になったら即座に経路を再計算する
			if (m_StuckTimer > 15)
			{
			if (!m_PathList.empty())
				{
				m_PathList.erase(m_PathList.begin());
				m_StuckTimer = 0;
				}
				else if (m_State == BUSTERS_SEARCH && m_TargetFurnitureIndex != -1)
				{
					// 経路のないスタックも処理
					m_TargetFurnitureIndex = -1;
					m_State = BUSTERS_WAIT_RESELECT;
					m_WaitTimer = 60;
					m_StuckTimer = 0;
				}
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
		float dz = gPos.z - m_Position.z;
		float dist = sqrtf(dx * dx + dz * dz);

		GHOST_STATE ghostState = GetGhost()->GetState();

		if (dist < 1.0f && !GetGhost()->IsInvincible() &&
			ghostState != GS_CAUGHT &&
			ghostState != GS_TRANSFORM &&
			ghostState != GS_SCARE)
		{
			GetGhost()->SetState(GS_CAUGHT);
			m_PathList.clear();
		}
	}
}

bool IsWallBlock(float x, float z)
{
	// Field_IsWall を使うことでチュートリアル壁フラグ（g_TutorialWallEnabled）が反映される
	return Field_IsWall(x, z);
}

bool IsObstacle(float x, float z, float radius, int ignoreFurnitureIndex = -1)
{
	// 床なし（Y=0がID:0）のマスは通行不可
	if (Field_IsNoFloor(x, z)) return true;

	// 壁の判定 (中心と4隅をチェックしてめり込みを防ぐ)
	// 1マスの狭い通路で引っかからないよう、コーナー判定の半径を少し小さくする
	float checkR = radius * 0.4f;
	if (IsWallBlock(x, z) ||
		IsWallBlock(x + checkR, z + checkR) ||
		IsWallBlock(x + checkR, z - checkR) ||
		IsWallBlock(x - checkR, z + checkR) ||
		IsWallBlock(x - checkR, z - checkR))
	{
		return true;
	}

	/*// 家具の判定
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (i == ignoreFurnitureIndex) continue; // ターゲット家具は通り抜けてOK

		Furniture* pFurn = GetFurniture(i);
		if (!pFurn) continue;

		int fID = pFurn->GetBlockID();
		if (fID == 13 || fID == 68 || fID == 69) continue;

		XMFLOAT3 fPos = pFurn->GetPos();
		float dx = fPos.x - x;
		float dz = fPos.z - z;
		float distSq = dx * dx + dz * dz;

		// 家具の半径を0.6f程度と仮定。キャラの半径と合わせて判定
		float sumR = 0.6f + radius;

		if (distSq < sumR * sumR)
		{
			return true; // 家具にぶつかる！
		}
	}*/
	return false;
}

bool CanPassLine(const XMFLOAT3& start, const XMFLOAT3& end, float radius, int ignoreFurnitureIndex)
{
	float dx = end.x - start.x;
	float dz = end.z - start.z;
	float len = sqrtf(dx * dx + dz * dz);

	if (len <= 0.001f) return true;

	float ndx = dx / len;
	float ndz = dz / len;
	// 精密にチェックするため、ステップサイズを0.2fに変更（角抜け防止）
	int steps = (int)(len / 0.2f) + 1;

	for (int i = 0; i <= steps; i++)
	{
		float t = (float)i / steps;
		float px = start.x + dx * t;
		float pz = start.z + dz * t;

		// IsWallBlock ではなく IsObstacle を使う
		if (IsObstacle(px, pz, radius, ignoreFurnitureIndex))
		{
			return false;
		}
	}
	return true;
}

void Busters::CheckState(void)
{
	if (m_IsTutorial)
	{
		return;
	}

	if (m_State == BUSTERS_LURED || m_State == BUSTERS_WAIT_RESELECT)
	{
		return;
	}

	Ghost* ghost = GetGhost();
	if (!ghost) return;

	if (ghost->IsInvincible())
	{
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

			ghost->SetInvincible(300);
		}
		return;
	}

	// 壁チェック
	bool hasWall = Field_CheckWallBetween(m_Position, ghost->GetPos());


	// 判定範囲の決定（ヒステリシス付き）<
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

	int ignoreFurnIdx = -1;
	if (m_State == BUSTERS_SEARCH)
	{
		ignoreFurnIdx = m_TargetFurnitureIndex;
	}

	// === NavMesh風エージェント移動 ===
	
	// 1. 現在の目標ノードを決定
	XMFLOAT3 currentTarget = targetPos;
	if (!m_PathList.empty())
	{
		currentTarget = m_PathList[0];
	}

	// 2. 目標への距離と方向を計算
	float dx = currentTarget.x - m_Position.x;
	float dz = currentTarget.z - m_Position.z;
	float distSq = dx * dx + dz * dz;
	float dist = sqrtf(distSq);

	// 3. ノード到達判定（NavMesh風：ノードに十分近づいたら次へ）　
	const float NODE_REACH_DISTANCE = (m_State == BUSTERS_RUN_TO_STAIRS) ? 0.25f : 0.6f;
	if (!m_PathList.empty() && dist < NODE_REACH_DISTANCE)
	{
		m_PathList.erase(m_PathList.begin());
		
		// 次のノードがあれば更新
		if (!m_PathList.empty())
		{
			currentTarget = m_PathList[0];
			dx = currentTarget.x - m_Position.x;
			dz = currentTarget.z - m_Position.z;
			distSq = dx * dx + dz * dz;
			dist = sqrtf(distSq);
		}
	}

	// 4. 移動不要チェック
	if (dist < 0.05f) return;

	// 5. 移動方向を計算（先読み: 追跡・警戒時はパスの先を見て方向を補間）
	float dirX = dx / dist;
	float dirZ = dz / dist;

	if ((m_State == BUSTERS_CHASE || m_State == BUSTERS_SUSPICION) && m_PathList.size() >= 2)
	{
		// 現在のノードと次のノードの方向をブレンドして滑らかにする
		XMFLOAT3 nextNode = m_PathList[1];
		float ndx = nextNode.x - m_Position.x;
		float ndz = nextNode.z - m_Position.z;
		float ndist = sqrtf(ndx * ndx + ndz * ndz);
		if (ndist > 0.1f)
		{
			float nextDirX = ndx / ndist;
			float nextDirZ = ndz / ndist;
			// 現在ノードに近いほど次のノード方向の影響を大きくする
			float blendFactor = 1.0f - (dist / (dist + 1.0f));
			blendFactor = blendFactor * 0.5f; // 最大50%まで次のノード方向をブレンド
			dirX = dirX * (1.0f - blendFactor) + nextDirX * blendFactor;
			dirZ = dirZ * (1.0f - blendFactor) + nextDirZ * blendFactor;
			// 正規化
			float blendLen = sqrtf(dirX * dirX + dirZ * dirZ);
			if (blendLen > 0.001f)
			{
				dirX /= blendLen;
				dirZ /= blendLen;
			}
		}
	}

	// 6. 回転処理（NavMesh風：常に移動方向を向く、毎フレーム更新）
	float targetAngle = XMConvertToDegrees(atan2f(-dirX, -dirZ));
	float currentRot = GetRot().y;
	
	// 角度を-180〜180に正規化
	while (currentRot > 180.0f) currentRot -= 360.0f;
	while (currentRot < -180.0f) currentRot += 360.0f;
	while (targetAngle > 180.0f) targetAngle -= 360.0f;
	while (targetAngle < -180.0f) targetAngle += 360.0f;
	
	// 最短回転方向を計算
	float angleDiff = targetAngle - currentRot;
	while (angleDiff > 180.0f) angleDiff -= 360.0f;
	while (angleDiff < -180.0f) angleDiff += 360.0f;
	
	// 滑らかな回転（状態に応じて最大回転速度を変える）
	float maxRotSpeed = 15.0f;
	if (m_State == BUSTERS_CHASE) maxRotSpeed = 8.0f;       // 追跡時はより滑らかに
	else if (m_State == BUSTERS_SUSPICION) maxRotSpeed = 10.0f; // 警戒時もやや滑らか
	
	if (fabsf(angleDiff) > maxRotSpeed)
	{
		angleDiff = (angleDiff > 0) ? maxRotSpeed : -maxRotSpeed;
	}
	
	float newRot = currentRot + angleDiff;
	while (newRot > 180.0f) newRot -= 360.0f;
	if (newRot < -180.0f) newRot += 360.0f;
	SetRotY(newRot);

	// 7. 移動処理（追跡・警戒時は実際の回転方向に沿って移動する）
	float bodyRadius = 0.4f;
	float moveAmount = m_MoveSpeed;
	
	// 目標が近い場合は減速
	if (dist < 1.0f)
	{
		moveAmount *= (dist / 1.0f);
	}
	
	// 回転が大きく違う場合は移動を抑制（その場で回転）
	float angleDiffAbs = fabsf(angleDiff);
	if (m_State == BUSTERS_CHASE || m_State == BUSTERS_SUSPICION || m_State == BUSTERS_RUN_TO_STAIRS)
	{
		// 追跡・警戒・階段移動時は回転方向に沿って移動（旋回しながら走る）
		if (angleDiffAbs > 45.0f)
		{
			moveAmount *= 0.5f;
		}
		// 実際の向きに基づいて移動方向を計算
		float moveRadY = XMConvertToRadians(newRot);
		dirX = -sinf(moveRadY);
		dirZ = -cosf(moveRadY);
	}
	else
	{
		if (angleDiffAbs > 60.0f)
		{
			moveAmount *= 0.3f;
		}
	}

	float nextPosX = m_Position.x + dirX * moveAmount;
	float nextPosZ = m_Position.z + dirZ * moveAmount;

	// 8. 衝突判定と移動
	if (!IsObstacle(nextPosX, nextPosZ, bodyRadius, ignoreFurnIdx))
	{
		m_Position.x = nextPosX;
		m_Position.z = nextPosZ;
	}
	else
	{
		// 壁スライド移動
		bool moved = false;
		
		if (!moved && fabsf(dirX) > 0.1f)
		{
			float slideX = m_Position.x + dirX * moveAmount;
			if (!IsObstacle(slideX, m_Position.z, bodyRadius, ignoreFurnIdx))
			{
				m_Position.x = slideX;
				moved = true;
			}
		}
		
		if (!moved && fabsf(dirZ) > 0.1f)
		{
			float slideZ = m_Position.z + dirZ * moveAmount;
			if (!IsObstacle(m_Position.x, slideZ, bodyRadius, ignoreFurnIdx))
			{
				m_Position.z = slideZ;
				moved = true;
			}
		}
		
		// 完全にブロックされた場合、次のノードへスキップ
		if (!moved && !m_PathList.empty())
		{
			m_StuckTimer++;
			if (m_StuckTimer > 30)
			{
				m_PathList.erase(m_PathList.begin());
				m_StuckTimer = 0;
			}
		}
	}
}

void Busters::OnScared(void)
{
	SetGroundLevel(BUSTERS_HEIGHT);
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

void Busters::StartRunToStairs(XMFLOAT3 stairsPos)
{
	m_State = BUSTERS_RUN_TO_STAIRS;
	m_StairsTargetPos = stairsPos;
	m_RunToStairsDone = false;
	m_WaitTimer = 0;
	m_StuckTimer = 0;
	m_PathList.clear();
	m_PathList = Field_FindPath(m_Position, stairsPos);
	// Field_FindPath は終点→始点の逆順で返すので正順に直す
	if (!m_PathList.empty())
	{
		std::reverse(m_PathList.begin(), m_PathList.end());
		if (!m_PathList.empty()) m_PathList.erase(m_PathList.begin()); // 始点自身を除去
		// スムージングを適用して直線的に移動できる区間はノードをスキップ（半径を大きめにして障害物を回避）
		m_PathList = SmoothPath(m_PathList, 0.6f);
	}
	this->SetColor(1.0f, 0.5f, 0.0f, 1.0f); // オレンジ
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
	// 左手系 + Mayaモデル補正: モデルの正面がZ軸負方向
	// rotY=0でZ軸負方向を向くため、正面ベクトルは (-sin(rot), -cos(rot))
	float rotRad = XMConvertToRadians(GetRot().y);
	float forwardX = -sinf(rotRad);
	float forwardZ = -cosf(rotRad);
	XMVECTOR forwardVec = XMVectorSet(forwardX, 0.0f, forwardZ, 0.0f);

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
	// 最大100回トライして、床があり周囲に家具がない場所を探す
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
		int floorBlockID = 0;
		switch (floor)
		{
		case 0:
			blockID = Floor1[1][gz][gx];
			floorBlockID = Floor1[0][gz][gx];
			break;
		case 1:
			blockID = Floor2[1][gz][gx];
			floorBlockID = Floor2[0][gz][gx];
			break;
		case 2:
			blockID = Floor3[1][gz][gx];
			floorBlockID = Floor3[0][gz][gx];
			break;
		}

		// IDが0 (空気) で、床が存在する（0以外）場合のみチェック
		if (blockID == 0 && floorBlockID != 0)
		{
			// 周囲に家具がないかチェック（3x3の範囲）
			bool hasFurnitureNearby = false;
			for (int i = -1; i <= 1; i++)
			{
				for (int j = -1; j <= 1; j++)
				{
					int checkX = gx + i;
					int checkZ = gz + j;

					// 範囲内かチェック
					if (checkX < 0 || checkX >= MAP_WIDTH || checkZ < 0 || checkZ >= MAP_LENGTH)
						continue;

					int furnitureID = 0;
					switch (floor)
					{
					case 0: furnitureID = Floor1[1][checkZ][checkX]; break;
					case 1: furnitureID = Floor2[1][checkZ][checkX]; break;
					case 2: furnitureID = Floor3[1][checkZ][checkX]; break;
					}

					// 50〜69は家具マーカーなので、これが周囲にあったら避ける
					if (furnitureID >= 50 && furnitureID <= 69)
					{
						hasFurnitureNearby = true;
						break;
					}
				}
				if (hasFurnitureNearby) break;
			}

			// 周囲に家具がなければスポーンOK
			if (!hasFurnitureNearby)
			{
				// グリッド座標 -> ワールド座標 変換
				// field.cpp の計算式に合わせる: (x - W/2, z - H/2)
				float wx = (float)gx - MAP_WIDTH / 2.0f;
				float wz = MAP_LENGTH / 2.0f - (float)gz;

				return { wx, BUSTERS_HEIGHT, wz };
			}
		}
		attempts++;
	}

	// 見つからなかった場合の安全策 (原点)
	return { 0.0f, PATROL_HEIGHT, 0.0f };
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
		Busters* b = new Busters(pos, { 0.12f, 0.12f, 0.12f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\busters_v3.fbx");
		if (b) {
			b->SetGroundLevel(PATROL_HEIGHT);
			g_BustersList[0].push_back(b);
		}
	}

	// 2階 (Floor 1)
	for (int i = 0; i < 2; i++)
	{
		XMFLOAT3 pos = GetRandomBusterPos(1);
		Busters* b = new Busters(pos, { 0.12f, 0.12f, 0.12f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\busters_v3.fbx");
		if (b) {
			b->SetGroundLevel(PATROL_HEIGHT);
			g_BustersList[1].push_back(b);
		}
	}

	// 3階 (Floor 2) - チュートリアル用バスターズはTutorial_Objectで別管理。通常バスターズは生成しない
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

Busters* GetBustersByIndex(int index)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
	{
		if (index >= 0 && index < (int)g_BustersList[currentFloor].size()) {
			return g_BustersList[currentFloor][index];
		}
	}
	return NULL;
}

int Busters_GetCurrentFloorCount(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
	{
		return (int)g_BustersList[currentFloor].size();
	}
	return 0;
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

bool Busters_IsAnyInRange(const XMFLOAT3& pos, float range)
{
	float rangeSq = range * range;
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
	{
		for (Busters* buster : g_BustersList[currentFloor])
		{
			XMFLOAT3 bPos = buster->GetPos();
			float dx = bPos.x - pos.x;
			float dz = bPos.z - pos.z;
			if (dx * dx + dz * dz <= rangeSq)
			{
				return true;
			}
		}
	}
	return false;
}


// =================================================================
// ゲージMAX時の処理
// =================================================================

// アニメーション開始前に一度だけ呼ぶ（勝利判定 + アニメーション起動）
// 戻り値: true=フロア移行アニメを開始した / false=勝利フェードを開始した
bool Busters_CheckGaugeEvent(void)
{
	// デバッグモード中は勝敗判定をスキップ
	if (DEBUG_BUSTERS_ROTATION) return false;

	if (!UI_IsScareGaugeMax()) return false;

	int currentFloor = Field_GetCurrentFloor();

	if (currentFloor == END_FLOOR - 1)
	{
		// クリア階（3階）の場合 -> ゲーム勝利
		StartFade(SCENE_ANM_WIN);
		return false;
	}
	else if (currentFloor > 0)
	{
		// 2階以上の場合 -> フロア降下アニメ開始を呼び出し元に通知
		return true;
	}
	else
	{
		// 1階の場合 -> 逃げ場なし（プレイヤーの勝利）
// 		StartFade(SCENE_ANM_WIN);
		return false;
	}
}

// フロア移行の実体処理（アニメーション完了後に呼ぶ）
void Busters_DoFloorTransition(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor <= 0) return;

	// ゲージをリセット
	UI_ResetScareGauge();

	// 0.0以下で敗北になるのでとりあえず回復
	AddScareGauge(BUSTERS_DEFOURT_GAUGE);

	int nextFloor = currentFloor - 1;

	// 階層に応じて増やす人数を決める
	int addCount = 0;
	if (nextFloor == 1) addCount = 1; // 2階へ行くとき： +1人
	if (nextFloor == 0) addCount = 2; // 1階へ行くとき： +2人

	if (currentFloor == 2)
	{
		// 3階からの遷移：チュートリアル用スポーンのバスターズが混在しているため
		// 全員削除して次の階に新規スポーンする
		for (Busters* buster : g_BustersList[currentFloor])
			delete buster;
		g_BustersList[currentFloor].clear();

		// 次の階の人数分だけ新規生成（通常フロア遷移と同じ増加分を適用） 
		int spawnCount = 1 + addCount; // 元々いた1人分 + 増加分
		for (int i = 0; i < spawnCount; i++)
		{
			XMFLOAT3 newPos = GetRandomBusterPos(nextFloor);
			Busters* newBuster = new Busters(
				{ newPos.x, BUSTERS_HEIGHT, newPos.z },
				{ 0.12f, 0.12f, 0.12f },
				{ 0.0f, 0.0f, 0.0f },
				"asset\\model\\busters_v3.fbx"
			);
			if (newBuster)
			{
				newBuster->SetGroundLevel(PATROL_HEIGHT);
				g_BustersList[nextFloor].push_back(newBuster);
			}
		}
	}
	else
	{
		for (Busters* buster : g_BustersList[currentFloor])
		{
			XMFLOAT3 newPos = GetRandomBusterPos(nextFloor);
			buster->SetPos(newPos);
			g_BustersList[nextFloor].push_back(buster);
		}
		g_BustersList[currentFloor].clear();

		for (int i = 0; i < addCount; i++)
		{
			if (nextFloor >= 0 && nextFloor < MAP_FLOORS)
			{
				XMFLOAT3 newPos = GetRandomBusterPos(nextFloor);

				Busters* newBuster = new Busters(
					{ newPos.x, BUSTERS_HEIGHT, newPos.z },
					{ 0.12f, 0.12f, 0.12f },
					{ 0.0f, 0.0f, 0.0f },
					"asset\\model\\busters_v3.fbx"
				);

				if (newBuster) {
					newBuster->SetGroundLevel(PATROL_HEIGHT);
					g_BustersList[nextFloor].push_back(newBuster);
				}
			}
		}
	}

	// 幽霊の階層移動はアニメーション完了後（FADE_MAX時）にgame.cpp側で行う
}

// 現在フロアのバスターズを全削除する
void Busters_DeleteCurrentFloor(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor < 0 || currentFloor >= MAP_FLOORS) return;
	for (Busters* buster : g_BustersList[currentFloor])
		delete buster;
	g_BustersList[currentFloor].clear();
}

// 階段到着済みのバスターズを先頭から1体削除する
// 戻り値: true=1体削除した / false=到着済みバスターズがいなかった
bool Busters_DeleteFirstArrived(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor < 0 || currentFloor >= MAP_FLOORS) return false;

	auto& list = g_BustersList[currentFloor];
	for (auto it = list.begin(); it != list.end(); ++it)
	{
		if ((*it)->IsRunToStairsDone())
		{
			delete *it;
			list.erase(it);
			return true;
		}
	}
	return false;
}

// 指定フロアにバスターズを生成する
// 既存のバスターズをすべて削除してから、フロアに応じた人数で新規生成する
void Busters_SpawnOnFloor(int floorIndex)
{
	if (floorIndex < 0 || floorIndex >= MAP_FLOORS) return;

	// 既存バスターズを全削除してから生成する（重複防止）
	for (Busters* buster : g_BustersList[floorIndex])
		delete buster;
	g_BustersList[floorIndex].clear();

	// 生成人数：基本1体 + 階層ごとの増加分
	// 2階(index1)へ：1+1=2体、1階(index0)へ：1+2=3体
	int addCount = 0;
	if (floorIndex == 1) addCount = 1;
	if (floorIndex == 0) addCount = 2;
	int spawnCount = 1 + addCount;

	for (int i = 0; i < spawnCount; i++)
	{
		XMFLOAT3 newPos = GetRandomBusterPos(floorIndex);
		Busters* newBuster = new Busters(
			{ newPos.x, BUSTERS_HEIGHT, newPos.z },
			{ 0.12f, 0.12f, 0.12f },
			{ 0.0f, 0.0f, 0.0f },
			"asset\\model\\busters_v3.fbx"
		);
		if (newBuster)
		{
			newBuster->SetGroundLevel(PATROL_HEIGHT);
			g_BustersList[floorIndex].push_back(newBuster);
		}
	}
}

// 指定座標・フロアにバスターズを1体生成する
void Busters_SpawnAt(const XMFLOAT3& pos, int floorIndex)
{
	if (floorIndex < 0 || floorIndex >= MAP_FLOORS) return;

	Busters* newBuster = new Busters(
		{ pos.x, BUSTERS_HEIGHT, pos.z },
		{ 0.12f, 0.12f, 0.12f },
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\busters_v3.fbx"
	);

	if (newBuster)
	{
		newBuster->SetGroundLevel(PATROL_HEIGHT);
		g_BustersList[floorIndex].push_back(newBuster);
	}
}



void Busters::Draw(void)
{
	// バスターズ（ボーンあり）を描画

	AnimSprite3D::Draw();

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

// =================================================================
// 中継地点（ドア）のブラックリスト管理関数
// =================================================================

// リストに追加して、次回から選ばれないようにする
void Busters::AddIgnoreRelayDoor(int furnitureIndex)
{
	m_IgnoredDoorIndices.push_back(furnitureIndex);
}

// 目標に到達した時にリストを空にする
void Busters::ClearIgnoreRelayDoors(void)
{
	m_IgnoredDoorIndices.clear();
}

// 指定されたドアが既にリストに入っているかチェックする
bool Busters::IsIgnoredRelayDoor(int furnitureIndex)
{

	for (int ignoredIndex : m_IgnoredDoorIndices)
	{
		if (ignoredIndex == furnitureIndex) return true;
	}
	return false;
}

// =================================================================
// フロア降下アニメーション用グローバル関数
// =================================================================

void Busters_StartFloorExitAnim(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor < 0 || currentFloor >= MAP_FLOORS) return;

	std::vector<XMFLOAT3> exits;
	if (currentFloor == 0)
	{
		// 1階はID4（出口ブロック）へ誘導
		exits = Field_GetFloor1ExitPositions(currentFloor);
	}
	else
	{
		// 2階以上はID5・6（下付き階段）へ誘導
		exits = Field_GetStairsExitPositions(currentFloor);
	}

	if (exits.empty())
	{
		// フォールバック：97マーカーへ誘導
		XMFLOAT3 fallback = Field_GetMarker97WorldPos(currentFloor);
		for (Busters* buster : g_BustersList[currentFloor])
			buster->StartRunToStairs(fallback);
		return;
	}

	for (Busters* buster : g_BustersList[currentFloor])
	{
		// 各バスターズの現在位置から最も近い出口を選ぶ
		XMFLOAT3 pos = buster->GetPos();
		XMFLOAT3 nearest = exits[0];
		float minDist = FLT_MAX;
		for (const XMFLOAT3& exit : exits)
		{
			float dx = exit.x - pos.x;
			float dz = exit.z - pos.z;
			float dist = dx * dx + dz * dz;
			if (dist < minDist)
			{
				minDist = dist;
				nearest = exit;
			}
		}
		buster->StartRunToStairs(nearest);
	}
}

bool Busters_IsFloorExitAnimDone(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor < 0 || currentFloor >= MAP_FLOORS) return false;

	// バスターズが0人の場合は未完了扱い（アニメ開始直後の即遷移を防ぐ）
	if (g_BustersList[currentFloor].empty()) return false;

	for (Busters* buster : g_BustersList[currentFloor])
	{
		if (!buster->IsRunToStairsDone()) return false;
	}
	return true;
}
