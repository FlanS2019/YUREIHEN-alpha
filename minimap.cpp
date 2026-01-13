#include "minimap.h"
#include "sprite.h"
#include "ghost.h"
#include "busters.h"
#include "Floor1.h"
#include "define.h"
#include <cmath>
     

static Sprite* g_SpriteDot = nullptr;
static Sprite* g_SpritePlayerIcon = nullptr; // プレイヤーアイコン
// ドット描画関数
static void DrawDot(float worldX, float worldZ, float r, float g, float b)
{
	Ghost* player = GetGhost();
	if (!player) return;
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

	g_SpriteDot->SetColor(XMFLOAT4(r, g, b, 1.0f));

	g_SpriteDot->SetPos(XMFLOAT2(screenX, screenY));

	g_SpriteDot->SetSize(XMFLOAT2(BLOCK_SIZE, BLOCK_SIZE));
	g_SpriteDot->Draw();
}

void Minimap_Initialize(void)
{

	// 引数: 位置, サイズ, 回転, 色, ブレンド, 画像パス
	g_SpriteDot = new Sprite(
		XMFLOAT2(0.0f, 0.0f),               // 位置 (後でSetPosで変えるので適当でOK)
		XMFLOAT2(BLOCK_SIZE, BLOCK_SIZE),   // サイズ
		0.0f,                               // 回転
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),   // 色 (白)
		BLENDSTATE_ALFA,              // ブレンド
		L"asset/texture/fade.png"          // 画像
	);

	g_SpritePlayerIcon = new Sprite(
		XMFLOAT2(MINIMAP_POS_X, MINIMAP_POS_Y), // 位置は固定
		XMFLOAT2(20.0f, 20.0f),                 // ★サイズ（少し大きめに設定）
		0.0f,
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),       // 色（白＝画像そのまま）
		BLENDSTATE_ALFA,
		L"asset/texture/icon_ghost.png"        // ★プレイヤー用画像
	);
}

void Minimap_Finalize(void)
{
	if (g_SpriteDot) {
		delete g_SpriteDot;
		g_SpriteDot = nullptr;
	}

	if (g_SpritePlayerIcon) {
		delete g_SpritePlayerIcon;
		g_SpritePlayerIcon = nullptr;
	}
}

void Minimap_Update(void)
{
}

void Minimap_Draw(void)
{
	Ghost* player = GetGhost();
	if (!player || !g_SpriteDot) return;

	// 背景
	g_SpriteDot->SetColor(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f));
	g_SpriteDot->SetPos(XMFLOAT2(MINIMAP_POS_X, MINIMAP_POS_Y));
	g_SpriteDot->SetSize(XMFLOAT2((float)VIEW_RANGE * BLOCK_SIZE * 2.2f, (float)VIEW_RANGE * BLOCK_SIZE * 2.2f));
	g_SpriteDot->Draw();

	// 壁
	XMFLOAT3 playerPos = player->GetPos();
	int px = (int)round(playerPos.x + MAP_WIDTH / 2.0f);
	int pz = (int)round(MAP_LENGTH / 2.0f - playerPos.z);

	for (int z = pz - VIEW_RANGE; z <= pz + VIEW_RANGE; z++)
	{
		for (int x = px - VIEW_RANGE; x <= px + VIEW_RANGE; x++)
		{
			if (x < 0 || x >= MAP_WIDTH || z < 0 || z >= MAP_LENGTH) continue;

			// 壁があるかチェック (Y=1:壁の層)
			// Floor1.h の定義に従うなら、0以外かつ99以外などを壁とする
			int id = Floor1[1][z][x];
			if (id != 0 && id != 99)
			{
				float wx = (float)x - MAP_WIDTH / 2.0f;
				float wz = MAP_LENGTH / 2.0f - (float)z;
				DrawDot(wx, wz, 0.5f, 0.5f, 0.5f);
			}
		}
	}

	// 敵
	Busters* buster = GetBusters();
	if (buster)
	{
		XMFLOAT3 bPos = buster->GetPos();
		DrawDot(bPos.x, bPos.z, 1.0f, 0.0f, 0.0f);
	}

	// 自分
	g_SpritePlayerIcon->SetColor(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
	g_SpritePlayerIcon->SetPos(XMFLOAT2(MINIMAP_POS_X, MINIMAP_POS_Y));
	g_SpritePlayerIcon->SetSize(XMFLOAT2(BLOCK_SIZE * 1.5f, BLOCK_SIZE * 1.5f));
	g_SpritePlayerIcon->Draw();
}