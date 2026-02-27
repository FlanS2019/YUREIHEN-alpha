#include "define.h"
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
#include <algorithm>
#include "keyboard.h"
#include "Floor1.h"
#include "Floor2.h"
#include "Floor3.h"
using namespace DirectX;

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11Buffer* g_IndexBuffer = NULL;

// 壁の表示フラグ（true=表示, false=非表示）
static bool g_DebugShowWalls = true;

// インスタンス描画用バッファ
#define MAX_INSTANCES (5000)
static ID3D11Buffer* g_InstanceBuffer = NULL;

#define MAX_BLOCK_TYPES 100 
static ID3D11ShaderResourceView* g_BlockTextures[MAX_BLOCK_TYPES];
static ID3D11ShaderResourceView* g_TextureStairs;

XMFLOAT3 rotateBox = XMFLOAT3(0, 0, 0);
static std::vector<MAPDATA> g_MapList;

// 2階描画時にZ=0～17の真下にFloor1を重ねて表示するためのリスト（当たり判定なし）
static std::vector<MAPDATA> g_SubFloorMapList;

static int g_CurrentFloor = START_FLOOR - 1;

// 壁判定の有効/無効フラグ（デバッグシーン等で無効化する）
static bool g_WallCheckEnabled = true;

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
	case 2:  // ダークオークの原木（抜けられない壁）
	case 4:  // シラカバの板材
	case 14: // ダークオークフェンス
	case 15: // オークフェンス
	case 16: // ダイアモンド
	case 17: // カーペット
	case 18: // 窓ガラス
	case 39: // コピー用ブロック
		return FIELD_BOX;

	case 3:  // ダークオークの板材（ゴーストのみ通過可能な壁）
		return FIELD_WALL_PASS;

		// --- 階段として扱うもの ---
	case 5: case 6: case 7: case 8: // 下付き階段
		return FIELD_STAIRS_UP;

	case 9: case 10: case 11: case 12: // 上付き階段
		return FIELD_STAIRS_DOWN;

	case 98: case 13: // 方向指示ブロック・ドア
		return FIELD_NONE;

	default:
		// 家具ブロック（JSONで model が定義されているもの）はFIELD_NONEとして扱う
		if (IsFurnitureBlock(minecraftID)) return FIELD_NONE;
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
	g_SubFloorMapList.clear();

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

				auto isFaceVisible = [&](int myID, int targetY, int targetZ, int targetX) -> bool {
					int neighborID = GetMapBlockID(floor, targetY, targetZ, targetX);

					if (neighborID == 0) return true; // 隣が空気なら絶対に面を描画する

					if (ConvertMapID(neighborID) == FIELD_NONE) return true;

					// 透過ブロック(ガラス等)を並べた時、隣が同じブロックなら接合面を消す
					if (myID == neighborID) return false;

					// ID:2とID:3は同じ壁テクスチャなので、隣接時は接合面を消す
					if ((myID == 2 || myID == 3) && (neighborID == 2 || neighborID == 3)) return false;

					// 自分が不透明ブロックで、隣が透過ブロックの場合は面を描画する
					// 他に透過ブロックIDがあれば || neighborID == XX を追加
					if (myID != 18 && neighborID == 18) return true;

					// それ以外（隣が別の不透明ブロックなど）は完全に埋もれるので面を消す
					return false;
					};

				MAPDATA data;

				// 各方向の面が見えるかチェック

				data.drawFace[0] = isFaceVisible(mcID, y, z + 1, x); // 0: 手前 (Z座標反転のため z+1)
				data.drawFace[1] = isFaceVisible(mcID, y, z, x + 1); // 1: 右   (x+1)
				data.drawFace[2] = isFaceVisible(mcID, y - 1, z, x); // 2: 下   (y-1)
				data.drawFace[3] = isFaceVisible(mcID, y, z - 1, x); // 3: 奥   (Z座標反転のため z-1)
				data.drawFace[4] = isFaceVisible(mcID, y, z, x - 1); // 4: 左   (x-1)
				data.drawFace[5] = isFaceVisible(mcID, y + 1, z, x); // 5: 上   (y+1)

				// 6面すべて見えないなら、ブロック自体を描画リストから除外
				data.isHidden = !(data.drawFace[0] || data.drawFace[1] || data.drawFace[2] ||
					data.drawFace[3] || data.drawFace[4] || data.drawFace[5]);

				if (data.isHidden) continue;

				data.pos = XMFLOAT3(
					(x - offsetX),
					(float)y - 1.0f,
					(offsetZ - z)
				);

				data.no = type;
				data.isHidden = false;
				data.rotY = 0.0f;
				data.blockID = mcID;
				data.mapY = y;

				data.currentScale = 1.0f;

				g_MapList.push_back(data);
			}
		}
	}

	// テクスチャバッチング用にソート（blockIDベース）
	std::sort(g_MapList.begin(), g_MapList.end(), [](const MAPDATA& a, const MAPDATA& b) {
		// 壁ブロック(ID:2,3)はy軸でテクスチャが決まるのでmapYでソート
		bool aIsWall = (a.blockID == 2 || a.blockID == 3);
		bool bIsWall = (b.blockID == 2 || b.blockID == 3);
		if (aIsWall && bIsWall) return a.mapY < b.mapY;
		if (aIsWall != bIsWall) return aIsWall < bIsWall;
		return a.blockID < b.blockID;
		});

	// 2階の場合: マップチップZ=0～17の真下にFloor1を描画用リストとして構築
	// 配列インデックス: idx = (MAP_LENGTH-1) - Z なので Z=0→idx=40, Z=17→idx=23
	// Floor2の底面はpos.y=-1 (Y=0)。Floor1を8段下（1フロア分）に配置する
	if (floor == 1)
	{
		const float SUB_Y_OFFSET = -8.0f; // 1フロア分 (MAP_HEIGHT=8) 下にずらす
		const int SUB_Z_IDX_MIN = 20;     // マップチップZ=17 → 配列インデックス 40-17=23
		const int SUB_Z_IDX_MAX = 40;     // マップチップZ=0  → 配列インデックス 40-0=40
		const int SUB_X_MIN = 0;          // X=0
		const int SUB_X_MAX = MAP_W - 1;  // X=52

		for (int y = 0; y < MAP_HEIGHT; y++)
		{
			for (int z = SUB_Z_IDX_MIN; z <= SUB_Z_IDX_MAX; z++)
			{
				for (int x = SUB_X_MIN; x <= SUB_X_MAX; x++)
				{
					int mcID = Floor1[y][z][x];
					if (mcID == 0) continue;

					FIELD_TYPE type = ConvertMapID(mcID);
					if (type == FIELD_NONE) continue;

					// 隣接面の可視チェック（Floor1配列で完結）
					auto isSubFaceVisible = [&](int ny, int nz, int nx) -> bool {
						if (ny < 0 || ny >= MAP_HEIGHT) return true;
						if (nz < 0 || nz >= MAP_H || nx < 0 || nx >= MAP_W) return true;
						int nID = Floor1[ny][nz][nx];
						if (nID == 0) return true;
						if (ConvertMapID(nID) == FIELD_NONE) return true;
						if (mcID == nID) return false;
						if ((mcID == 2 || mcID == 3) && (nID == 2 || nID == 3)) return false;
						if (mcID != 18 && nID == 18) return true;
						return false;
					};

					MAPDATA data;
					data.drawFace[0] = isSubFaceVisible(y, z + 1, x);
					data.drawFace[1] = isSubFaceVisible(y, z, x + 1);
					data.drawFace[2] = isSubFaceVisible(y - 1, z, x);
					data.drawFace[3] = isSubFaceVisible(y, z - 1, x);
					data.drawFace[4] = isSubFaceVisible(y, z, x - 1);
					data.drawFace[5] = isSubFaceVisible(y + 1, z, x);

					bool anyFace = false;
					for (int i = 0; i < 6; i++) anyFace |= data.drawFace[i];
					if (!anyFace) continue;

					data.pos = XMFLOAT3(
						(x - offsetX),
						(float)y - 1.0f + SUB_Y_OFFSET,
						(offsetZ - z)
					);
					data.no = type;
					data.isHidden = false;
					data.rotY = 0.0f;
					data.blockID = mcID;
					data.mapY = y;
					data.currentScale = 1.0f;

					g_SubFloorMapList.push_back(data);
				}
			}
		}

		std::sort(g_SubFloorMapList.begin(), g_SubFloorMapList.end(), [](const MAPDATA& a, const MAPDATA& b) {
			bool aIsWall = (a.blockID == 2 || a.blockID == 3);
			bool bIsWall = (b.blockID == 2 || b.blockID == 3);
			if (aIsWall && bIsWall) return a.mapY < b.mapY;
			if (aIsWall != bIsWall) return aIsWall < bIsWall;
			return a.blockID < b.blockID;
			});
	}
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

	g_CurrentFloor = START_FLOOR - 1; // 3階スタート
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
	HRESULT hr = pDevice->CreateBuffer(&idesc, NULL, &g_InstanceBuffer);
	if (FAILED(hr) || g_InstanceBuffer == NULL)
	{
		MessageBox(NULL, L"インスタンスバッファの作成に失敗しました", L"エラー", MB_OK);
		return;
	}
}

