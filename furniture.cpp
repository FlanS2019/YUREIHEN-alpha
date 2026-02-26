#include "furniture.h"
#include "Camera.h"
#include "shader.h"
#include "ghost.h"
#include "keyboard.h"
#include "define.h"
#include "Floor1.h"
#include "Floor2.h"
#include "Floor3.h"
#include "field.h"
#include <cmath>   // sinf用
#include <fstream>
#include <map>
#include <string>
#include <vector>

Furniture* g_Furniture[FURNITURE_NUM]{};

static int g_FurnitureCount = 0;

static std::map<int, std::string> g_BlockNameJaMap;
static bool g_FileLoadFailed = false; // 読み込み失敗フラグ
static bool g_BlockDefinitionsLoaded = false; // 読み込み完了フラグ

// ブロック定義構造体（モデル・アクション情報を保持）
struct BlockDefinition
{
	std::string names_ja;
	std::string model;   // fbxモデルパス（空文字列なら家具ではない）
	std::string action;  // "scare", "lure", "stop", "none"（空文字列なら家具ではない）
};

static std::map<int, BlockDefinition> g_BlockDefMap;

// アクション文字列を FURNITURE_ACTION に変換
static FURNITURE_ACTION ParseAction(const std::string& actionStr)
{
	if (actionStr == "scare") return ACTION_SCARE;
	if (actionStr == "lure")  return ACTION_LURE;
	if (actionStr == "stop")  return ACTION_STOP;
	return ACTION_NONE;
}

// block_definitions.json を読み込み、IDと定義情報のマップを作成する
static void LoadBlockDefinitions()
{
	if (g_BlockDefinitionsLoaded) return;
	g_BlockDefinitionsLoaded = true;

	std::ifstream file("SchemToArray\\block_definitions.json");
	if (!file.is_open())
	{
		g_FileLoadFailed = true;
		return;
	}

	// エラー回避のため、明示的に文字列へ読み込む
	std::string content;
	file.seekg(0, std::ios::end);
	content.reserve(static_cast<size_t>(file.tellg()));
	file.seekg(0, std::ios::beg);
	content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	// JSONの文字列値を抽出するヘルパー
	auto findStringValue = [](const std::string& obj, const char* key) -> std::string {
		std::string searchKey = std::string("\"") + key + "\"";
		size_t npos = obj.find(searchKey);
		if (npos == std::string::npos) return "";
		size_t colon = obj.find(':', npos);
		if (colon == std::string::npos) return "";
		size_t s = obj.find('"', colon);
		if (s == std::string::npos) return "";
		size_t e = obj.find('"', s + 1);
		if (e == std::string::npos) return "";
		return obj.substr(s + 1, e - s - 1);
	};

	// JSONの数値を抽出するヘルパー
	auto findIntValue = [](const std::string& obj, const char* key) -> int {
		std::string searchKey = std::string("\"") + key + "\"";
		size_t ipos = obj.find(searchKey);
		if (ipos == std::string::npos) return -1;
		size_t colon = obj.find(':', ipos);
		if (colon == std::string::npos) return -1;
		size_t s = obj.find_first_of("0123456789", colon);
		if (s == std::string::npos) return -1;
		size_t e = obj.find_first_not_of("0123456789", s);
		std::string idStr = (e == std::string::npos) ? obj.substr(s) : obj.substr(s, e - s);
		if (idStr.empty()) return -1;
		return std::stoi(idStr);
	};

	size_t pos = 0;
	while ((pos = content.find('{', pos)) != std::string::npos)
	{
		size_t level = 1;
		size_t endPos = pos + 1;
		while (level > 0 && endPos < content.length())
		{
			if (content[endPos] == '{') level++;
			else if (content[endPos] == '}') level--;
			endPos++;
		}

		if (level == 0)
		{
			std::string object = content.substr(pos, endPos - pos);

			std::string names_ja = findStringValue(object, "names_ja");
			std::string model = findStringValue(object, "model");
			std::string action = findStringValue(object, "action");

			// id または default_id を探す
			int id = findIntValue(object, "id");
			if (id == -1) id = findIntValue(object, "default_id");

			if (id != -1 && !names_ja.empty())
			{
				// 後方互換: g_BlockNameJaMap にも保存
				g_BlockNameJaMap[id] = names_ja;

				// 新しい定義マップにも保存
				BlockDefinition def;
				def.names_ja = names_ja;
				def.model = model;
				def.action = action;
				g_BlockDefMap[id] = def;
			}
		}
		// 次の '{' から探索を続ける（入れ子構造にも対応）
		pos++;
	}
}

