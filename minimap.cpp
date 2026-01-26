#include "minimap.h"
#include "sprite.h"
#include "ghost.h"
#include "busters.h"
#include "field.h"
#include "define.h"
#include <cmath>

// 全階層のデータを読み込む
#include "Floor1.h"
#include "Floor2.h"
#include "Floor3.h"

#define MINIMAP_POS_X (MINIMAP_POS_OFFSET)
#define MINIMAP_POS_Y (SCREEN_HEIGHT - MINIMAP_POS_OFFSET)

static Sprite* g_MiniMapBG = nullptr;
static Sprite* g_MiniMapDot = nullptr;
static Sprite* g_GhostIcon = nullptr; // プレイヤーアイコン
static Sprite* g_BusterIcon = nullptr; // 敵アイコン

// 前方宣言
static void DrawMiniMapDot(float worldX, float worldZ, float r, float g, float b);
static void DrawMiniMapIcon(Sprite* sprite, float worldX, float worldZ);

// 階層ごとのブロックID取得ヘルパー
static int GetMapBlockID(int floor, int y, int z, int x)
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

void Minimap_Initialize(void)
{
	// 背景
	g_MiniMapBG = new Sprite(
		{ MINIMAP_POS_X, MINIMAP_POS_Y },
		{ (float)VIEW_RANGE * BLOCK_SIZE * 2.2f, (float)VIEW_RANGE * BLOCK_SIZE * 2.2f },
		0.0f,
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f),
		BLENDSTATE_ALFA,
		L"asset/texture/fade.png"
	);

	// 壁（ドット）
	g_MiniMapDot = new Sprite(
		{ 0,0 },
		{ 0,0 },
		0.0f,
		{ 0.0f, 0.0f, 0.0f, 0.5f },
		BLENDSTATE_ALFA,
		L"asset/texture/fade.png"
	);

	// プレイヤーアイコン (中心固定)
	g_GhostIcon = new Sprite(
		XMFLOAT2(MINIMAP_POS_X, MINIMAP_POS_Y),
		XMFLOAT2(30.0f, 30.0f), // サイズ調整
		0.0f,
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		BLENDSTATE_ALFA,
		L"asset/texture/icon_ghost.png"
	);

	// バスターズアイコン
	g_BusterIcon = new Sprite(
		XMFLOAT2(MINIMAP_POS_X, MINIMAP_POS_Y),
		XMFLOAT2(30.0f, 30.0f), // サイズ調整
		0.0f,
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		BLENDSTATE_ALFA,
		L"asset/texture/icon_buster.png"
	);
}

void Minimap_Finalize(void)
{
	if (g_MiniMapBG) { delete g_MiniMapBG; g_MiniMapBG = nullptr; }
	if (g_MiniMapDot) { delete g_MiniMapDot; g_MiniMapDot = nullptr; }
	if (g_GhostIcon) { delete g_GhostIcon; g_GhostIcon = nullptr; }
	if (g_BusterIcon) { delete g_BusterIcon; g_BusterIcon = nullptr; }
}

void Minimap_Update(void)
{

}

void Minimap_Draw(void)
{
	Ghost* ghost = GetGhost();
	if (!ghost || !g_MiniMapBG) return;

	// 背景描画
	g_MiniMapBG->Draw();

	// 壁を最小限だけ描画（フレームレート向上のため）
	XMFLOAT3 playerPos = ghost->GetPos();
	int px = (int)round(playerPos.x + MAP_WIDTH / 2.0f);
	int pz = (int)round(MAP_LENGTH / 2.0f - playerPos.z);

	// フルレンジで壁を描画
	int reducedRange = VIEW_RANGE;

	for (int z = pz - reducedRange; z <= pz + reducedRange; z++)
	{
		for (int x = px - reducedRange; x <= px + reducedRange; x++)
		{
			// ヘルパー関数を使って、現在の階のブロックIDを取得
			// Y=1 (壁レイヤー) を参照
			int id = GetMapBlockID(currentFloor, 1, z, x);

			// 壁があるかチェック (Y=1:壁の層)
			int id = Floor1[1][z][x];
			int id = Floor1[1][z][x];
			if (id != 0 && id != 98 && id != 99)
			{
				float wx = (float)x - MAP_WIDTH / 2.0f;
				float wz = MAP_LENGTH / 2.0f - (float)z;
				DrawMiniMapDot(wx, wz, 0.5f, 0.5f, 0.5f);
			}
		}
	}

	// 敵アイコン描画
	Busters* buster = GetBusters();
	if (buster)
	{
		// 敵のワールド座標をミニマップ座標に変換して描画
		DrawMiniMapIcon(g_BusterIcon, buster->GetPos().x, buster->GetPos().z);
	}

	// 4. プレイヤーアイコン描画 (中心)
	g_GhostIcon->Draw();
}


// ドット描画関数
static void DrawMiniMapDot(float worldX, float worldZ, float r, float g, float b)
{
	Ghost* player = GetGhost();
	if (!player || !g_MiniMapDot) return;
	XMFLOAT3 playerPos = player->GetPos();

	// プレイヤーからの相対位置
	float diffX = worldX - playerPos.x;
	float diffZ = worldZ - playerPos.z;

	// 画面上の座標に変換
	float screenX = MINIMAP_POS_X + (diffX * BLOCK_SIZE);
	float screenY = MINIMAP_POS_Y - (diffZ * BLOCK_SIZE);

	// ミニマップの範囲外なら描画しない
	float limit = VIEW_RANGE * BLOCK_SIZE;
	if (screenX < MINIMAP_POS_X - limit || screenX > MINIMAP_POS_X + limit ||
		screenY < MINIMAP_POS_Y - limit || screenY > MINIMAP_POS_Y + limit)
	{
		return;
	}

	g_MiniMapDot->SetColor(XMFLOAT4(r, g, b, 1.0f));
	g_MiniMapDot->SetPos(XMFLOAT2(screenX, screenY));
	g_MiniMapDot->SetSize(XMFLOAT2(BLOCK_SIZE, BLOCK_SIZE)); // マップチップサイズ
	g_MiniMapDot->Draw();
}

// アイコン描画用ヘルパー関数
static void DrawMiniMapIcon(Sprite* sprite, float worldX, float worldZ)
{
	Ghost* player = GetGhost();
	if (!player || !sprite) return;
	XMFLOAT3 playerPos = player->GetPos();

	float diffX = worldX - playerPos.x;
	float diffZ = worldZ - playerPos.z;

	float screenX = MINIMAP_POS_X + (diffX * BLOCK_SIZE);
	float screenY = MINIMAP_POS_Y - (diffZ * BLOCK_SIZE);

	float limit = VIEW_RANGE * BLOCK_SIZE;

	float margin = 20.0f;
	if (screenX < MINIMAP_POS_X - limit - margin || screenX > MINIMAP_POS_X + limit + margin ||
		screenY < MINIMAP_POS_Y - limit - margin || screenY > MINIMAP_POS_Y + limit + margin)
	{
		return;
	}

	sprite->SetPos(XMFLOAT2(screenX, screenY));
	sprite->Draw();
}