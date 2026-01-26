#include "minimap.h"
#include "sprite.h"
#include "ghost.h"
#include "busters.h"
#include "Floor1.h"
#include "define.h"
#include <cmath>

#define MINIMAP_POS_X (MINIMAP_POS_OFFSET)
#define MINIMAP_POS_Y (SCREEN_HEIGHT - MINIMAP_POS_OFFSET)

static Sprite* g_MiniMapBG = nullptr;
static Sprite* g_MiniMapDot = nullptr;
static Sprite* g_GhostIcon = nullptr; // プレイヤーアイコン
static Sprite* g_BusterIcon = nullptr; // 敵アイコン

void Minimap_Initialize(void)
{

	// 引数: 位置, サイズ, 回転, 色, ブレンド, 画像パス
	g_MiniMapBG = new Sprite(
		{ MINIMAP_POS_X, MINIMAP_POS_Y },    // 位置
		{ (float)VIEW_RANGE * BLOCK_SIZE * 2.2f, (float)VIEW_RANGE * BLOCK_SIZE * 2.2f },   // サイズ
		0.0f,                               // 回転
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f),   // 色 (白)
		BLENDSTATE_ALFA,					// ブレンド
		L"asset/texture/fade.png"           // 画像
	);

	g_MiniMapDot = new Sprite(
		{ 0,0 },    // 位置 (後でSetPosで変えるので適当でOK)
		{ 0,0 },   // サイズ
		0.0f,                               // 回転
		{ 0.0f, 0.0f, 0.0f, 0.5f },   // 色 (白)
		BLENDSTATE_ALFA,					// ブレンド
		L"asset/texture/fade.png"           // 画像
	);

	g_GhostIcon = new Sprite(
		XMFLOAT2(MINIMAP_POS_X, MINIMAP_POS_Y), // 位置は固定
		XMFLOAT2(50.0f, 50.0f),                 // サイズ（少し大きめに設定）
		0.0f,									// 回転
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),       // 色（白＝画像そのまま）
		BLENDSTATE_ALFA,						// ブレンド
		L"asset/texture/icon_ghost.png"         // プレイヤー用画像
	);

	g_BusterIcon = new Sprite(
		XMFLOAT2(MINIMAP_POS_X, MINIMAP_POS_Y), // 位置は固定
		XMFLOAT2(50.0f, 50.0f),                 // サイズ（少し大きめに設定）
		0.0f,									// 回転
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),       // 色（白＝画像そのまま）
		BLENDSTATE_ALFA,						// ブレンド
		L"asset/texture/icon_buster.png"         // プレイヤー用画像
	);
}

void Minimap_Finalize(void)
{
	if (g_MiniMapBG) {
		delete g_MiniMapBG;
		g_MiniMapBG = nullptr;
	}

	if (g_GhostIcon) {
		delete g_GhostIcon;
		g_GhostIcon = nullptr;
	}
}

void Minimap_Update(void)
{
	// 敵
	Busters* buster = GetBusters();
	if (buster)
	{
		g_BusterIcon->SetPos({ buster->GetPos().x, buster->GetPos().z });
	}


}

void Minimap_Draw(void)
{
	Ghost* ghost = GetGhost();
	if (!ghost || !g_MiniMapBG) return;

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
			if (x < 0 || x >= MAP_WIDTH || z < 0 || z >= MAP_LENGTH) continue;

			// 壁があるかチェック (Y=1:壁の層)
			int id = Floor1[1][z][x];
			if (id != 0 && id != 98 && id != 99)
			{
				float wx = (float)x - MAP_WIDTH / 2.0f;
				float wz = MAP_LENGTH / 2.0f - (float)z;
				DrawMiniMapDot(wx, wz, 0.5f, 0.5f, 0.5f);
			}
		}
	}

	g_BusterIcon->Draw();
	g_GhostIcon->Draw();
}


// ドット描画関数
static void DrawMiniMapDot(float worldX, float worldZ, float r, float g, float b)
{
	Ghost* player = GetGhost();
	if (!player || !g_MiniMapBG) return;
	XMFLOAT3 playerPos = player->GetPos();

	float diffX = worldX - playerPos.x;
	float diffZ = worldZ - playerPos.z;

	float screenX = MINIMAP_POS_X + (diffX * BLOCK_SIZE);
	float screenY = MINIMAP_POS_Y - (diffZ * BLOCK_SIZE);

	float limit = VIEW_RANGE * BLOCK_SIZE;
	if (screenX < MINIMAP_POS_X - limit || screenX > MINIMAP_POS_X + limit ||
		screenY < MINIMAP_POS_Y - limit || screenY > MINIMAP_POS_Y + limit)
	{
		return;
	}

	g_MiniMapDot->SetColor(XMFLOAT4(r, g, b, 1.0f));
	g_MiniMapDot->SetPos(XMFLOAT2(screenX, screenY));
	g_MiniMapDot->SetSize(XMFLOAT2(BLOCK_SIZE, BLOCK_SIZE));
	g_MiniMapDot->Draw();
}