int GetFurnitureBlockID(int floor, int y, int z, int x)
{
	// 範囲チェック
	if (y < 0 || y >= MAP_HEIGHT || z < 0 || z >= MAP_LENGTH || x < 0 || x >= MAP_WIDTH) return 0;

	switch (floor)
	{
	case 0: return Floor1[y][z][x];
	case 1: return Floor2[y][z][x];
	case 2: return Floor3[y][z][x];
	default: return 0;
	}
}

// =========================================================
// 家具の配置 (Initialize)
// =========================================================
void Furniture_Initialize(void)
{
	Furniture_Finalize();

	// ブロック定義を読み込む（初回のみ）
	LoadBlockDefinitions();

	// 1. カウントをリセット
	g_FurnitureCount = 0;

	int currentFloor = Field_GetCurrentFloor();

	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int z = 0; z < MAP_LENGTH; z++)
		{
			for (int x = 0; x < MAP_WIDTH; x++)
			{

				int id = GetFurnitureBlockID(currentFloor, y, z, x);

				// ブロック定義にモデルが登録されている場合のみ家具として生成
				if (g_BlockDefMap.count(id) > 0 && !g_BlockDefMap[id].model.empty())
				{
					// ワールド座標に変換
					float wx = (float)x - MAP_WIDTH / 2.0f;
					float wz = MAP_LENGTH / 2.0f - (float)z;
					float wy = (float)y - 1.0f;

					float rotY = Field_CalculateRotationFromMarker(wx, wy, wz);

					const BlockDefinition& def = g_BlockDefMap[id];
					FURNITURE_ACTION action = ParseAction(def.action);

					CreateFurniture(
						{ wx, wy, wz },
						{ 1.0f, 1.0f, 1.0f },
						{ 0.0f, rotY, 0.0f },
						def.model.c_str(),
						action,
						id
					);
				}
			}
		}
	}
}

// =========================================================
// 家具クラスのメソッド実装
// =========================================================