void Field_Update(void)
{
	if (Keyboard_IsKeyDownTrigger(KK_M))
	{
		g_DebugShowWalls = !g_DebugShowWalls;
	}

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
			FIELD_TYPE mcType = ConvertMapID(mcID);

			if (mcType == FIELD_BOX || mcType == FIELD_WALL_PASS)
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
	XMFLOAT3 cameraAt = pCamera->GetAtPos();
	XMVECTOR vCamPos = XMLoadFloat3(&cameraPos);
	XMVECTOR vCamAt = XMLoadFloat3(&cameraAt);
	XMVECTOR vForward = XMVector3Normalize(XMVectorSubtract(vCamAt, vCamPos));

	D3D11_VIEWPORT vp;
	UINT numVP = 1;
	g_pContext->RSGetViewports(&numVP, &vp);

	float radX = XMConvertToRadians(rotateBox.x);
	float radY = XMConvertToRadians(rotateBox.y);
	float radZ = XMConvertToRadians(rotateBox.z);
	bool hasBaseRot = (rotateBox.x != 0.0f || rotateBox.y != 0.0f || rotateBox.z != 0.0f);
	XMMATRIX baseRotMtx = hasBaseRot ? XMMatrixRotationRollPitchYaw(radX, radY, radZ) : XMMatrixIdentity();

	// 面ごとのバッチリスト (6面分)
	static std::vector<XMFLOAT4X4> batchListFace[6];
	for (int i = 0; i < 6; i++) {
		batchListFace[i].clear();
		if (batchListFace[i].capacity() < MAX_INSTANCES) batchListFace[i].reserve(MAX_INSTANCES);
	}

	ID3D11ShaderResourceView* currentSRV = nullptr;

	auto FlushBatch = [&](void) {
		if (currentSRV == nullptr) return;

		for (int face = 0; face < 6; face++) {
			if (batchListFace[face].empty()) continue;

			D3D11_MAPPED_SUBRESOURCE msr;
			if (SUCCEEDED(g_pContext->Map(g_InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) {
				memcpy(msr.pData, batchListFace[face].data(), sizeof(XMFLOAT4X4) * batchListFace[face].size());
				g_pContext->Unmap(g_InstanceBuffer, 0);
			}

			UINT strides[2] = { sizeof(Vertex3D), sizeof(XMFLOAT4X4) };
			UINT offsets[2] = { 0, 0 };
			ID3D11Buffer* vbs[2] = { g_VertexBuffer, g_InstanceBuffer };

			g_pContext->IASetVertexBuffers(0, 2, vbs, strides, offsets);
			g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			g_pContext->PSSetShaderResources(0, 1, &currentSRV);

			UINT startIndex = face * 6;
			g_pContext->DrawIndexedInstanced(6, (UINT)batchListFace[face].size(), startIndex, 0, 0);

			batchListFace[face].clear();
		}
		};



	for (const auto& mapData : g_MapList)
	{
		if (mapData.isHidden) continue;

		// 壁（Y座標が0以上＝地面より上にあるブロック）の場合、フラグがfalseなら描画をスキップする
		// 地面（pos.y = -1.0f）は常に表示する
		if (!g_DebugShowWalls && mapData.pos.y >= 0.0f)
		{
			continue;
		}

		XMVECTOR vPos = XMLoadFloat3(&mapData.pos);
		XMVECTOR vToPos = XMVectorSubtract(vPos, vCamPos);

		// 距離によるカリング
		float distSq;
		XMStoreFloat(&distSq, XMVector3LengthSq(vToPos));
		if (distSq > 2500.0f) continue;

		// 背面カリング (カメラの後ろにあるものは描画しない)
		float dot;
		XMStoreFloat(&dot, XMVector3Dot(vToPos, vForward));
		if (dot < -2.0f) continue; // 余裕を持って-2.0f

		// 錐体カリング (Frustum Culling) もどき
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
		else if (mapData.blockID == 2 || mapData.blockID == 3) {
			// 壁ブロックはy軸でテクスチャを選択
			if (mapData.mapY == 1) nextSRV = g_BlockTextures[2];       // kabesita.png
			else if (mapData.mapY == 2) nextSRV = g_BlockTextures[3];  // tunagime.png
			else nextSRV = g_BlockTextures[4];                         // kabeue.png
		}
		else {
			int id = mapData.blockID;
			if (id <= 0 || id >= MAX_BLOCK_TYPES || g_BlockTextures[id] == nullptr) nextSRV = g_BlockTextures[0];
			else nextSRV = g_BlockTextures[id];
		}

		// MAX_INSTANCES を超えそうならフラッシュ
		bool overLimit = false;
		for (int i = 0; i < 6; i++) {
			if (batchListFace[i].size() >= MAX_INSTANCES - 1) overLimit = true;
		}

		if (nextSRV != currentSRV || overLimit)
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

		// 見えている面だけをそれぞれのバッチに登録
		for (int i = 0; i < 6; i++) {
			if (mapData.drawFace[i]) {
				batchListFace[i].push_back(m);
			}
		}
	}
	FlushBatch();

	// 2階の場合: Z=0～17真下のFloor1を描画（当たり判定なし）
	if (!g_SubFloorMapList.empty())
	{
		currentSRV = nullptr;
		for (int i = 0; i < 6; i++) batchListFace[i].clear();

		for (const auto& mapData : g_SubFloorMapList)
		{
			XMVECTOR vPos = XMLoadFloat3(&mapData.pos);
			XMVECTOR vToPos = XMVectorSubtract(vPos, vCamPos);

			float distSq;
			XMStoreFloat(&distSq, XMVector3LengthSq(vToPos));
			if (distSq > 2500.0f) continue;

			float dot;
			XMStoreFloat(&dot, XMVector3Dot(vToPos, vForward));
			if (dot < -2.0f) continue;

			XMVECTOR vClipPos = XMVector3TransformCoord(vPos, VP);
			XMFLOAT3 clipPos;
			XMStoreFloat3(&clipPos, vClipPos);

			float marginX = 0.2f;
			float marginY = 0.8f;
			if (clipPos.x < -1.0f - marginX || clipPos.x > 1.0f + marginX ||
				clipPos.y < -1.0f - marginY || clipPos.y > 1.0f + marginY) continue;
			if (clipPos.z < 0.0f || clipPos.z > 1.0f) continue;

			ID3D11ShaderResourceView* nextSRV = nullptr;
			if (mapData.no == FIELD_STAIRS_UP || mapData.no == FIELD_STAIRS_DOWN) {
				nextSRV = g_TextureStairs;
			}
			else if (mapData.blockID == 2 || mapData.blockID == 3) {
				if (mapData.mapY == 1) nextSRV = g_BlockTextures[2];
				else if (mapData.mapY == 2) nextSRV = g_BlockTextures[3];
				else nextSRV = g_BlockTextures[4];
			}
			else {
				int id = mapData.blockID;
				if (id <= 0 || id >= MAX_BLOCK_TYPES || g_BlockTextures[id] == nullptr) nextSRV = g_BlockTextures[0];
				else nextSRV = g_BlockTextures[id];
			}

			bool overLimit = false;
			for (int i = 0; i < 6; i++) {
				if (batchListFace[i].size() >= MAX_INSTANCES - 1) overLimit = true;
			}

			if (nextSRV != currentSRV || overLimit)
			{
				FlushBatch();
				currentSRV = nextSRV;
			}

			XMMATRIX world = XMMatrixTranslation(mapData.pos.x, mapData.pos.y, mapData.pos.z);
			XMFLOAT4X4 m;
			XMStoreFloat4x4(&m, XMMatrixTranspose(world));

			for (int i = 0; i < 6; i++) {
				if (mapData.drawFace[i]) {
					batchListFace[i].push_back(m);
				}
			}
		}
		FlushBatch();
	}
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
	if (!g_WallCheckEnabled) return false;

	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);

	if (gx >= 0 && gx < MAP_W && gz >= 0 && gz < MAP_H)
	{
		int mcID = GetMapBlockID(g_CurrentFloor, 1, gz, gx);
		FIELD_TYPE type = ConvertMapID(mcID);
		if (type == FIELD_BOX || type == FIELD_WALL_PASS) return true;

		int mcID2 = GetMapBlockID(g_CurrentFloor, 2, gz, gx);
		FIELD_TYPE type2 = ConvertMapID(mcID2);
		if (type2 == FIELD_BOX || type2 == FIELD_WALL_PASS) return true;
	}
	return false;
}

bool Field_IsOuterWall(float x, float z)
{
	if (!g_WallCheckEnabled) return false;

	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);
	if (gx < 0 || gx >= MAP_W || gz < 0 || gz >= MAP_H) return true;
	if (gx == 0 || gx == MAP_W - 1 || gz == 0 || gz == MAP_H - 1) return true;
	return false;
}

