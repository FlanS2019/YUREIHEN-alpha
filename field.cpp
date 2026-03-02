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

// 天井描画用リスト（1階・2階・3階、描画専用・当たり判定なし）
static std::vector<MAPDATA> g_CeilingList;

// 天井テクスチャ（3種類）
// [0]: kabeue.png（壁上部）[1]: tenjou_kado.png（角）[2]: tenjou_hen.png（辺）
#define CEILING_TEXTURE_NUM (3)
static ID3D11ShaderResourceView* g_CeilingTextures[CEILING_TEXTURE_NUM];

static int g_CurrentFloor = START_FLOOR - 1;

// 壁判定の有効/無効フラグ（デバッグシーン等で無効化する）
static bool g_WallCheckEnabled = true;

// チュートリアル壁ごとの当たり判定有効フラグ（インデックス0=壁1/ID13, 1=壁2/ID14, 2=壁3/ID15）
static bool g_TutorialWallEnabled[3] = { true, true, true };

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

	int id;
	switch (floor)
	{
	case 0: id = Floor1[y][z][x]; break;
	case 1: id = Floor2[y][z][x]; break;
	case 2: id = Floor3[y][z][x]; break;
	default: return 0;
	}

	// 負の値は無効（旧バックアップ方式の名残）
	if (id < 0) return 0;

	// チュートリアル壁は当たり判定フラグが false の場合は 0（空気）として扱う
	if (id == 13 && !g_TutorialWallEnabled[0]) return 0;
	if (id == 14 && !g_TutorialWallEnabled[1]) return 0;
	if (id == 15 && !g_TutorialWallEnabled[2]) return 0;

	return id;
}

