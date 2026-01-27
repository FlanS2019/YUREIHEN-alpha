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
	m_Icon(nullptr)
{
	m_Icon = new Billboard();

	m_Icon->Initialize({ 0.0f, 0.0f, 0.0f }, { 0.7f, 0.7f }, { 0.0f, 0.0f, 0.0f }, true);
}

Busters::~Busters()
{
	if (m_Icon)
	{
		delete m_Icon;
		m_Icon = nullptr;
	}
}

void Busters::Update(void)
{
	JumpUpdate(*(Transform3D*)this);

	if (m_DetectionGraceTimer > 0)
	{
		m_DetectionGraceTimer--;
	}

	// アイコンの状態更新
	if (m_Icon)
	{
		// 状態に合わせてアイコンを一発切り替え
		switch (m_State)
		{
		case BUSTERS_SEARCH:    // 探索中
			m_Icon->SetIcon(BILLBOARD_ICON::NONE); // 何も出さない
			break;

		case BUSTERS_SUSPICION: // 警戒中（？）
		case BUSTERS_LURED:		// 誘引中もハテナ
			m_Icon->SetIcon(BILLBOARD_ICON::QUESTION);
			break;

		case BUSTERS_CHASE:     // 追跡中（！）
			m_Icon->SetIcon(BILLBOARD_ICON::ALERT);
			break;

		case BUSTERS_STUN:      // 気絶中ならSTUNアイコン
			m_Icon->SetIcon(BILLBOARD_ICON::STUN);
			break;
		}

		// 位置合わせ（頭上）
		XMFLOAT3 iconPos = m_Position;

		iconPos.y += 3.25f;

		m_Icon->SetPos(iconPos);
		m_Icon->Update();
	}

	if (m_WaitTimer > 0)
	{
		m_WaitTimer--;
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
				m_PathList = Field_FindPath(m_Position, targetFurniture->GetPos());

				// 経路が見つかった場合は反転（ゴール->スタートで返ってくるため）
				if (!m_PathList.empty())
				{
					std::reverse(m_PathList.begin(), m_PathList.end());
					m_PathList.erase(m_PathList.begin()); // 現在地のタイルをスキップ
				}

				if (m_PathList.empty() && targetFurniture)
				{
					// 同一タイル内の場合はそのまま目的地とする
					m_PathList.push_back(targetFurniture->GetPos());
				}

				// それでも空なら失敗
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
		break;

	case BUSTERS_SUSPICION: // 警戒
		if (GetGhost())
		{
			// 壁がない場合は直線的に進む
			if (!Field_CheckWallBetween(m_Position, GetGhost()->GetPos()))
			{
				m_PathList.clear();
				nextStepPos = GetGhost()->GetPos();
			}
			else
			{
				// 経路が空、またはターゲットに到達したら再計算
				if (m_PathList.empty())
				{
					m_PathList = Field_FindPath(m_Position, GetGhost()->GetPos());
					if (!m_PathList.empty()) {
						std::reverse(m_PathList.begin(), m_PathList.end());
						m_PathList.erase(m_PathList.begin()); // 現在地のタイルをスキップ
					}
				}
			}

			if (m_PathList.empty())
			{
				nextStepPos = GetGhost()->GetPos();
			}
		}
		m_MoveSpeed = BUSTERS_MOVE_SPEED_SUSPICION;
		break;

	case BUSTERS_CHASE: // 追跡
		if (GetGhost())
		{
			// 壁がない場合は直線的に追跡する
			if (!Field_CheckWallBetween(m_Position, GetGhost()->GetPos()))
			{
				m_PathList.clear();
				nextStepPos = GetGhost()->GetPos();
			}
			else
			{
				// 追跡中は最短経路を常に更新
				m_PathList = Field_FindPath(m_Position, GetGhost()->GetPos());
				if (!m_PathList.empty()) {
					std::reverse(m_PathList.begin(), m_PathList.end());
					m_PathList.erase(m_PathList.begin()); // 現在地のタイルをスキップ
				}
			}

			if (m_PathList.empty())
			{
				nextStepPos = GetGhost()->GetPos();
			}
		}
		m_MoveSpeed = BUSTERS_MOVE_SPEED_CHASE;
		break;
	}

	// 経路が存在する場合、次の目的地へ向かう
	if (!m_PathList.empty())
	{
		nextStepPos = m_PathList[0];

		float dx = nextStepPos.x - m_Position.x;
		float dz = nextStepPos.z - m_Position.z;
		float distSq = dx * dx + dz * dz;

		// 目的地に近づいたら次のノードへ
		if (distSq < 0.2f * 0.2f)
		{
			m_PathList.erase(m_PathList.begin());
			if (m_PathList.empty())
			{
				if (m_State == BUSTERS_SEARCH || m_State == BUSTERS_LURED)
				{
					m_State = BUSTERS_SEARCH;
					m_TargetFurnitureIndex = -1;
					m_WaitTimer = 120; // 到着後の待機
				}
			}
		}
	}

	MoveTo(nextStepPos);
}