void Furniture::Update(void)
{
	// 0. クールタイムの更新
	if (m_CooldownTimer > 0.0f)
	{
		m_CooldownTimer -= 1.0f / 60.0f;
		if (m_CooldownTimer <= 0.0f)
		{
			m_CooldownTimer = 0.0f;
			ResetColor(); // クールタイム終了で色を戻す
		}
		else
		{
			SetColor(0.0f, 0.0f, 1.0f, 1.0f); // クールタイム中は青色
		}
	}

	// 1. Ghostとの距離計算
	Ghost* pGhost = GetGhost();
	if (pGhost)
	{
		XMFLOAT3 ghostPos = pGhost->GetPos();
		XMFLOAT3 furniturePos = GetPos();

		// Y軸を除いた水平面での距離計算
		float dx = furniturePos.x - ghostPos.x;
		float dz = furniturePos.z - ghostPos.z;
		m_DistanceToGhost = sqrtf(dx * dx + dz * dz);

		// Y軸方向の距離制限チェック
		float dy = fabsf(furniturePos.y - ghostPos.y);
		if (dy > 50.0f)
		{
			// Y軸が50を超えていれば検知範囲外の距離として設定
			m_DistanceToGhost = FURNITURE_DETECTION_RANGE * 2.0f;
		}
	}

	// 2. アクション実行時の処理 (ビジュアル変化)
	if (m_IsActing || GetIsJumping())
	{

		switch (m_ActionType)
		{
		case ACTION_SCARE: // 驚かせ -> ジャンプ
			JumpUpdate(*(Transform3D*)this);
			break;

		case ACTION_LURE: // 誘引する -> 揺れ
		{
			m_ActionTimer += 1.0f;
			// 揺れ計算
			float shakeAmount = 0.1f;
			float offsetX = sinf(m_ActionTimer * 2.0f) * shakeAmount;

			// 減衰
			float decay = 1.0f - (m_ActionTimer / 60.0f);
			if (decay < 0.0f) decay = 0.0f;
			offsetX *= decay;

			SetPosX(m_BasePos.x + offsetX);

			if (m_ActionTimer >= 60.0f)
			{
				m_IsActing = false;
				SetPos(m_BasePos);
			}
		}
		break;

		case ACTION_STOP: // 停止中 -> 回転
		{
			m_ActionTimer += 1.0f;
			// 回転計算
			float speed = 30.0f;
			AddRotY(speed);

			if (m_ActionTimer >= 60.0f)
			{
				m_IsActing = false;
			}
		}
		break;
		}
	}

	// ビルボードの更新
	XMFLOAT3 pos = GetPos();
	XMFLOAT3 rot = GetRot();
	XMFLOAT3 size = GetDisplaySize();

	// マーカー（ID:98）がある方向を向くようにオフセットを計算
	// field.cpp の Field_CalculateRotationFromMarker の戻り値に対応する正面方向を算出
	// 0度: -Z方向, 180度: +Z方向, 90度: -X方向, 270度: +X方向
	float rad = XMConvertToRadians(rot.y);
	float dirX = -sinf(rad); // 90度で -1, 270度で 1
	float dirZ = -cosf(rad); // 0度で -1, 180度で 1

	// モデルの厚み（size.z）の半分より少し手前に配置
	float offsetDist = (size.z * 0.5f) + 0.4f;

	pos.x += dirX * offsetDist;
	pos.z += dirZ * offsetDist;
	pos.y += size.y * 0.5f; // 家具の高さの中央付近に配置

	m_Billboard.SetPos(pos);

	// 距離判定 (Ghost検出範囲の1.5倍 未満かつクールタイム中でないかつアクション中でない場合に表示)
	if (m_DistanceToGhost < (FURNITURE_DETECTION_RANGE * 1.5f) && !IsCoolingDown() && !GetIsActing())
	{
		switch (m_ActionType)
		{
		case ACTION_SCARE: m_Billboard.SetIcon(BILLBOARD_ICON::ALERT);    break;
		case ACTION_LURE:  m_Billboard.SetIcon(BILLBOARD_ICON::QUESTION); break;
		case ACTION_STOP:  m_Billboard.SetIcon(BILLBOARD_ICON::STUN);     break;
		default:           m_Billboard.SetIcon(BILLBOARD_ICON::NONE);     break;
		}
	}
	else
	{
		m_Billboard.SetIcon(BILLBOARD_ICON::NONE);
	}

	m_Billboard.Update();
}

