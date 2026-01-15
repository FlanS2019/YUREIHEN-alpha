#include "field.h"
#include "texture.h"
#include "Camera.h"
#include "sprite.h"
#include "box.h"
#include "define.h"
#include "ghost.h"
#include <DirectXMath.h>
#include <vector>
#include <queue>
#include <map>
#include <cmath> 

#include "Floor1.h"

using namespace DirectX;

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11Buffer* g_IndexBuffer = NULL;

// インスタンス描画用バッファ
#define MAX_INSTANCES (5000)
static ID3D11Buffer* g_InstanceBuffer = NULL;

#define MAX_BLOCK_TYPES 100 
static ID3D11ShaderResourceView* g_BlockTextures[MAX_BLOCK_TYPES];
static ID3D11ShaderResourceView* g_TextureStairs;

XMFLOAT3 rotateBox = XMFLOAT3(0, 0, 0);
static std::vector<MAPDATA> g_MapList;

static int g_CurrentFloor = 0;

#undef MAP_W
#undef MAP_H
#define MAP_W (MAP_WIDTH)
#define MAP_H (MAP_LENGTH)

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
	case 18: // 窓ガラス
	case 39: // コピー用ブロック
		return FIELD_BOX;

		// --- 階段として扱うもの ---
	case 5: case 6: case 7: case 8: // 下付き階段
		return FIELD_STAIRS_UP;

	case 9: case 10: case 11: case 12: // 上付き階段
		return FIELD_STAIRS_DOWN;
	case 50: case 51: case 52:case 53:case 54:case 55:case 56:case 58:case 57: //家具
		return FIELD_NONE;
	default:
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

	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int z = 0; z < MAP_H; z++)
		{
			for (int x = 0; x < MAP_W; x++)
			{
				int mcID = Floor1[y][z][x];
				if (mcID == 0) continue;

				FIELD_TYPE type = ConvertMapID(mcID);
				if (type == FIELD_NONE) continue;

				bool isVisible = false;

				if (y + 1 >= MAP_HEIGHT || Floor1[y + 1][z][x] == 0) isVisible = true;
				else if (y - 1 < 0 || Floor1[y - 1][z][x] == 0) isVisible = true;
				else if (z + 1 >= MAP_H || Floor1[y][z + 1][x] == 0) isVisible = true;
				else if (z - 1 < 0 || Floor1[y][z - 1][x] == 0) isVisible = true;
				else if (x + 1 >= MAP_W || Floor1[y][z][x + 1] == 0) isVisible = true;
				else if (x - 1 < 0 || Floor1[y][z][x - 1] == 0) isVisible = true;

				if (!isVisible) continue;

				MAPDATA data;

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

	g_BlockTextures[1] = LoadTexture(L"asset\\texture\\yuka.png");		 // ID 1
	g_BlockTextures[2] = LoadTexture(L"asset\\texture\\kabesita.png");   // ID 2
	g_BlockTextures[3] = LoadTexture(L"asset\\texture\\tunagime.png");	 // ID 3
	g_BlockTextures[4] = LoadTexture(L"asset\\texture\\kabeue.png");	 // ID 4
	g_BlockTextures[13] = LoadTexture(L"asset\\texture\\wood.png");      // ID 13
	g_BlockTextures[14] = LoadTexture(L"asset\\texture\\wood.png");      // ID 14
	g_BlockTextures[16] = LoadTexture(L"asset\\texture\\green.png");	 // ID 16
	g_BlockTextures[17] = LoadTexture(L"asset\\texture\\tairu.png");	 // ID 17
	g_BlockTextures[18] = LoadTexture(L"asset\\texture\\garasu.png");	 // ID 18

	if (g_BlockTextures[0] == nullptr) g_BlockTextures[0] = LoadTexture(L"asset\\texture\\grass.png");

	g_TextureStairs = LoadTexture(L"asset\\texture\\wood.png");

	g_CurrentFloor = 0; // 3階スタート
	LoadMapData(g_CurrentFloor);

	if (!g_MapList.empty()) {
		CreateBox(pDevice, pContext, &g_VertexBuffer, &g_IndexBuffer);
	}

	// インスタンスバッファの作成
	D3D11_BUFFER_DESC idesc{};
	idesc.ByteWidth = sizeof(XMFLOAT4X4) * MAX_INSTANCES;
	idesc.Usage = D3D11_USAGE_DYNAMIC;
	idesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	idesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	pDevice->CreateBuffer(&idesc, NULL, &g_InstanceBuffer);
}

void Field_Update(void)
{
	static uint8_t shouldHide[MAP_LENGTH][MAP_WIDTH];
	memset(shouldHide, 0, sizeof(shouldHide));

	Ghost* ghost = GetGhost();

	if (!ghost) return;

	XMFLOAT3 cameraPos = GetCamera()->GetPos();
	XMFLOAT3 playerPos = ghost->GetPos();

	playerPos.y += 1.0f;

	float dx = playerPos.x - cameraPos.x;
	float dz = playerPos.z - cameraPos.z;
	float distSq = dx * dx + dz * dz;

	// 距離が近すぎる場合はスキップ
	if (distSq < 0.25f) return;

	// 極端に遠い場合は処理を制限（無限ループ防止）
	const float MAX_DIST = 30.0f; // 50から30に短縮
	float dist = sqrtf(distSq);
	if (dist > MAX_DIST) dist = MAX_DIST;

	float invDist = 1.0f / dist;
	float stepX = dx * invDist;
	float stepZ = dz * invDist;

	// ステップを1.0fに変更（0.5fから）して処理回数を半減
	const float STEP_SIZE = 1.0f;
	float endDist = dist - 0.5f;

	// レイを飛ばす回数を制限
	int rayCount = 0;
	const int MAX_RAYS = 50;

	for (float currentDist = 0.0f; currentDist < endDist && rayCount < MAX_RAYS; currentDist += STEP_SIZE)
	{
		rayCount++;
		float checkX = cameraPos.x + stepX * currentDist;
		float checkZ = cameraPos.z + stepZ * currentDist;

		int gridX = WorldToGridX(checkX);
		int gridZ = WorldToGridZ(checkZ);

		if (gridX >= 0 && gridX < MAP_W && gridZ >= 0 && gridZ < MAP_H)
		{
			// Y=1 (壁レイヤー) をチェック
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
							shouldHide[targetZ][targetX] = 1;
						}
					}
				}
			}
		}
		currentDist += 0.1f; // 少しずつ進める
	}

	// フラグに基づいてブロックの表示/非表示を更新
	for (auto& mapData : g_MapList)
	{
		int mapGridX = WorldToGridX(mapData.pos.x);
		int mapGridZ = WorldToGridZ(mapData.pos.z);

		if (mapGridX >= 0 && mapGridX < MAP_W && mapGridZ >= 0 && mapGridZ < MAP_H)
		{
			mapData.isHidden = (mapData.pos.y >= 0.0f && shouldHide[mapGridZ][mapGridX]);
		}
	}
}

