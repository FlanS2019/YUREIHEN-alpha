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
#include "Floor1.h"

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11Buffer* g_IndexBuffer = NULL;

static ID3D11Buffer* g_SimpleVertexBuffer = NULL;
static ID3D11Buffer* g_SimpleIndexBuffer = NULL;

#define MAX_BLOCK_TYPES 100 
static ID3D11ShaderResourceView* g_BlockTextures[MAX_BLOCK_TYPES];
static ID3D11ShaderResourceView* g_TextureStairs; // 階段用

XMFLOAT3 rotateBox = XMFLOAT3(0, 0, 0);
static std::vector<MAPDATA> g_MapList;

static int g_CurrentFloor = 0;

// =====================================================================
// マップ設定
// =====================================================================

#undef MAP_W
#undef MAP_H
#define MAP_W (MAP_WIDTH)   // 53
#define MAP_H (MAP_LENGTH)  // 41 (Floor1.hに合わせて修正)


// 内部関数: 座標変換
static int WorldToGridX(float x) { return (int)round(x + MAP_W / 2.0f); }
static int WorldToGridZ(float z) { return (int)round(MAP_H / 2.0f - z); }
static float GridToWorldX(int gx) { return (float)gx - MAP_W / 2.0f; }
static float GridToWorldZ(int gz) { return MAP_H / 2.0f - (float)gz; }

FIELD_TYPE ConvertMapID(int minecraftID)
{
	switch (minecraftID)
	{
	case 0: return FIELD_NONE; // 空気

		// --- 箱（壁・床）として扱うもの ---
	case 1:  // トウヒの板材
	case 2:  // ダークオークの原木
	case 3:  // ダークオークの板材
	case 4:  // シラカバの板材
	case 13: // ドア（仮で壁扱い）
	case 14: // ダークオークフェンス
	case 15: // オークフェンス
	case 16: // ダイアモンド
	case 17: // カーペット
	case 39: // コピー用ブロック
		return FIELD_BOX;

		// --- 階段として扱うもの ---
	case 5: case 6: case 7: case 8: // 下付き階段
		return FIELD_STAIRS_UP;

	case 9: case 10: case 11: case 12: // 上付き階段
		return FIELD_STAIRS_DOWN;


	case 50: case 51: case 52: //家具
		return FIELD_NONE;
		// --- その他 (-1など) ---
	default:
		// IDが正の数ならとりあえず箱として表示
		if (minecraftID > 0) return FIELD_BOX;
		return FIELD_NONE;
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

				bool isVisible = false;

				// 上下左右前後をチェック (配列外参照に注意)
				// どこか一箇所でも「空」なら、そのブロックは見える可能性がある
				if (y + 1 >= MAP_HEIGHT || Floor1[y + 1][z][x] == 0) isVisible = true; // 上
				else if (y - 1 < 0 || Floor1[y - 1][z][x] == 0) isVisible = true;      // 下
				else if (z + 1 >= MAP_H || Floor1[y][z + 1][x] == 0) isVisible = true; // 手前
				else if (z - 1 < 0 || Floor1[y][z - 1][x] == 0) isVisible = true;      // 奥
				else if (x + 1 >= MAP_W || Floor1[y][z][x + 1] == 0) isVisible = true; // 右
				else if (x - 1 < 0 || Floor1[y][z][x - 1] == 0) isVisible = true;      // 左

				// どこからも見えないならリストに追加しない（描画しない）
				if (!isVisible) continue;

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

				data.blockID = mcID;

				// 階段の向き調整（必要であればIDを見て回転させる）
				// 例: if(mcID == 5) data.rotY = 90.0f; など

				g_MapList.push_back(data);
			}
		}
	}
	std::sort(g_MapList.begin(), g_MapList.end(), [](const MAPDATA& a, const MAPDATA& b) {
		return a.blockID < b.blockID;
		});
}