void Busters::CheckState(void)
{
	Ghost* ghost = GetGhost();
	if (!ghost) return;
	if (m_WaitTimer > 0) return;
	if (m_DetectionGraceTimer > 0) return;

	XMFLOAT3 ghostPos = ghost->GetPos();
	XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
	XMVECTOR myVec = XMLoadFloat3(&m_Position);
	m_DistanceToGhost = XMVectorGetX(XMVector3Length(XMVectorSubtract(ghostVec, myVec)));

	// 変身中（憑依中）
	if (ghost->GetState() == GS_TRANSFORM || ghost->GetState() == GS_SCARE)
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

	bool hasWall = Field_CheckWallBetween(m_Position, ghostPos);

	if (!hasWall && m_DistanceToGhost < BUSTERS_PATROL_RANGH)
	{
		if (m_State != BUSTERS_CHASE)
		{
			m_State = BUSTERS_CHASE;
			this->SetColor(1.0f, 0.0f, 0.0f, 1.0f); // 赤
			ghost->SetIsDetectedByBuster(true);

			// 発見されたらコンボリセット
			//UI_ScareCombo_Reset();
		}

		// 距離が近づくにつれ恐怖ゲージを減らす（赤発見時のみ）
		if (m_DistanceToGhost < BUSTERS_PATROL_RANGH)
		{
			float reduceAmount = (1.0f - (m_DistanceToGhost / BUSTERS_PATROL_RANGH)) * -0.1f;
			AddScareGauge(reduceAmount);
		}
	}
	else if (!hasWall && m_DistanceToGhost < BUSTERS_SUSPICION_RANGE)
	{
		if (m_State != BUSTERS_SUSPICION)
		{
			m_State = BUSTERS_SUSPICION;
			this->SetColor(1.0f, 1.0f, 0.0f, 1.0f); // 黄
			ghost->SetIsDetectedByBuster(false);
		}
	}
	else
	{
		if (m_State != BUSTERS_SEARCH)
		{
			m_State = BUSTERS_SEARCH;
			this->ResetColor();
			ghost->SetIsDetectedByBuster(false);
			m_TargetFurnitureIndex = -1;
			m_WaitTimer = 30;
		}
	}
}

void Busters::MoveTo(XMFLOAT3 targetPos)
{
	if (GetIsJumping()) return;

	float dx = targetPos.x - m_Position.x;
	float dz = targetPos.z - m_Position.z;

	// 到着判定を移動速度に合わせて調整
	if (fabsf(dx) < m_MoveSpeed && fabsf(dz) < m_MoveSpeed) return;

	float len = sqrtf(dx * dx + dz * dz);
	if (len > 0)
	{
		dx /= len;
		dz /= len;
	}

	float angle = atan2f(dx, dz);
	float deg = XMConvertToDegrees(angle);
	SetRotY(deg + 180.0f);

	auto checkWallCollision = [&](float nx, float nz) -> bool {
		float r = 0.4f; // 当たり判定半径
		return (Field_IsWall(nx + r, m_Position.y, nz + r) ||
			Field_IsWall(nx + r, m_Position.y, nz - r) ||
			Field_IsWall(nx - r, m_Position.y, nz + r) ||
			Field_IsWall(nx - r, m_Position.y, nz - r));
		};

	// --- X軸移動 ---
	float nextX = m_Position.x + dx * m_MoveSpeed;
	if (!checkWallCollision(nextX, m_Position.z)) 
	{
		m_Position.x = nextX;
	}

	// --- Z軸移動 ---
	float nextZ = m_Position.z + dz * m_MoveSpeed;
	if (!checkWallCollision(m_Position.x, nextZ)) // 同じ関数を再利用！
	{
		m_Position.z = nextZ;
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
	m_DetectionGraceTimer = 120; // 2秒間（60FPS想定）発見されないようにする
	m_PathList = Field_FindPath(m_Position, targetPos);

	if (!m_PathList.empty())
	{
		std::reverse(m_PathList.begin(), m_PathList.end());
		m_PathList.erase(m_PathList.begin());
	}

	if (m_PathList.empty())
	{
		m_PathList.push_back(targetPos);
	}
}

void Busters::OnStopped(void)
{
	m_WaitTimer = 300;
	this->SetColor(0.5f, 0.0f, 0.5f, 1.0f); // 紫

	m_State = BUSTERS_STUN;
}

void Busters::SetIsGhostDiscover(bool discover)
{
	if (m_WaitTimer > 0) return;
	if (discover) this->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
	else this->ResetColor();
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

void BustersLured(XMFLOAT3 pos)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
	{
		for (Busters* buster : g_BustersList[currentFloor]) {
			buster->OnLured(pos);
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

	if (currentFloor > 0)
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