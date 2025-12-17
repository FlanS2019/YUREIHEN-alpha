#include "field.h"
#include "texture.h"
#include "Camera.h"
#include "sprite.h"
#include "box.h"
#include "define.h"
#include "ghost.h"
#include <vector>
#include <queue>
#include <map>
#include <cmath> 

// Minecraftのマップデータをインクルード
#include "levelmap.h" 

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11Buffer* g_IndexBuffer = NULL;

static ID3D11ShaderResourceView* g_TextureBox;   // 箱用
static ID3D11ShaderResourceView* g_TextureStairs;// 階段用

XMFLOAT3 rotateBox = XMFLOAT3(0, 0, 0);
static std::vector<MAPDATA> g_MapList;

static int g_CurrentFloor = 0;

// =====================================================================
// マップ設定
// =====================================================================

// サイズ定義を levelmap.h に合わせる
#undef MAP_W
#undef MAP_H
#define MAP_W (MAP_WIDTH)   // 53
#define MAP_H (MAP_LENGTH)  // 31


// 内部関数: 座標変換
static int WorldToGridX(float x) { return (int)round(x + MAP_W / 2.0f); }
static int WorldToGridZ(float z) { return (int)round(MAP_H / 2.0f - z); }
static float GridToWorldX(int gx) { return (float)gx - MAP_W / 2.0f; }
static float GridToWorldZ(int gz) { return MAP_H / 2.0f - (float)gz; }

// ID変換関数
FIELD_TYPE ConvertMapID(int minecraftID)
{
	switch (minecraftID)
	{
	case 5: return FIELD_BOX; // 外壁・床
	case 1: return FIELD_BOX; // 床・内壁
	case 2: return FIELD_BOX; // 壁
	case 3: return FIELD_STAIRS_UP;   // 階段（上り）
	case 4: return FIELD_STAIRS_DOWN; // 階段（下り）
	default: return FIELD_NONE;
	}
}

// =====================================================================
// マップ読み込み
// =====================================================================
void LoadMapData(int floor)
{
	g_MapList.clear();

	float offsetX = MAP_W / 2.0f;
	float offsetZ = MAP_H / 2.0f;

	// Minecraftデータの Y, Z, X ループ
	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int z = 0; z < MAP_H; z++)
		{
			for (int x = 0; x < MAP_W; x++)
			{
				// Floor1 を参照
				int mcID = Floor1[y][z][x];
				if (mcID == 0) continue;

				FIELD_TYPE type = ConvertMapID(mcID);
				if (type == FIELD_NONE) continue;

				MAPDATA data;

				// 座標計算 (Minecraft Y=1 -> Game Y=0.0)
				data.pos = XMFLOAT3(
					(x - offsetX),
					(float)y - 1.0f,
					(offsetZ - z)
				);

				data.no = type;
				data.isHidden = false;
				data.rotY = 0.0f;

				g_MapList.push_back(data);
			}
		}
	}
}

void Field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_TextureBox = LoadTexture(L"asset\\texture\\grass.png");
	g_TextureStairs = LoadTexture(L"asset\\texture\\wood.png");

	g_CurrentFloor = 2; // 3階スタート
	LoadMapData(g_CurrentFloor);

	if (!g_MapList.empty()) {
		CreateBox(pDevice, pContext, &g_VertexBuffer, &g_IndexBuffer);
	}
}