void Field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	for (int i = 0; i < MAX_BLOCK_TYPES; i++) g_BlockTextures[i] = nullptr;

	g_BlockTextures[1] = LoadTexture(L"asset\\texture\\yuka.png");  // ID 1
	g_BlockTextures[2] = LoadTexture(L"asset\\texture\\kabesita.png");   // ID 2
	g_BlockTextures[3] = LoadTexture(L"asset\\texture\\tunagime.png");// ID 3
	g_BlockTextures[4] = LoadTexture(L"asset\\texture\\kabeue.png");   // ID 4
	g_BlockTextures[13] = LoadTexture(L"asset\\texture\\wood.png");           // ID 13
	g_BlockTextures[14] = LoadTexture(L"asset\\texture\\wood.png");          // ID 14
	g_BlockTextures[16] = LoadTexture(L"asset\\texture\\green.png");  // ID 16
	g_BlockTextures[17] = LoadTexture(L"asset\\texture\\tairu.png"); // ID 17

	// デフォルト画像（読み込まれていないID用）
	if (g_BlockTextures[0] == nullptr) g_BlockTextures[0] = LoadTexture(L"asset\\texture\\grass.png");

	g_TextureStairs = LoadTexture(L"asset\\texture\\wood.png");

	g_CurrentFloor = 0; // 3階スタート
	LoadMapData(g_CurrentFloor);

	if (!g_MapList.empty()) {
		CreateBox(pDevice, pContext, &g_VertexBuffer, &g_IndexBuffer);
	
	}

	CreateSimpleBox(pDevice, &g_SimpleVertexBuffer, &g_SimpleIndexBuffer);
}

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

	float dx = playerPos.x - cameraPos.x;
	float dz = playerPos.z - cameraPos.z;
	float dist = sqrtf(dx * dx + dz * dz);

	if (dist < 0.5f) return;

	float stepX = dx / dist;
	float stepZ = dz / dist;

	float currentDist = 0.0f;

	while (currentDist < dist - 0.5f)
	{
		float checkX = cameraPos.x + stepX * currentDist;
		float checkZ = cameraPos.z + stepZ * currentDist;

		int gridX = WorldToGridX(checkX);
		int gridZ = WorldToGridZ(checkZ);

		if (gridX >= 0 && gridX < MAP_W && gridZ >= 0 && gridZ < MAP_H)
		{
			// Y=1 (壁) をチェック
			int mcID = Floor1[1][gridZ][gridX];

			if (ConvertMapID(mcID) == FIELD_BOX)
			{
				int range = 2;
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

	for (auto& mapData : g_MapList)
	{
		int mapGridX = WorldToGridX(mapData.pos.x);
		int mapGridZ = WorldToGridZ(mapData.pos.z);

		if (mapGridX >= 0 && mapGridX < MAP_W && mapGridZ >= 0 && mapGridZ < MAP_H)
		{
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
	int lastID = -999;
	Shader_Begin();

	XMMATRIX View = GetCamera()->GetView();
	XMMATRIX Projection = GetCamera()->GetProjection();
	XMMATRIX VP = View * Projection;

	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& mapData : g_MapList)
	{
		if (mapData.isHidden) continue;

		//カメラから遠すぎる場合は描画しない
		/*XMFLOAT3 cameraPos = GetCamera()->GetPos();
		float dx = mapData.pos.x - cameraPos.x;
		float dz = mapData.pos.z - cameraPos.z;

		if (dx * dx + dz * dz > 1600.0f) continue;
		*/

		int id = mapData.blockID;

		if (id != lastID)
		{
		// 階段の場合
		if (mapData.no == FIELD_STAIRS_UP || mapData.no == FIELD_STAIRS_DOWN)
		{
			g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
			g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			g_pContext->PSSetShaderResources(0, 1, &g_TextureStairs);
		}
		else
		{
			// 箱（壁・床）

			// テクスチャがロードされていない、または範囲外、またはID=0(デフォルト)の場合
			if (id <= 0 || id >= MAX_BLOCK_TYPES || g_BlockTextures[id] == nullptr)
			{
				g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
				g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
				g_pContext->PSSetShaderResources(0, 1, &g_BlockTextures[0]);
			}
			else
			{
				g_pContext->IASetVertexBuffers(0, 1, &g_SimpleVertexBuffer, &stride, &offset);
				g_pContext->IASetIndexBuffer(g_SimpleIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
				g_pContext->PSSetShaderResources(0, 1, &g_BlockTextures[id]);
			}
		}

		lastID = id;
		}

		XMMATRIX ScalingMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
		XMMATRIX TranslationMatrix = XMMatrixTranslation(mapData.pos.x, mapData.pos.y, mapData.pos.z);
		XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(rotateBox.x),
			XMConvertToRadians(rotateBox.y + mapData.rotY),
			XMConvertToRadians(rotateBox.z));

		XMMATRIX Model = ScalingMatrix * RotationMatrix * TranslationMatrix;
		XMMATRIX WVP = Model * VP;

		Shader_SetWorldMatrix(Model);
		Shader_SetMatrix(WVP);

		// 描画
		g_pContext->DrawIndexed(36, 0, 0);
	}
}
void Field_Finalize(void)
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_IndexBuffer);

	SAFE_RELEASE(g_SimpleVertexBuffer);
	SAFE_RELEASE(g_SimpleIndexBuffer);

	for (int i = 0; i < MAX_BLOCK_TYPES; i++)
	{
		SAFE_RELEASE(g_BlockTextures[i]);
	}
	SAFE_RELEASE(g_TextureStairs);
	g_MapList.clear();
}

void Field_ChangeFloor(int floorIndex)
{
	g_CurrentFloor = floorIndex;
	LoadMapData(g_CurrentFloor);
}

int Field_GetCurrentFloor(void)
{
	return g_CurrentFloor;
}

// ----------------------------------------------------------------
// 判定関数
// ----------------------------------------------------------------

FIELD_TYPE Field_GetBlockType(float x, float z)
{
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);

	if (gx >= 0 && gx < MAP_W && gz >= 0 && gz < MAP_H)
	{
		int mcID = Floor1[1][gz][gx];
		FIELD_TYPE type = ConvertMapID(mcID);

		if (type == FIELD_NONE)
		{
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
		// Y=1 チェック
		int mcID = Floor1[1][gz][gx];
		if (ConvertMapID(mcID) == FIELD_BOX) return true;

		// Y=2 チェック
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

		if (Field_IsWall(checkX, checkZ)) return true;

		currentDist += 0.5f;
	}
	return false;
}

float Field_GetFloorY(float x, float y, float z)
{
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);

	if (gx >= 0 && gx < MAP_W && gz >= 0 && gz < MAP_H)
	{
		if (Floor1[0][gz][gx] != 0) return 0.0f;
	}
	return -999.0f;
}

// ----------------------------------------------------------------
// 経路探索
// ----------------------------------------------------------------

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

	// ゴールが壁なら補正
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