static void ResetTutorialWallsOnFloor3()
{
	// Floor3 配列は書き換えず、当たり判定フラグをリセットするのみ
	g_TutorialWallEnabled[0] = true;
	g_TutorialWallEnabled[1] = true;
	g_TutorialWallEnabled[2] = true;
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
	case 13: // チュートリアル用１つ目の壁（描画なし・当たり判定のみ）
	case 14: // チュートリアル用２つ目の壁（描画なし・当たり判定のみ）
	case 15: // チュートリアル用３つ目の壁（描画なし・当たり判定のみ）
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

	case 98: // 方向指示ブロック
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

				// チュートリアル壁（ID 13/14/15）は描画リストに追加しない（見えない壁）
				if (mcID == 13 || mcID == 14 || mcID == 15) continue;

				FIELD_TYPE type = ConvertMapID(mcID);

				if (type == FIELD_NONE) continue;

				auto isFaceVisible = [&](int myID, int targetY, int targetZ, int targetX) -> bool {
					int neighborID = GetMapBlockID(floor, targetY, targetZ, targetX);

					if (neighborID == 0 || neighborID == 13 || neighborID == 14 || neighborID == 15) return true; // 隣が空気なら絶対に面を描画する

					if (ConvertMapID(neighborID) == FIELD_NONE) return true;

					// 透過ブロック(ガラス等)を並べた時、隣が同じブロックなら接合面を消す
					if (myID == neighborID) return false;

					// ID:2/3/13/14/15 は同じ壁テクスチャ系として隣接時の接合面を消す
					if ((myID == 2 || myID == 3 ) &&
						(neighborID == 2 || neighborID == 3)) return false;

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
		// 壁ブロック(ID:2,3,13,14,15)はy軸でテクスチャが決まるのでmapYでソート
		bool aIsWall = (a.blockID == 2 || a.blockID == 3 || a.blockID == 13 || a.blockID == 14 || a.blockID == 15);
		bool bIsWall = (b.blockID == 2 || b.blockID == 3 || b.blockID == 13 || b.blockID == 14 || b.blockID == 15);
		if (aIsWall && bIsWall) return a.mapY < b.mapY;
		if (aIsWall != bIsWall) return aIsWall < bIsWall;
		return a.blockID < b.blockID;
		});

	// 2階の場合: マップチップZ=0～20の真下にFloor1を描画用リストとして構築
	// 配列インデックス: idx = (MAP_LENGTH-1) - Z なので Z=0→idx=40, Z=20→idx=20
	// Floor2の底面はpos.y=-1 (Y=0)。Floor1を8段下（1フロア分）に配置する
	if (floor == 1)
	{
		const float SUB_Y_OFFSET = -8.0f; // 1フロア分 (MAP_HEIGHT=8) 下にずらす
		const int SUB_Z_IDX_MIN = 20;     // マップチップZ=20 → 配列インデックス 40-20=20
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

	// 1階・2階・3階の天井を生成（描画専用、室内から見上げると見える下面をY=7の高さに配置）
	g_CeilingList.clear();
	if (floor == 0 || floor == 1 || floor == 2)
	{
		// pos.y = Y-1.0f → Y=7 なら 6.0f
		// 下面（face[2]）を有効にすることで、山上からは見えず室内から見上げると見える
		const float CEIL_Y = 6.0f;
		// 3x3パターン定義（行=z%3、列=x%3）
		// 0=中央(kabeue/blockID=200), 1=角(tenjou_kado/blockID=201), 2=辺(tenjou_hen/blockID=202)
		// パターン: 1,2,1 / 2,0,2 / 1,2,1
		static const int s_pattern[3][3] = {
			{ 1, 2, 1 },
			{ 2, 0, 2 },
			{ 1, 2, 1 },
		};

		for (int z = 0; z < MAP_H; z++)
		{
			for (int x = 0; x < MAP_W; x++)
			{
				// マップ外（建物外）判定：Y=0,1 がともに 0 のマスはマップ範囲外なので天井を置かない
				int floorID = GetMapBlockID(floor, 0, z, x);
				int wallID0  = GetMapBlockID(floor, 1, z, x);
				if (floorID == 0 && wallID0 == 0) continue;

				// Y=1 が壁ブロックの場合は天井を置かない（1階・2階・3階共通）
				FIELD_TYPE wallType = ConvertMapID(wallID0);
				if (wallType == FIELD_BOX) continue;

			// 吹き抜け判定：1階のみ。天井層(Y=MAP_HEIGHT-1)に階段ブロックがあれば吹き抜けとして天井を置かない
				// 2階・3階は吹き抜けなしのため常に天井を描画する
				if (floor == 0)
				{
					int ceilID = GetMapBlockID(floor, MAP_HEIGHT - 1, z, x);
					FIELD_TYPE ceilType = ConvertMapID(ceilID);
					if (ceilType == FIELD_STAIRS_UP || ceilType == FIELD_STAIRS_DOWN) continue;
				}

				int px = x % 3;
				int pz = z % 3;
				int patVal = s_pattern[pz][px];

				// blockID: 200=中央, 201=角, 202=辺
				int blockID = 200 + patVal;

				// rotY: 角ピースのみ四隅で回転、辺ピースは横辺(pz==1)のみ90°
				float rotY = 0.0f;
				if (patVal == 1)
				{
					// 四隅: (pz,px) = (0,0)->90°, (0,2)->180°, (2,2)->270°, (2,0)->0°
					if      (pz == 0 && px == 0) rotY = XM_PIDIV2;
					else if (pz == 0 && px == 2) rotY = XM_PI;
					else if (pz == 2 && px == 2) rotY = XM_PI + XM_PIDIV2;
					else if (pz == 2 && px == 0) rotY = 0.0f;
				}
				else if (patVal == 2)
				{
					// 辺: (0,1)=上辺->180°, (1,0)=左辺->90°, (1,2)=右辺->270°, (2,1)=下辺->0°
					if      (pz == 0 && px == 1) rotY = XM_PI;
					else if (pz == 1 && px == 0) rotY = XM_PIDIV2;
					else if (pz == 1 && px == 2) rotY = XM_PI + XM_PIDIV2;
					else if (pz == 2 && px == 1) rotY = 0.0f;
				}

				// 下面（face[2]）のみ描画→室内から見上げると見える
				MAPDATA data;
				for (int i = 0; i < 6; i++) data.drawFace[i] = false;
				data.drawFace[2] = true;
				data.pos = XMFLOAT3(
					(float)x - offsetX,
					CEIL_Y,
					offsetZ - (float)z
				);
				data.blockID = blockID;
				data.mapY = 7;
				data.no = FIELD_NONE;
				data.isHidden = false;
				data.rotY = rotY;
				data.currentScale = 1.0f;
				g_CeilingList.push_back(data);
			}
		}
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
	g_BlockTextures[15] = LoadTexture(L"asset\\texture\\wood.png");      // ID 15
	g_BlockTextures[16] = LoadTexture(L"asset\\texture\\green.png");	 // ID 16
	g_BlockTextures[17] = LoadTexture(L"asset\\texture\\tairu.png");	 // ID 17
	g_BlockTextures[18] = LoadTexture(L"asset\\texture\\garasu.png");	 // ID 18

	if (g_BlockTextures[0] == nullptr) g_BlockTextures[0] = LoadTexture(L"asset\\texture\\grass.png");

	g_TextureStairs = LoadTexture(L"asset\\texture\\wood.png");

	// 天井テクスチャ（3種類）
	g_CeilingTextures[0] = LoadTexture(L"asset\\texture\\kabeue.png");
	g_CeilingTextures[1] = LoadTexture(L"asset\\texture\\tenjou_kado.png");
	g_CeilingTextures[2] = LoadTexture(L"asset\\texture\\tenjou_hen.png");

	g_CurrentFloor = START_FLOOR - 1; // 3階スタート
	ResetTutorialWallsOnFloor3();
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
		else if (mapData.blockID == 2 || mapData.blockID == 3 || mapData.blockID == 13 || mapData.blockID == 14 || mapData.blockID == 15) {
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
			else if (mapData.blockID >= 200) {
				// 天井専用テクスチャ（blockID = 200 + index）
				int idx = mapData.blockID - 200;
				if (idx < 0 || idx >= CEILING_TEXTURE_NUM || g_CeilingTextures[idx] == nullptr) nextSRV = g_CeilingTextures[0];
				else nextSRV = g_CeilingTextures[idx];
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

	// 天井描画（1階・2階・3階）
	if (!g_CeilingList.empty())
	{
		currentSRV = nullptr;
		for (int i = 0; i < 6; i++) batchListFace[i].clear();

		for (const auto& mapData : g_CeilingList)
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
			if (clipPos.x < -1.2f || clipPos.x > 1.2f ||
				clipPos.y < -1.8f || clipPos.y > 1.8f) continue;
			if (clipPos.z < 0.0f || clipPos.z > 1.0f) continue;

			// blockID 200〜203 → CeilingTextures[0〜3]
			int texIdx = mapData.blockID - 200;
			if (texIdx < 0 || texIdx >= CEILING_TEXTURE_NUM) texIdx = 0;
			ID3D11ShaderResourceView* nextSRV = g_CeilingTextures[texIdx];

			bool overLimit = false;
			for (int i = 0; i < 6; i++) {
				if (batchListFace[i].size() >= MAX_INSTANCES - 1) overLimit = true;
			}
			if (nextSRV != currentSRV || overLimit)
			{
				FlushBatch();
				currentSRV = nextSRV;
			}

			XMMATRIX world = XMMatrixRotationY(mapData.rotY)
				* XMMatrixTranslation(mapData.pos.x, mapData.pos.y, mapData.pos.z);
			XMFLOAT4X4 m;
			XMStoreFloat4x4(&m, XMMatrixTranspose(world));
			// 下面（face[2]）のみ：室内から見上げると見える
			if (mapData.drawFace[2]) batchListFace[2].push_back(m);
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
	for (int i = 0; i < CEILING_TEXTURE_NUM; i++) SAFE_RELEASE(g_CeilingTextures[i]);
	g_MapList.clear();
	g_SubFloorMapList.clear();
	g_CeilingList.clear();
}

void Field_ChangeFloor(int floorIndex)
{
	g_CurrentFloor = floorIndex;
	if (g_CurrentFloor == 2)
	{
		ResetTutorialWallsOnFloor3();
	}
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
		currentDist += 0.8f;
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


	{
		FIELD_TYPE endType = ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, endZ, endX));
		if (endType == FIELD_BOX || endType == FIELD_WALL_PASS ||
			endType == FIELD_STAIRS_UP || endType == FIELD_STAIRS_DOWN)
		{
			int dx[] = { 0, 0, 1, -1 };
			int dz[] = { 1, -1, 0, 0 };
			for (int i = 0; i < 4; i++) {
				int nx = endX + dx[i];
				int nz = endZ + dz[i];
				if (nx >= 0 && nx < MAP_W && nz >= 0 && nz < MAP_H) {
					FIELD_TYPE t = ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, nz, nx));
					if (t != FIELD_BOX && t != FIELD_WALL_PASS &&
						t != FIELD_STAIRS_UP && t != FIELD_STAIRS_DOWN) {
						endX = nx; endZ = nz; break;
					}
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
		std::fill(nodes[i].begin(), nodes[i].end(), Node{ 0, 0, 0.0f, 0.0f, 0, 0 });
	}

	Node startNode = { startX, startZ, 0.0f, 0.0f, -1, -1 };
	openList.push(startNode);
	nodes[startZ][startX] = startNode;

	int dirX[] = { 0, 0, -1, 1, -1, 1, -1, 1 };  // 4方向 + 斜め4方向
	int dirZ[] = { -1, 1, 0, 0, -1, -1, 1, 1 };
	// 斜めコストを高めに設定して、障害物付近では直線移動を優先させる
	float dirCost[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.8f, 1.8f, 1.8f, 1.8f };
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

			// 壁・階段ブロックは通行不可（階段内部への迷い込みを防ぐ）
			FIELD_TYPE nextType = ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, nextZ, nextX));
			if (nextType == FIELD_BOX || nextType == FIELD_WALL_PASS ||
				nextType == FIELD_STAIRS_UP || nextType == FIELD_STAIRS_DOWN) continue;

			// Y=0が空気（床なし）のマスは通行不可
			if (GetMapBlockID(g_CurrentFloor, 0, nextZ, nextX) == 0) continue;

			// 斜め移動の場合、X方向・Z方向それぞれの隣接セルが通路であるか確認（コーナー通り抜け防止）
			if (i >= 4)
			{
				// X方向の隣（同じZで横に移動したセル）
				FIELD_TYPE sideTypeX = ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, current.z, current.x + dirX[i]));
				if (sideTypeX == FIELD_BOX || sideTypeX == FIELD_WALL_PASS ||
					sideTypeX == FIELD_STAIRS_UP || sideTypeX == FIELD_STAIRS_DOWN) continue;
				// Z方向の隣（同じXで縦に移動したセル）
				FIELD_TYPE sideTypeZ = ConvertMapID(GetMapBlockID(g_CurrentFloor, 1, current.z + dirZ[i], current.x));
				if (sideTypeZ == FIELD_BOX || sideTypeZ == FIELD_WALL_PASS ||
					sideTypeZ == FIELD_STAIRS_UP || sideTypeZ == FIELD_STAIRS_DOWN) continue;
				// Y=0（床なし）チェックも両隣に適用
				if (GetMapBlockID(g_CurrentFloor, 0, current.z, current.x + dirX[i]) == 0) continue;
				if (GetMapBlockID(g_CurrentFloor, 0, current.z + dirZ[i], current.x) == 0) continue;
			}

			if (closedList[nextZ][nextX]) continue;

			float newCost = current.cost + dirCost[i];
			float h = (float)(std::abs(endX - nextX) + std::abs(endZ - nextZ));

			Node neighbor = { nextX, nextZ, newCost, h, current.x, current.z };
			openList.push(neighbor);

			// より低コストのパスが見つかった場合のみ更新する
			Node& existing = nodes[nextZ][nextX];
			if (existing.parentX == 0 && existing.parentZ == 0 && !(nextX == startX && nextZ == startZ))
			{
				existing = neighbor;
			}
			else if (newCost < existing.cost)
			{
				existing = neighbor;
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
			int mcID0 = GetMapBlockID(floor, 0, z, x);
			int mcID1 = GetMapBlockID(floor, 1, z, x);
			if (mcID0 == 5 || mcID0 == 6 || mcID1 == 5 || mcID1 == 6)
			{
				float wx = GridToWorldX(x);
				float wz = GridToWorldZ(z);
				positions.push_back({ wx, PATROL_HEIGHT, wz });
			}
		}
	}
	return positions;
}

// 指定フロアのID4（1階出口ブロック）のワールド座標を全て取得する
std::vector<XMFLOAT3> Field_GetFloor1ExitPositions(int floor)
{
	std::vector<XMFLOAT3> positions;
	for (int z = 0; z < MAP_H; z++)
	{
		for (int x = 0; x < MAP_W; x++)
		{
			int mcID0 = GetMapBlockID(floor, 0, z, x);
			int mcID1 = GetMapBlockID(floor, 1, z, x);
			if (mcID0 == 4 || mcID1 == 4)
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

void Field_SetTutorialWall(int wallNumber, bool enabled)
{
	// wallNumber: 1→ID13, 2→ID14, 3→ID15
	if (wallNumber < 1 || wallNumber > 3) return;
	int targetID = 12 + wallNumber; // 13, 14, 15

	g_TutorialWallEnabled[wallNumber - 1] = enabled;

	// マップデータを再構築して描画リスト・壁判定を更新
	if (g_CurrentFloor == 2)
	{
		LoadMapData(g_CurrentFloor);
	}
}