#include "field.h"
#include "texture.h"
#include "Camera.h"
#include "sprite.h"
#include "box.h"
#include "define.h"
#include "ghost.h"
#include "furniture.h"
#include <DirectXMath.h>
#include <vector>
#include <queue>
#include <map>
#include <cmath> 
#include <algorithm> // sort用に必要

#include "Floor1.h"
#include "Floor2.h"
#include "Floor3.h"

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

// 階層と座標を指定してブロックIDを取得するヘルパー関数
static int GetMapBlockID(int floor, int y, int z, int x)
{
	// 配列外参照チェック
	if (y < 0 || y >= MAP_HEIGHT || z < 0 || z >= MAP_H || x < 0 || x >= MAP_W) return 0;

	switch (floor)
	{
	case 0: return Floor1[y][z][x]; // 1階
	case 1: return Floor2[y][z][x]; // 2階
	case 2: return Floor3[y][z][x]; // 3階
	default: return 0;
	}
}

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
	case 98:
	case 50: case 51: case 52:case 53:case 54:case 55:case 56:case 57:case 58:case 59: //家具
	case 60: case 61: case 62:case 63:case 64:case 65:case 66:case 67:case 68:case 69: //家具
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
				int mcID = GetMapBlockID(floor, y, z, x);
				if (mcID == 0) continue;

				FIELD_TYPE type = ConvertMapID(mcID);
				if (type == FIELD_NONE) continue;

				bool isVisible = false;

				if (GetMapBlockID(floor, y + 1, z, x) == 0) isVisible = true;
				else if (GetMapBlockID(floor, y - 1, z, x) == 0) isVisible = true;
				else if (GetMapBlockID(floor, y, z + 1, x) == 0) isVisible = true;
				else if (GetMapBlockID(floor, y, z - 1, x) == 0) isVisible = true;
				else if (GetMapBlockID(floor, y, z, x + 1) == 0) isVisible = true;
				else if (GetMapBlockID(floor, y, z, x - 1) == 0) isVisible = true;

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

				data.currentScale = 1.0f;

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

	g_CurrentFloor = 2; // 3階スタート
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
	// まず「今、隠すべき場所」を計算
	static uint8_t shouldHide[MAP_LENGTH][MAP_WIDTH];
	memset(shouldHide, 0, sizeof(shouldHide));

	Ghost* ghost = GetGhost();
	if (!ghost) return;

	XMFLOAT3 cameraPos = GetCamera()->GetPos();
	XMFLOAT3 playerPos = ghost->GetPos();

	// プレイヤーの頭の高さ(Y+1.0f)を目標にする
	playerPos.y += 1.0f;

	float dx = playerPos.x - cameraPos.x;
	float dz = playerPos.z - cameraPos.z;
	float distSq = dx * dx + dz * dz;

	// 距離が近すぎる場合はスキップ
	if (distSq < 0.25f) return;

	// 極端に遠い場合は処理を制限
	const float MAX_DIST = 30.0f;
	float dist = sqrtf(distSq);
	if (dist > MAX_DIST) dist = MAX_DIST;

	float invDist = 1.0f / dist;
	float stepX = dx * invDist;
	float stepZ = dz * invDist;

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
			int mcID = GetMapBlockID(g_CurrentFloor, 1, gridZ, gridX);

			if (ConvertMapID(mcID) == FIELD_BOX)
			{
				// 遮蔽物を見つけたら、その周囲も含めて「隠すフラグ」を立てる
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
	}

	float animSpeed = 0.2f; // アニメーション速度

	for (auto& mapData : g_MapList)
	{
		int mapGridX = WorldToGridX(mapData.pos.x);
		int mapGridZ = WorldToGridZ(mapData.pos.z);

		bool targetHide = false;

		// 範囲内かつ、壁(Y>=0)の場合のみ判定
		if (mapGridX >= 0 && mapGridX < MAP_W && mapGridZ >= 0 && mapGridZ < MAP_H)
		{
			if (mapData.pos.y >= 0.0f && shouldHide[mapGridZ][mapGridX])
			{
				targetHide = true;
			}
		}

		mapData.isHidden = targetHide;
		mapData.currentScale = 1.0f; // 念のためサイズは戻しておく
	}
}