void Furniture::Draw(void)
{


	if (m_CooldownTimer > 0.0f)
	{
		// クールタイム中は青
		SetColor(0.0f, 0.0f, 1.0f, 1.0f);
	}
	else if (m_IsTargeted)
	{
		// バスターズが狙っているなら赤
		SetColor(1.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (m_IsGhostTarget)
	{
		// 幽霊が変身できるなら緑
		SetColor(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else
	{
		// それ以外は元の色
		ResetColor();
	}

	Sprite3D::Draw();

	// シェーダーの状態をリセット（Billboard 描画の前に必要）
	Shader_RefreshState();

	m_Billboard.Draw();
}

void Furniture::StartAction(void)
{
	if (GetIsActing() || IsCoolingDown()) return; // 二重実行およびクールタイム中の実行を抑止

	if (m_ActionType == ACTION_SCARE)
	{
		JumpStart(); // ジャンプフラグON
	}
	else
	{
		m_IsActing = true; // 行う、アクション開始フラグON
		m_ActionTimer = 0.0f;
	}

	m_CooldownTimer = 10.0f; // 10秒間のクールタイムを設定
}

void CreateFurniture(XMFLOAT3 pos, XMFLOAT3 scale, XMFLOAT3 rot, const char* modelPath, FURNITURE_ACTION action, int blockID)
{
	// 配列がいっぱいなら何もしない（エラー防止）
	if (g_FurnitureCount >= FURNITURE_NUM) return;

	// 配列の「次の空いている場所」に家具を作る
	g_Furniture[g_FurnitureCount] = new Furniture(pos, scale, rot, modelPath, action, blockID);

	// 地面の高さをセット
	g_Furniture[g_FurnitureCount]->SetGroundLevel(pos.y);

	// カウントを進める
	g_FurnitureCount++;
}

// =========================================================
// グローバル関数の実装
// =========================================================

void Furniture_Update(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i])
		{
			g_Furniture[i]->SetIsTargeted(false);
			g_Furniture[i]->SetIsGhostTarget(false);
		}
	}

	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i]) g_Furniture[i]->Update();
	}
}

void Furniture_Draw(void)
{
	Camera* pCamera = GetCamera();
	if (!pCamera) return;

	XMFLOAT3 cameraPos = pCamera->GetPos();
	XMFLOAT3 cameraAt = pCamera->GetAtPos();
	XMVECTOR vCamPos = XMLoadFloat3(&cameraPos);
	XMVECTOR vCamAt = XMLoadFloat3(&cameraAt);
	XMVECTOR vForward = XMVector3Normalize(XMVectorSubtract(vCamAt, vCamPos));

	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i])
		{
			XMFLOAT3 pos = g_Furniture[i]->GetPos();
			XMVECTOR vPos = XMLoadFloat3(&pos);
			XMVECTOR vToPos = XMVectorSubtract(vPos, vCamPos);

			// 距離によるカリング (fieldと同じく50ユニット)
			float distSq;
			XMStoreFloat(&distSq, XMVector3LengthSq(vToPos));
			if (distSq > 2500.0f) continue;

			// 背面カリング (カメラの向きに対して後ろにある家具は描画しない)
			float dot;
			XMStoreFloat(&dot, XMVector3Dot(vToPos, vForward));
			if (dot < -2.0f) continue; // 余裕を持たせて-2.0f

			g_Furniture[i]->Draw();
		}
	}

	// 全家具の描画完了後、シェーダーの状態をリセット
	Shader_RefreshState();
}

void Furniture_Finalize(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i]) { delete g_Furniture[i]; g_Furniture[i] = nullptr; }
	}
}

Furniture* GetFurniture(int index)
{
	if (index >= 0 && index < FURNITURE_NUM) return g_Furniture[index];
	return nullptr;
}

// 与えられた数字のIDから日本語名を返す
std::string GetBlockNameJa(int id)
{
	if (!g_BlockDefinitionsLoaded && !g_FileLoadFailed)
	{
		LoadBlockDefinitions();
	}

	if (g_FileLoadFailed)
	{
		return "ファイルが見つかりませんでした";
	}

	if (g_BlockNameJaMap.count(id) > 0)
	{
		return g_BlockNameJaMap[id];
	}

	return "不明";
}

bool FurnitureScareStart(int index)
{
	if (index >= 0 && index < FURNITURE_NUM && g_Furniture[index])
	{
		g_Furniture[index]->StartAction();
		return true;
	}
	return false;
}

bool FurnitureScareEnded(int index)
{
	if (index >= 0 && index < FURNITURE_NUM && g_Furniture[index])
	{
		return !g_Furniture[index]->GetIsActing();
	}
	return false;
}

bool IsFurnitureBlock(int id)
{
	if (!g_BlockDefinitionsLoaded && !g_FileLoadFailed)
	{
		LoadBlockDefinitions();
	}
	if (g_BlockDefMap.count(id) > 0 && !g_BlockDefMap[id].model.empty())
	{
		return true;
	}
	return false;
}