bool Field_IsWallForGhost(float x, float z)
{
	if (!g_WallCheckEnabled) return false;

	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);

	if (gx < 0 || gx >= MAP_W || gz < 0 || gz >= MAP_H) return true;

	// FIELD_BOXのみ壁として扱う（FIELD_WALL_PASSはゴーストが通過可能）
	int mcID = GetMapBlockID(g_CurrentFloor, 1, gz, gx);
	if (ConvertMapID(mcID) == FIELD_BOX) return true;

	int mcID2 = GetMapBlockID(g_CurrentFloor, 2, gz, gx);
	if (ConvertMapID(mcID2) == FIELD_BOX) return true;

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
		FIELD_TYPE type = ConvertMapID(mcID);
		if (type != FIELD_NONE) return true;
	}
	return false;
}

bool Field_CheckWallBetween(XMFLOAT3 start, XMFLOAT3 end)
{
	if (!g_WallCheckEnabled) return false;

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


	if (ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, endZ, endX)) == FIELD_BOX ||
		ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, endZ, endX)) == FIELD_WALL_PASS)
	{
		int dx[] = { 0, 0, 1, -1 };
		int dz[] = { 1, -1, 0, 0 };
		for (int i = 0; i < 4; i++) {
			int nx = endX + dx[i];
			int nz = endZ + dz[i];
			if (nx >= 0 && nx < MAP_W && nz >= 0 && nz < MAP_H) {
				FIELD_TYPE t = ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, nz, nx));
				if (t != FIELD_BOX && t != FIELD_WALL_PASS) {
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

	int dirX[] = { 0, 0, -1, 1, -1, 1, -1, 1 };  // 4方向 + 斜め4方向
	int dirZ[] = { -1, 1, 0, 0, -1, -1, 1, 1 };
	float dirCost[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.41f, 1.41f, 1.41f, 1.41f };
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

		for (int i = 0; i < 8; i++)  // 8方向をチェック
		{
			int nextX = current.x + dirX[i];
			int nextZ = current.z + dirZ[i];

			if (nextX < 0 || nextX >= MAP_W || nextZ < 0 || nextZ >= MAP_H) continue;

			// 壁判定（FIELD_BOXとFIELD_WALL_PASSの両方を壁として扱う）
			FIELD_TYPE nextType = ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, nextZ, nextX));
			if (nextType == FIELD_BOX || nextType == FIELD_WALL_PASS) continue;

			// Y=0が空気（床なし）のマスは通行不可
			if (GetMapBlockID(g_CurrentFloor, 0, nextZ, nextX) == 0) continue;

			// 斜め移動の場合、両隣が通路であるか確認（コーナーにめり込まない）
			if (i >= 4)
			{
				int adjX = current.x + dirX[i];
				int adjZ = current.z + dirZ[i];
				int diagX = current.x + dirX[i];
				int diagZ = current.z + dirZ[i];

				FIELD_TYPE diagTypeX = ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, current.z, diagX));
				if (diagTypeX == FIELD_BOX || diagTypeX == FIELD_WALL_PASS) continue;
				FIELD_TYPE diagTypeZ = ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, diagZ, current.x));
				if (diagTypeZ == FIELD_BOX || diagTypeZ == FIELD_WALL_PASS) continue;
			}

			if (closedList[nextZ][nextX]) continue;

			float newCost = current.cost + dirCost[i];
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