void Field_Draw(void)
{
	Shader_BeginInstance();
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Camera* pCamera = GetCamera();
	XMMATRIX View = pCamera->GetView();
	XMMATRIX Projection = pCamera->GetProjection();
	XMMATRIX VP = View * Projection;

	Shader_SetMatrix(VP);

	XMFLOAT3 cameraPos = pCamera->GetPos();

	D3D11_VIEWPORT vp;
	UINT numVP = 1;
	g_pContext->RSGetViewports(&numVP, &vp);

	float radX = XMConvertToRadians(rotateBox.x);
	float radY = XMConvertToRadians(rotateBox.y);
	float radZ = XMConvertToRadians(rotateBox.z);
	bool hasBaseRot = (rotateBox.x != 0.0f || rotateBox.y != 0.0f || rotateBox.z != 0.0f);
	XMMATRIX baseRotMtx = hasBaseRot ? XMMatrixRotationRollPitchYaw(radX, radY, radZ) : XMMatrixIdentity();

	static std::vector<XMFLOAT4X4> batchList;
	batchList.clear();
	if (batchList.capacity() < MAX_INSTANCES) batchList.reserve(MAX_INSTANCES);

	ID3D11ShaderResourceView* currentSRV = nullptr;

	auto FlushBatch = [&](void) {
		if (batchList.empty() || currentSRV == nullptr) return;

		D3D11_MAPPED_SUBRESOURCE msr;
		if (SUCCEEDED(g_pContext->Map(g_InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) {
			memcpy(msr.pData, batchList.data(), sizeof(XMFLOAT4X4) * batchList.size());
			g_pContext->Unmap(g_InstanceBuffer, 0);
		}

		UINT strides[2] = { sizeof(Vertex3D), sizeof(XMFLOAT4X4) };
		UINT offsets[2] = { 0, 0 };
		ID3D11Buffer* vbs[2] = { g_VertexBuffer, g_InstanceBuffer };

		g_pContext->IASetVertexBuffers(0, 2, vbs, strides, offsets);
		g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		g_pContext->PSSetShaderResources(0, 1, &currentSRV);

		g_pContext->DrawIndexedInstanced(36, (UINT)batchList.size(), 0, 0, 0);

		batchList.clear();
		};

	for (const auto& mapData : g_MapList)
	{
		if (mapData.isHidden) continue;

		float dx = mapData.pos.x - cameraPos.x;
		float dy = mapData.pos.y - cameraPos.y;
		float dz = mapData.pos.z - cameraPos.z;
		if (dx * dx + dy * dy + dz * dz > 2500.0f) continue;

		XMVECTOR vPos = XMLoadFloat3(&mapData.pos);
		XMVECTOR vClipPos = XMVector3TransformCoord(vPos, VP);

		XMFLOAT3 clipPos;
		XMStoreFloat3(&clipPos, vClipPos);

		float marginX = 0.2f;
		float marginY = 0.8f;

		if (clipPos.x < -1.0f - marginX || clipPos.x > 1.0f + marginX ||
			clipPos.y < -1.0f - marginY || clipPos.y > 1.0f + marginY)
		{
			continue;
		}

		if (clipPos.z < 0.0f || clipPos.z > 1.0f) continue;

		ID3D11ShaderResourceView* nextSRV = nullptr;
		if (mapData.no == FIELD_STAIRS_UP || mapData.no == FIELD_STAIRS_DOWN) {
			nextSRV = g_TextureStairs;
		}
		else {
			int id = mapData.blockID;
			if (id <= 0 || id >= MAX_BLOCK_TYPES || g_BlockTextures[id] == nullptr) nextSRV = g_BlockTextures[0];
			else nextSRV = g_BlockTextures[id];
		}

		if (nextSRV != currentSRV || batchList.size() >= MAX_INSTANCES)
		{
			FlushBatch();
			currentSRV = nextSRV;
		}

		XMMATRIX world = XMMatrixTranslation(mapData.pos.x, mapData.pos.y, mapData.pos.z);
		if (hasBaseRot || mapData.rotY != 0.0f) {
			XMMATRIX rotation = mapData.rotY != 0.0f ? (baseRotMtx * XMMatrixRotationY(XMConvertToRadians(mapData.rotY))) : baseRotMtx;
			world = rotation * world;
		}

		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, XMMatrixTranspose(world));
		batchList.push_back(m);
	}
	FlushBatch();
}

void Field_Finalize(void)
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_IndexBuffer);
	SAFE_RELEASE(g_InstanceBuffer);
	for (int i = 0; i < MAX_BLOCK_TYPES; i++) SAFE_RELEASE(g_BlockTextures[i]);
	SAFE_RELEASE(g_TextureStairs);
	g_MapList.clear();
}