// LevelMapを使わず Floor1 を使うように変更
void Field_Update(void)
{

	static std::vector<std::vector<bool>> shouldHide(MAP_H, std::vector<bool>(MAP_W, false));

	// 配列のクリア
	for (int z = 0; z < MAP_H; z++) {
		for (int x = 0; x < MAP_W; x++) {
			shouldHide[z][x] = false;
		}
	}

	Ghost* ghost = GetGhost();
	if (!ghost) return;

	XMFLOAT3 cameraPos = GetCamera()->GetPos();
	XMFLOAT3 playerPos = ghost->GetPos();

	// プレイヤーの頭の高さ(Y+1.0f)を目標にする
	playerPos.y += 1.0f;

	// 3. レイキャスト (カメラ -> プレイヤー)
	float dx = playerPos.x - cameraPos.x;
	float dz = playerPos.z - cameraPos.z;
	float dist = sqrtf(dx * dx + dz * dz);

	// 近すぎる場合は計算不要
	if (dist < 0.5f) return;

	float stepX = dx / dist;
	float stepZ = dz / dist;

	float currentDist = 0.0f;

	while (currentDist < dist - 0.5f) // プレイヤーの手前0.5mまでチェック
	{
		float checkX = cameraPos.x + stepX * currentDist;
		float checkZ = cameraPos.z + stepZ * currentDist;

		int gridX = WorldToGridX(checkX);
		int gridZ = WorldToGridZ(checkZ);

		if (gridX >= 0 && gridX < MAP_W && gridZ >= 0 && gridZ < MAP_H)
		{
			// Floor1[1][z][x] (Y=1: 目の高さの壁) をチェック
			int mcID = Floor1[1][gridZ][gridX];

			// 壁(FIELD_BOX)があれば隠す
			if (ConvertMapID(mcID) == FIELD_BOX)
			{

				int range = 2; // 1 = 3x3マス, 2 = 5x5マス

				for (int oz = -range; oz <= range; oz++)
				{
					for (int ox = -range; ox <= range; ox++)
					{
						int targetX = gridX + ox;
						int targetZ = gridZ + oz;

						if (targetX >= 0 && targetX < MAP_W && targetZ >= 0 && targetZ < MAP_H)
						{
							shouldHide[targetZ][targetX] = true;
						}
					}
				}
			}
		}

		currentDist += 0.1f;
	}

	// 4. マップデータに反映
	for (auto& mapData : g_MapList)
	{
		int mapGridX = WorldToGridX(mapData.pos.x);
		int mapGridZ = WorldToGridZ(mapData.pos.z);

		if (mapGridX >= 0 && mapGridX < MAP_W && mapGridZ >= 0 && mapGridZ < MAP_H)
		{
			// 床(Y<0)は消さない。壁(Y>=0)で、かつフラグが立っていたら消す
			if (mapData.pos.y >= 0.0f && shouldHide[mapGridZ][mapGridX])
			{
				mapData.isHidden = true;
			}
			else
			{
				mapData.isHidden = false;
			}
		}
	}
}
void Field_Draw(void)
{
	Shader_Begin();

	XMMATRIX View = GetCamera()->GetView();
	XMMATRIX Projection = GetCamera()->GetProjection();
	XMMATRIX VP = View * Projection;

	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& mapData : g_MapList)
	{
		if (mapData.isHidden) continue;

		if (mapData.no == FIELD_STAIRS_UP || mapData.no == FIELD_STAIRS_DOWN)
		{
			g_pContext->PSSetShaderResources(0, 1, &g_TextureStairs);
		}
		else
		{
			g_pContext->PSSetShaderResources(0, 1, &g_TextureBox);
		}

		XMMATRIX ScalingMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
		XMMATRIX TranslationMatrix = XMMatrixTranslation(mapData.pos.x, mapData.pos.y, mapData.pos.z);
		XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(rotateBox.x),
			XMConvertToRadians(rotateBox.y),
			XMConvertToRadians(rotateBox.z));

		XMMATRIX Model = ScalingMatrix * RotationMatrix * TranslationMatrix;
		XMMATRIX WVP = Model * VP;

		Shader_SetWorldMatrix(Model);
		Shader_SetMatrix(WVP);

		g_pContext->DrawIndexed(6 * 6, 0, 0);
	}
}

void Field_Finalize(void)
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_IndexBuffer);
	SAFE_RELEASE(g_TextureBox);
	SAFE_RELEASE(g_TextureStairs);
	g_MapList.clear();
}

void Field_ChangeFloor(int floorIndex)
{
	// 今回は1フロアデータしかないので何もしないか、
	// floorIndexに応じて Floor2, Floor3 を読むように拡張する
	g_CurrentFloor = floorIndex;
	LoadMapData(g_CurrentFloor);
}

int Field_GetCurrentFloor(void)
{
	return g_CurrentFloor;
}

// ----------------------------------------------------------------
// 判定関数 (Floor1 を参照するように修正)
// ----------------------------------------------------------------

FIELD_TYPE Field_GetBlockType(float x, float z)
{
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);

	if (gx >= 0 && gx < MAP_W && gz >= 0 && gz < MAP_H)
	{
		// Y=1 (壁) を優先チェック
		int mcID = Floor1[1][gz][gx];
		FIELD_TYPE type = ConvertMapID(mcID);

		if (type == FIELD_NONE)
		{
			// Y=2 (上の壁/階段など) もチェック
			mcID = Floor1[2][gz][gx];
			type = ConvertMapID(mcID);
		}
		return type;
	}
	return FIELD_NONE;
}

bool Field_IsWall(float x, float z)
{
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);

	if (gx >= 0 && gx < MAP_W && gz >= 0 && gz < MAP_H)
	{
		// Y=1 に壁があれば true
		int mcID = Floor1[1][gz][gx];
		if (ConvertMapID(mcID) == FIELD_BOX) return true;

		// Y=2 もチェック
		int mcID2 = Floor1[2][gz][gx];
		if (ConvertMapID(mcID2) == FIELD_BOX) return true;
	}
	return false;
}

bool Field_IsOuterWall(float x, float z)
{
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);

	if (gx < 0 || gx >= MAP_W || gz < 0 || gz >= MAP_H) return true;
	if (gx == 0 || gx == MAP_W - 1 || gz == 0 || gz == MAP_H - 1) return true;

	return false;
}

bool Field_IsWall(float x, float y, float z)
{
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);


	int gy = (int)round(y + 1.0f);

	if (gx >= 0 && gx < MAP_W && gz >= 0 && gz < MAP_H && gy >= 0 && gy < MAP_HEIGHT)
	{
		int mcID = Floor1[gy][gz][gx];
		if (ConvertMapID(mcID) != FIELD_NONE) return true;
	}
	return false;
}

