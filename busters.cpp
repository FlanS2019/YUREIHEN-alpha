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
	m_IsGhostDiscover(false),
	m_IsTutorial(false)
{
	m_Icon = new Billboard();
	m_LastPathCalcGhostPos = { -999.0f, 0.0f, -999.0f }; 
	m_Icon->Initialize({ 0.0f, 0.0f, 0.0f }, { 0.7f, 0.7f }, { 0.0f, 0.0f, 0.0f }, true);
	m_PathUpdateTimer = rand() % 15;
	m_PrevPos = pos;
	m_StuckTimer = 0;
	m_RotationUpdateCounter = 0;
	m_PrevTargetAngle = 0.0f;
	m_AngleFlipCounter = 0;

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
			float dirRadY = XMConvertToRadians(currentRot + 90.0f);
			float dirX = cosf(dirRadY);
			float dirZ = sinf(dirRadY);
			headPos.x += dirX * 0.8f;
			headPos.z += dirZ * 0.8f;
			m_pHeadlight->SetPosition(headPos.x, headPos.y, headPos.z);
			m_pHeadlight->SetDirection(XMFLOAT4(dirX, -0.3f, dirZ, 0.0f));
			m_pHeadlight->SetRange(15.0f);
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
		float radY = XMConvertToRadians(rotY + 90.0f);  // MoveToの-90度オフセットに対応

		// 方向計算
		float dirX = cosf(radY);
		float dirZ = sinf(radY);

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
			Ghost* ghost = GetGhost();
			if (ghost)
			{
			float dx = ghost->GetPos().x - m_Position.x;
			float dz = ghost->GetPos().z - m_Position.z;
			float angle = atan2f(dz, dx);
			float deg = XMConvertToDegrees(angle) - 90.0f;
				
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
				float angle = atan2f(dz, dx);
				float deg = XMConvertToDegrees(angle) - 90.0f;
				
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
					m_WaitTimer = 60;
				}
				else
				{
					m_PathList = Field_FindPath(m_Position, destination);
					if (m_PathList.empty())
					{
						// 経路が見つからない（壁の中などで行けない）場合は、壁に突っ込まずに潔く諦める
						m_TargetFurnitureIndex = -1;
						m_WaitTimer = 60; // 諦めて少し待機してから、別の目標を探す
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
									float distToDoor = sqrtf(d1x * d1x + d1z * d1z);

									float d2x = ghostPos.x - dPos.x;
									float d2z = ghostPos.z - dPos.z;
									float distToGhost = sqrtf(d2x * d2x + d2z * d2z);

									float totalDist = distToDoor + distToGhost;

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

	if (m_State == BUSTERS_SEARCH && m_TargetFurnitureIndex != -1)
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
			m_AngleFlipCounter = 0;
			}
				else if (m_State == BUSTERS_SEARCH && m_TargetFurnitureIndex != -1)
				{
					// 経路のないスタックも処理
					m_TargetFurnitureIndex = -1;
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
		float dy = gPos.y - m_Position.y;
		float dz = gPos.z - m_Position.z;
		float dist = sqrtf(dx * dx + dy * dy + dz * dz);

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

int GetBlockIDFromWorldPos(float x, float z)
{
	// ワールド座標 -> グリッド座標(gx, gz) への変換
	// GetRandomBusterPos の逆算: gx = wx + MAP_WIDTH / 2.0f
	// 四捨五入するために +0.5f して int にキャスト
	int gx = (int)(x + MAP_WIDTH / 2.0f + 0.5f);
	int gz = (int)(MAP_LENGTH / 2.0f - z + 0.5f);

	// 配列外参照防止
	if (gx < 0 || gx >= MAP_WIDTH || gz < 0 || gz >= MAP_LENGTH) return 1; // 範囲外は壁扱い

	int floor = Field_GetCurrentFloor();

	// Y=1 (壁レイヤー) の情報を参照
	switch (floor)
	{
	case 0: return Floor1[1][gz][gx];
	case 1: return Floor2[1][gz][gx];
	case 2: return Floor3[1][gz][gx];
	default: return 1;
	}
}

bool IsWallBlock(float x, float z)
{
	int id = GetBlockIDFromWorldPos(x, z);

	// 0(空気)、13(ドア)、17(カーペット) は通り抜け可能
	if (id == 0 || id == 13 || id == 17) return false;

	// 50〜69(家具マーカー) や 98(方向指定マーカー) もマップチップの壁としては扱わない
	if ((id >= 50 && id <= 69) || id == 98) return false;

	return true; // それ以外（本物の壁）は true を返す
}

bool IsChokePoint(float x, float z)
{
	// 1マス(1.0f)の周囲の壁をチェック
	bool wallL = IsWallBlock(x - 1.0f, z);
	bool wallR = IsWallBlock(x + 1.0f, z);
	bool wallU = IsWallBlock(x, z + 1.0f);
	bool wallD = IsWallBlock(x, z - 1.0f);

	// 左右が壁、または上下が壁（ドアや細い通路）
	if (wallL && wallR) return true;
	if (wallU && wallD) return true;

	// L字の角（曲がり角）
	if ((wallL || wallR) && (wallU || wallD)) return true;

	return false;
}

bool IsObstacle(float x, float z, float radius, int ignoreFurnitureIndex = -1)
{
	// 壁の判定 (中心と4隅をチェックしてめり込みを防ぐ)
	float checkR = radius * 0.7f;
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

bool CanPassLine(const XMFLOAT3& start, const XMFLOAT3& end, float radius, int ignoreFurnitureIndex = -1)
{
	float dx = end.x - start.x;
	float dz = end.z - start.z;
	float len = sqrtf(dx * dx + dz * dz);

	if (len <= 0.001f) return true;

	float ndx = dx / len;
	float ndz = dz / len;
	int steps = (int)(len / 0.5f) + 1;

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

	if (m_State == BUSTERS_LURED)
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
	const float NODE_REACH_DISTANCE = 0.6f;
	if (!m_PathList.empty() && dist < NODE_REACH_DISTANCE)
	{
		m_PathList.erase(m_PathList.begin());
		m_AngleFlipCounter = 0;
		
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

	// 5. 移動方向を計算
	float dirX = dx / dist;
	float dirZ = dz / dist;

	// 6. 回転処理（NavMesh風：常に移動方向を向く、毎フレーム更新）
	// atan2(dirZ, dirX)はX軸正方向が0度だが、モデルはZ軸負方向が正面
	// そのため90度のオフセットを追加
	float targetAngle = XMConvertToDegrees(atan2f(dirZ, dirX)) - 90.0f;
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
	
	// 滑らかな回転（最大回転速度を制限）
	const float MAX_ROT_SPEED = 15.0f;
	if (fabsf(angleDiff) > MAX_ROT_SPEED)
	{
		angleDiff = (angleDiff > 0) ? MAX_ROT_SPEED : -MAX_ROT_SPEED;
	}
	
	float newRot = currentRot + angleDiff;
	while (newRot > 180.0f) newRot -= 360.0f;
	while (newRot < -180.0f) newRot += 360.0f;
	SetRotY(newRot);

	// 7. 移動処理
	float bodyRadius = 0.4f;
	float moveAmount = m_MoveSpeed;
	
	// 目標が近い場合は減速
	if (dist < 1.0f)
	{
		moveAmount *= (dist / 1.0f);
	}
	
	// 回転が大きく違う場合は移動を抑制（その場で回転）
	if (fabsf(angleDiff) > 60.0f)
	{
		moveAmount *= 0.3f;
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
	// MoveTo内でatan2(dirZ, dirX) - 90度でSetRotYしているので、
	// 正面ベクトルは(sin(rot), cos(rot))ではなく、
	// rot+90度を使って(cos(rot+90), sin(rot+90)) = (-sin(rot), cos(rot))となる
	// 簡略化: 元の方向 = atan2の結果なので、rot + 90度を使う
	float rotRad = XMConvertToRadians(GetRot().y + 90.0f);
	float forwardX = cosf(rotRad);
	float forwardZ = sinf(rotRad);
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

				return { wx, PATROL_HEIGHT, wz };
			}
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
		float currentAngle = radY - halfFovRad + (halfFovRad * 2.0f * progress);

		// 終点計算
		float dx = sinf(currentAngle);
		float dz = cosf(currentAngle);
		XMFLOAT3 endPos = {
			startPos.x + dx * range,
			startPos.y,
			startPos.z + dz * range
		};

		// 中心から外周への線
		if (i == 0 || i == segments) // 両端のみ描画
		{
			vertices.push_back({ startPos, color });
			vertices.push_back({ endPos, color });
		}

		// 外周の弧を描画（ひとつ前の点と結ぶ）
		if (i > 0)
		{
			float prevAngle = radY - halfFovRad + (halfFovRad * 2.0f * ((float)(i - 1) / segments));
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
		// シェーダー設定
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
		Busters* b = new Busters(pos, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\busters_v3.fbx");
		if (b) {
			b->SetGroundLevel(PATROL_HEIGHT);
			g_BustersList[0].push_back(b);
		}
	}

	// 2階 (Floor 1)
	{
		XMFLOAT3 pos = GetRandomBusterPos(1);
		Busters* b = new Busters(pos, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\busters_v3.fbx");
		if (b) {
			b->SetGroundLevel(PATROL_HEIGHT);
			g_BustersList[1].push_back(b);
		}
	}

	// 3階 (Floor 2) - チュートリアル用バスターズはTutorial_Bustarsで管理
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

// =================================================================
// ゲージMAX時の処理
// =================================================================
void Busters_CheckGaugeEvent(void)
{
	// デバッグモード中は勝敗判定をスキップ
	if (DEBUG_BUSTERS_ROTATION) return;

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
					"asset\\model\\busters_v3.fbx"
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

// =================================================================
// 中継地点（ドア）のブラックリスト管理関数
// =================================================================

// リストに追加して、次回から選ばれないようにする
void Busters::AddIgnoreRelayDoor(int furnitureIndex)
{
	m_IgnoredDoorIndices.push_back(furnitureIndex);
}

// 目標に到着した時にリストを空にする
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