void Field_ChangeFloor(int floorIndex)
{
	g_CurrentFloor = floorIndex;
	LoadMapData(g_CurrentFloor);

	Furniture_Initialize();
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
		int mcID = GetMapBlockID(g_CurrentFloor, 1, gz, gx);
		FIELD_TYPE type = ConvertMapID(mcID);

		if (type == FIELD_NONE)
		{
			mcID = GetMapBlockID(g_CurrentFloor, 2, gz, gx);
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
		int mcID = GetMapBlockID(g_CurrentFloor, 1, gz, gx);
		if (ConvertMapID(mcID) == FIELD_BOX) return true;

		int mcID2 = GetMapBlockID(g_CurrentFloor, 2, gz, gx);
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
		int mcID = GetMapBlockID(g_CurrentFloor, gy, gz, gx);
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

		if (GetMapBlockID(g_CurrentFloor, 0, gz, gx) != 0) return 0.0f;
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


	if (ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, endZ, endX)) == FIELD_BOX)
	{
		int dx[] = { 0, 0, 1, -1 };
		int dz[] = { 1, -1, 0, 0 };
		for (int i = 0; i < 4; i++) {
			int nx = endX + dx[i];
			int nz = endZ + dz[i];
			if (nx >= 0 && nx < MAP_W && nz >= 0 && nz < MAP_H) {
				if (ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, nz, nx)) != FIELD_BOX) {
					endX = nx; endZ = nz; break;
				}
			}
		}
	}

	std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;
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
	int maxCalculationSteps = 1000;

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

			// 壁判定
			if (ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, nextZ, nextX)) == FIELD_BOX) continue;

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

			if (steps > maxCalculationSteps) break;
		}
	}

	return path;
}

float Field_CalculateRotationFromMarker(float x, float y, float z)
{
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);
	int gy = (int)round(y + 1.0f);
	int markerID = 98; // 目印のID

	// 配列外参照防止
	if (gx < 1 || gx >= MAP_W - 1 || gz < 1 || gz >= MAP_H - 1 || gy < 0 || gy >= MAP_HEIGHT) return 0.0f;

	// GetMapBlockID を使用して現在の階層のデータを参照する
	if (GetMapBlockID(g_CurrentFloor, gy, gz - 1, gx) == markerID) return 180.0f; // 奥

	if (GetMapBlockID(g_CurrentFloor, gy, gz + 1, gx) == markerID) return 0.0f;   // 手前

	if (GetMapBlockID(g_CurrentFloor, gy, gz, gx + 1) == markerID) return 270.0f; // 右

	if (GetMapBlockID(g_CurrentFloor, gy, gz, gx - 1) == markerID) return 90.0f;  // 左

	return 0.0f;
}