// 定数バッファのキャッシュ用
static DirectX::XMFLOAT4X4 g_CachedWVP;
static DirectX::XMFLOAT4X4 g_CachedWorld;

void Field_Draw(void)
{
	Shader_BeginInstance();

	// 描画トポロジーをインデックスデータに合わせて三角形リストに設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Camera* pCamera = GetCamera();
	XMMATRIX View = pCamera->GetView();
	XMMATRIX Projection = pCamera->GetProjection();
	XMMATRIX VP = View * Projection;

	// インスタンス描画用に ViewProjection 行列をセット
	Shader_SetMatrix(VP);

	XMFLOAT3 cameraPos = pCamera->GetPos();
	XMFLOAT3 cameraAtPos = pCamera->GetAtPos();
	
	XMVECTOR lookVec = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&cameraAtPos), XMLoadFloat3(&cameraPos)));
	XMFLOAT3 look;
	XMStoreFloat3(&look, lookVec);

	// 回転計算（一度だけ）
	float radX = XMConvertToRadians(rotateBox.x);
	float radY = XMConvertToRadians(rotateBox.y);
	float radZ = XMConvertToRadians(rotateBox.z);
	bool hasBaseRot = (rotateBox.x != 0.0f || rotateBox.y != 0.0f || rotateBox.z != 0.0f);
	XMMATRIX baseRotMtx = hasBaseRot ? XMMatrixRotationRollPitchYaw(radX, radY, radZ) : XMMatrixIdentity();

	// テクスチャごとにグループ化して描画
	struct InstanceGroup {
		ID3D11ShaderResourceView* pSRV;
		std::vector<XMFLOAT4X4> matrices;
	};
	std::map<ID3D11ShaderResourceView*, InstanceGroup> groups;

	for (const auto& mapData : g_MapList)
	{
		if (mapData.isHidden) continue;

		float dx = mapData.pos.x - cameraPos.x;
		float dy = mapData.pos.y - cameraPos.y;
		float dz = mapData.pos.z - cameraPos.z;
		
		float distSq = dx * dx + dy * dy + dz * dz;
		if (distSq > 1600.0f) continue; // 40mまで

		float dot = dx * look.x + dy * look.y + dz * look.z;
		if (dot < -1.0f) continue;

		int id = mapData.blockID;
		ID3D11ShaderResourceView* pTexture = nullptr;

		if (mapData.no == FIELD_STAIRS_UP || mapData.no == FIELD_STAIRS_DOWN) {
			pTexture = g_TextureStairs;
		} else {
			if (id <= 0 || id >= MAX_BLOCK_TYPES || g_BlockTextures[id] == nullptr) {
				pTexture = g_BlockTextures[0];
			} else {
				pTexture = g_BlockTextures[id];
			}
		}

		auto& group = groups[pTexture];
		if (group.matrices.empty()) {
			group.pSRV = pTexture;
		}

		XMMATRIX world = XMMatrixTranslation(mapData.pos.x, mapData.pos.y, mapData.pos.z);
		if (hasBaseRot || mapData.rotY != 0.0f) {
			XMMATRIX rotation = mapData.rotY != 0.0f ? 
				(baseRotMtx * XMMatrixRotationY(XMConvertToRadians(mapData.rotY))) : baseRotMtx;
			world = rotation * world;
		}

		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, XMMatrixTranspose(world));
		group.matrices.push_back(m);
	}

	UINT strides[2] = { sizeof(Vertex3D), sizeof(XMFLOAT4X4) };
	UINT offsets[2] = { 0, 0 };

	for (auto& pair : groups) {
		auto& group = pair.second;
		if (group.matrices.empty()) continue;

		size_t count = group.matrices.size();
		for (size_t i = 0; i < count; i += MAX_INSTANCES) {
			size_t batchSize = (count - i) > MAX_INSTANCES ? MAX_INSTANCES : (count - i);

			D3D11_MAPPED_SUBRESOURCE msr;
			if (SUCCEEDED(g_pContext->Map(g_InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) {
				memcpy(msr.pData, &group.matrices[i], sizeof(XMFLOAT4X4) * batchSize);
				g_pContext->Unmap(g_InstanceBuffer, 0);
			}

			ID3D11Buffer* vbs[2] = { g_VertexBuffer, g_InstanceBuffer };
			g_pContext->IASetVertexBuffers(0, 2, vbs, strides, offsets);
			g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			g_pContext->PSSetShaderResources(0, 1, &group.pSRV);

			g_pContext->DrawIndexedInstanced(36, (UINT)batchSize, 0, 0, 0);
		}
	}
}

void Field_Finalize(void)
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_IndexBuffer);

	SAFE_RELEASE(g_InstanceBuffer);

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
	// static を使用して再確保によるコストを削減
	static std::vector<std::vector<bool>> closedList;
	static std::vector<std::vector<Node>> nodes;
	
	if (closedList.size() != (size_t)MAP_H) closedList.assign(MAP_H, std::vector<bool>(MAP_W, false));
	if (nodes.size() != (size_t)MAP_H) nodes.resize(MAP_H, std::vector<Node>(MAP_W));

	for (int i = 0; i < MAP_H; ++i) {
		std::fill(closedList[i].begin(), closedList[i].end(), false);
	}

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