// 指定フロアのSTAIRS_UPブロックのワールド座標を最初に見つけた1つ返す
XMFLOAT3 Field_GetStairsUpWorldPos(int floor)
{
	for (int z = 0; z < MAP_H; z++)
	{
		for (int x = 0; x < MAP_W; x++)
		{
			int mcID = GetMapBlockID(floor, 1, z, x);
			if (ConvertMapID(mcID) == FIELD_STAIRS_UP)
			{
				float wx = GridToWorldX(x);
				float wz = GridToWorldZ(z);
				return { wx, PATROL_HEIGHT, wz };
			}
		}
	}
	return { 0.0f, PATROL_HEIGHT, 0.0f };
}

// マップID97（バスターズ誘導マーカー）のワールド座標を返す
XMFLOAT3 Field_GetMarker97WorldPos(int floor)
{
	for (int z = 0; z < MAP_H; z++)
	{
		for (int x = 0; x < MAP_W; x++)
		{
			int mcID = GetMapBlockID(floor, 1, z, x);
			if (mcID == 97)
			{
				float wx = GridToWorldX(x);
				float wz = GridToWorldZ(z);
				return { wx, PATROL_HEIGHT, wz };
			}
		}
	}
	// 97が見つからない場合はマップ中央にフォールバック（階段5・6には誘導しない）
	return { 0.0f, PATROL_HEIGHT, 0.0f };
}