bool Field_CheckWallBetween(XMFLOAT3 start, XMFLOAT3 end)
{
	float dx = end.x - start.x;
	float dz = end.z - start.z;
	float dist = sqrtf(dx * dx + dz * dz);

	if (dist < 0.5f) return false;

	float stepX = dx / dist;
	float stepZ = dz / dist;
	float currentDist = 0.5f;

	while (currentDist < dist - 0.5f)
	{
		float checkX = start.x + stepX * currentDist;
		float checkZ = start.z + stepZ * currentDist;

		if (Field_IsWall(checkX, checkZ))
		{
			return true;
		}

		currentDist += 0.5f;
	}

	return false;
}

float Field_GetFloorY(float x, float y, float z)
{
	// 床があるか簡易判定
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);

	if (gx >= 0 && gx < MAP_W && gz >= 0 && gz < MAP_H)
	{
		// Minecraft Y=0 (Game Y=-1.0) にブロックがあれば床高さ0.0fとみなす
		if (Floor1[0][gz][gx] != 0) return 0.0f;
	}
	return -999.0f;
}

// =========================================================
// 経路探索 (A*)
// =========================================================

struct Node {
	int x, z;
	float cost;
	float heuristic;
	int parentX, parentZ;
	bool operator>(const Node& other) const {
		return (cost + heuristic) > (other.cost + other.heuristic);
	}
};

std::vector<XMFLOAT3> Field_FindPath(XMFLOAT3 start, XMFLOAT3 end)
{
	std::vector<XMFLOAT3> path;

	int startX = WorldToGridX(start.x);
	int startZ = WorldToGridZ(start.z);
	int endX = WorldToGridX(end.x);
	int endZ = WorldToGridZ(end.z);

	if (startX < 0 || startX >= MAP_W || startZ < 0 || startZ >= MAP_H ||
		endX < 0 || endX >= MAP_W || endZ < 0 || endZ >= MAP_H)
	{
		return path;
	}

	// ゴールが壁なら補正 (Floor1 を参照)
	if (ConvertMapID(Floor1[1][endZ][endX]) == FIELD_BOX)
	{
		int dx[] = { 0, 0, 1, -1 };
		int dz[] = { 1, -1, 0, 0 };
		for (int i = 0; i < 4; i++) {
			int nx = endX + dx[i];
			int nz = endZ + dz[i];
			if (nx >= 0 && nx < MAP_W && nz >= 0 && nz < MAP_H) {
				if (ConvertMapID(Floor1[1][nz][nx]) != FIELD_BOX) {
					endX = nx; endZ = nz; break;
				}
			}
		}
	}

	std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;
	std::vector<std::vector<bool>> closedList(MAP_H, std::vector<bool>(MAP_W, false));
	std::vector<std::vector<Node>> nodes(MAP_H, std::vector<Node>(MAP_W));

	Node startNode = { startX, startZ, 0.0f, 0.0f, -1, -1 };
	openList.push(startNode);
	nodes[startZ][startX] = startNode;

	int dirX[] = { 0, 0, -1, 1 };
	int dirZ[] = { -1, 1, 0, 0 };
	bool found = false;

	while (!openList.empty())
	{
		Node current = openList.top();
		openList.pop();

		if (current.x == endX && current.z == endZ)
		{
			found = true;
			break;
		}

		if (closedList[current.z][current.x]) continue;
		closedList[current.z][current.x] = true;

		for (int i = 0; i < 4; i++)
		{
			int nextX = current.x + dirX[i];
			int nextZ = current.z + dirZ[i];

			if (nextX < 0 || nextX >= MAP_W || nextZ < 0 || nextZ >= MAP_H) continue;

			// 壁判定 (Floor1[1][z][x])
			if (ConvertMapID(Floor1[1][nextZ][nextX]) == FIELD_BOX) continue;

			if (closedList[nextZ][nextX]) continue;

			float newCost = current.cost + 1.0f;
			float h = (float)(std::abs(endX - nextX) + std::abs(endZ - nextZ));

			Node neighbor = { nextX, nextZ, newCost, h, current.x, current.z };
			openList.push(neighbor);

			if (nodes[nextZ][nextX].parentX == 0 && nodes[nextZ][nextX].parentZ == 0)
			{
				nodes[nextZ][nextX] = neighbor;
			}
		}
	}

	if (found)
	{
		int cx = endX;
		int cz = endZ;
		int maxSteps = MAP_W * MAP_H;
		int steps = 0;

		while (cx != -1 && cz != -1 && steps < maxSteps)
		{
			path.push_back({ GridToWorldX(cx), 0.0f, GridToWorldZ(cz) });
			Node& n = nodes[cz][cx];
			if (cx == startX && cz == startZ) break;
			cx = n.parentX;
			cz = n.parentZ;
			steps++;
		}
	}

	return path;
}