// 指定フロアのID5またはID6（階段ブロック）のワールド座標を全て取得する
std::vector<XMFLOAT3> Field_GetStairsExitPositions(int floor)
{
	std::vector<XMFLOAT3> positions;
	for (int z = 0; z < MAP_H; z++)
	{
		for (int x = 0; x < MAP_W; x++)
		{
			int mcID = GetMapBlockID(floor, 0, z, x);
			if (mcID == 5 || mcID == 6)
			{
				float wx = GridToWorldX(x);
				float wz = GridToWorldZ(z);
				positions.push_back({ wx, PATROL_HEIGHT, wz });
			}
		}
	}
	return positions;
}

// 現在フロアの指定ワールド座標のY=0またはY=1レイヤーの生マップIDを返す
int Field_GetRawBlockID(float x, float z)
{
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);
	if (gx < 0 || gx >= MAP_W || gz < 0 || gz >= MAP_H) return 0;
	int id = GetMapBlockID(g_CurrentFloor, 1, gz, gx);
	if (id == 0) id = GetMapBlockID(g_CurrentFloor, 0, gz, gx);
	return id;
}

bool Field_IsNoFloor(float x, float z)
{
	int gx = WorldToGridX(x);
	int gz = WorldToGridZ(z);
	if (gx < 0 || gx >= MAP_W || gz < 0 || gz >= MAP_H) return true;
	return (GetMapBlockID(g_CurrentFloor, 0, gz, gx) == 0);
}

void Field_SetWallCheckEnabled(bool enabled)
{
	g_WallCheckEnabled = enabled;
}