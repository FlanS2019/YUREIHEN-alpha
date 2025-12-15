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

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11Buffer* g_IndexBuffer = NULL;

static ID3D11ShaderResourceView* g_TextureBox;   // 箱用
static ID3D11ShaderResourceView* g_TextureStairs;// 階段用

XMFLOAT3 rotateBox = XMFLOAT3(0, 0, 0);
static std::vector<MAPDATA> g_MapList;

// 現在の階層 (0=1階, 1=2階, 2=3階)
static int g_CurrentFloor = 0;

// =====================================================================
// マップデザイン (3階層分)
// 0:なし, 1:床, 2:壁, 3:上り階段, 4:下り階段
// =====================================================================

#define MAP_W (41)
#define MAP_H (41)

// マップデータ (3次元配列)
static int LevelMap[MAP_FLOORS][MAP_H][MAP_W] = {
	// --- 1階 (Floor 0) ---
	{
		{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,3,3,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,3,3,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,3,3,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,2,2,1,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2}, // 3: 上り階段(中央)
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,2,2,1,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
	},
	// --- 2階 (Floor 1) ---
	{
		{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2},
		{2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2},
		{2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2},
		{2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2},
		{2,1,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,1,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,4,4,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,4,4,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,4,4,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,2,2,1,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2}, // 4: 下り階段(中央)
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,2,2,1,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,3,3,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,3,3,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,3,3,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, // 3: 上り階段(入口奥)
	},
	// --- 3階 (Floor 2) ---
	{
		{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2},
		{2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2},
		{2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2},
		{2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2},
		{2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,2,2,1,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,2,2,1,2,2,2,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,4,4,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,4,4,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,4,4,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2},
		{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, // 4: 下り階段(入口奥)
	},
};

// 内部関数: 座標変換
static int WorldToGridX(float x) { return (int)round(x + MAP_W / 2.0f); }
static int WorldToGridZ(float z) { return (int)round(MAP_H / 2.0f - z); }
static float GridToWorldX(int gx) { return (float)gx - MAP_W / 2.0f; }
static float GridToWorldZ(int gz) { return MAP_H / 2.0f - (float)gz; }

// マップデータ再読み込み（階層が変わったときに呼ぶ）
void LoadMapData(int floor)
{
	if (floor < 0 || floor >= MAP_FLOORS) return;

	g_MapList.clear();

	// 中心を(0,0)にするためのオフセット
	float offsetX = MAP_W / 2.0f;
	float offsetZ = MAP_H / 2.0f;

	for (int z = 0; z < MAP_H; z++)
	{
		for (int x = 0; x < MAP_W; x++)
		{
			int blockType = LevelMap[floor][z][x];

			// 1(床) 以上なら、まず「床」を作る
			if (blockType >= FIELD_BOX)
			{
				MAPDATA floorData;
				floorData.pos = XMFLOAT3((x - offsetX), -1.0f, (offsetZ - z));
				floorData.no = FIELD_BOX;
				g_MapList.push_back(floorData);
			}

			// 壁、階段の作成
			if (blockType == 2) // 壁
			{
				for (int h = 0; h < 5; h++)
				{
					MAPDATA wallData;
					// Y座標を 0.0, 1.0, 2.0... とずらして配置
					wallData.pos = XMFLOAT3((x - offsetX), (float)h, (offsetZ - z));
					wallData.no = FIELD_BOX;
					wallData.isHidden = false;
					g_MapList.push_back(wallData);
				}
			}
			else if (blockType == 3) // 上り階段
			{
				MAPDATA stairData;
				stairData.pos = XMFLOAT3((x - offsetX), 0.0f, (offsetZ - z));
				stairData.no = FIELD_STAIRS_UP;
				g_MapList.push_back(stairData);
			}
			else if (blockType == 4) // 下り階段
			{
				MAPDATA stairData;
				stairData.pos = XMFLOAT3((x - offsetX), 0.0f, (offsetZ - z));
				stairData.no = FIELD_STAIRS_DOWN;
				g_MapList.push_back(stairData);
			}
		}
	}
}

void Field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_TextureBox = LoadTexture(L"asset\\texture\\grass.png");
	g_TextureStairs = LoadTexture(L"asset\\texture\\wood.png"); // 階段用（なければgrass.pngでも可）

	// 3階からスタート
	g_CurrentFloor = 2;
	LoadMapData(g_CurrentFloor);

	if (!g_MapList.empty()) {
		CreateBox(pDevice, pContext, &g_VertexBuffer, &g_IndexBuffer);
	}
}

// field.cpp の Field_Update 関数

void Field_Update(void)
{
	// 1. 非表示フラグ管理用の一時配列 (falseで初期化)
	// スタックオーバーフローを防ぐためstaticにするか、vectorを使う
	std::vector<std::vector<bool>> shouldHide(MAP_H, std::vector<bool>(MAP_W, false));

	Ghost* ghost = GetGhost();
	if (!ghost) return;

	// 2. カメラとプレイヤーの位置を取得
	XMFLOAT3 cameraPos = GetCamera()->GetPos();
	XMFLOAT3 playerPos = ghost->GetPos();

	// プレイヤーの少し上(頭あたり)を目標にする
	playerPos.y += 1.0f;

	// 3. レイキャスト (カメラ -> プレイヤー)
	float dx = playerPos.x - cameraPos.x;
	float dz = playerPos.z - cameraPos.z;
	float dist = sqrtf(dx * dx + dz * dz);

	// 近すぎる場合は計算不要
	if (dist < 0.5f) return;

	float stepX = dx / dist;
	float stepZ = dz / dist;

	// カメラからプレイヤーの手前(1.0m)までチェック
	float currentDist = 0.0f;

	while (currentDist < dist - 1.0f)
	{
		float checkX = cameraPos.x + stepX * currentDist;
		float checkZ = cameraPos.z + stepZ * currentDist;

		// グリッド座標に変換
		int gridX = WorldToGridX(checkX);
		int gridZ = WorldToGridZ(checkZ);

		// 範囲内かつ、壁がある場所なら
		if (gridX >= 0 && gridX < MAP_W && gridZ >= 0 && gridZ < MAP_H)
		{
			// 現在の階層で壁かどうかチェック
			if (LevelMap[g_CurrentFloor][gridZ][gridX] == 2)
			{
				// ヒットした壁の「周囲1マス」も含めて非表示フラグを立てる
				// これで「点」ではなく「太い線（3x3の幅）」で壁が消える
				int range = 2; // 半径 (1なら3x3, 2なら5x5)

				for (int oz = -range; oz <= range; oz++)
				{
					for (int ox = -range; ox <= range; ox++)
					{
						int targetX = gridX + ox;
						int targetZ = gridZ + oz;

						// 配列外参照チェック
						if (targetX >= 0 && targetX < MAP_W && targetZ >= 0 && targetZ < MAP_H)
						{
							shouldHide[targetZ][targetX] = true;
						}
					}
				}
			}
		}

		currentDist += 0.5f; // 0.5m進める
	}

	// 4. マップデータに反映
	for (auto& mapData : g_MapList)
	{
		// 座標から配列インデックスを計算
		int mapGridX = WorldToGridX(mapData.pos.x);
		int mapGridZ = WorldToGridZ(mapData.pos.z);

		// 範囲チェック
		if (mapGridX >= 0 && mapGridX < MAP_W && mapGridZ >= 0 && mapGridZ < MAP_H)
		{
			// 床(Y=-1.0)は消さない。壁(Y>=0.0)で、かつフラグが立っていたら消す
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

		// テクスチャ切り替え
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

// 階層切り替え
void Field_ChangeFloor(int floorIndex)
{
	if (floorIndex < 0 || floorIndex >= MAP_FLOORS) return;
	g_CurrentFloor = floorIndex;
	LoadMapData(g_CurrentFloor);
}

int Field_GetCurrentFloor(void)
{
	return g_CurrentFloor;
}

// 壁判定等は現在の階層のマップデータを参照する
FIELD_TYPE Field_GetBlockType(float x, float z)
{
	int mapX = WorldToGridX(x);
	int mapZ = WorldToGridZ(z);

	if (mapX >= 0 && mapX < MAP_W && mapZ >= 0 && mapZ < MAP_H)
	{
		int type = LevelMap[g_CurrentFloor][mapZ][mapX];
		if (type == 2) return FIELD_BOX;
		if (type == 3) return FIELD_STAIRS_UP;
		if (type == 4) return FIELD_STAIRS_DOWN;
	}
	return FIELD_NONE;
}

// ----------------------------------------------------------------
// 全ての壁を判定（Busters, 視線判定用）
// ----------------------------------------------------------------
bool Field_IsWall(float x, float z)
{
	int mapX = WorldToGridX(x);
	int mapZ = WorldToGridZ(z);

	if (mapX >= 0 && mapX < MAP_W && mapZ >= 0 && mapZ < MAP_H)
	{
		// 壁ブロック(2)なら true
		if (LevelMap[g_CurrentFloor][mapZ][mapX] == 2)
		{
			return true;
		}
	}
	return false;
}

// ----------------------------------------------------------------
// 外周の壁だけ判定（Ghost用）
// ----------------------------------------------------------------
bool Field_IsOuterWall(float x, float z)
{
	int mapX = WorldToGridX(x);
	int mapZ = WorldToGridZ(z);

	// マップ範囲外なら壁扱い
	if (mapX < 0 || mapX >= MAP_W || mapZ < 0 || mapZ >= MAP_H) return true;

	// 外周（端っこ）の壁ブロックなら true
	if (mapX == 0 || mapX == MAP_W - 1 || mapZ == 0 || mapZ == MAP_H - 1)
	{
		if (LevelMap[g_CurrentFloor][mapZ][mapX] == 2)
		{
			return true;
		}
	}

	return false;
}

bool Field_IsWall(float x, float y, float z)
{
	if (y > -0.5f) return Field_IsWall(x, z);
	return false;
}

bool Field_CheckWallBetween(XMFLOAT3 start, XMFLOAT3 end)
{
	float dx = end.x - start.x;
	float dz = end.z - start.z;
	float dist = sqrtf(dx * dx + dz * dz);

	// 距離が近すぎる場合は「壁なし」とみなす
	if (dist < 0.5f) return false;

	// 正規化（1.0m単位のベクトルにする）
	float stepX = dx / dist;
	float stepZ = dz / dist;

	// 0.5m刻みで少しずつ進んで壁があるか調べる
	// ※始点(自分)と終点(相手)の座標そのものはチェックしないように少し内側を見る
	float currentDist = 0.5f;

	while (currentDist < dist - 0.5f)
	{
		float checkX = start.x + stepX * currentDist;
		float checkZ = start.z + stepZ * currentDist;

		// 現在の階層で壁判定
		if (Field_IsWall(checkX, checkZ))
		{
			return true; // 途中で壁に当たった
		}

		currentDist += 0.5f; // 0.5m進める
	}

	return false; // 最後まで壁がなかった
}

float Field_GetFloorY(float x, float y, float z)
{
	int mapX = WorldToGridX(x);
	int mapZ = WorldToGridZ(z);

	if (mapX >= 0 && mapX < MAP_W && mapZ >= 0 && mapZ < MAP_H)
	{
		if (LevelMap[g_CurrentFloor][mapZ][mapX] >= 1) return 0.0f;
	}
	return -999.0f;
}

// =========================================================
// 経路探索
// =========================================================

// Node構造体 (関数より前に定義)
struct Node {
	int x, z;
	float cost;      // 実移動コスト
	float heuristic; // 推定コスト
	int parentX, parentZ; // 親ノード（経路復元用）

	// 優先度付きキューのための比較演算子 (コストが小さい方が優先)
	bool operator>(const Node& other) const {
		return (cost + heuristic) > (other.cost + other.heuristic);
	}
};

std::vector<XMFLOAT3> Field_FindPath(XMFLOAT3 start, XMFLOAT3 end)
{
	std::vector<XMFLOAT3> path;

	// スタートとゴールのグリッド座標を計算
	int startX = WorldToGridX(start.x);
	int startZ = WorldToGridZ(start.z);
	int endX = WorldToGridX(end.x);
	int endZ = WorldToGridZ(end.z);

	// 範囲外チェック
	if (startX < 0 || startX >= MAP_W || startZ < 0 || startZ >= MAP_H ||
		endX < 0 || endX >= MAP_W || endZ < 0 || endZ >= MAP_H)
	{
		return path; // 空のパスを返す
	}

	// ゴールが壁なら補正 (現在の階層を参照)
	if (LevelMap[g_CurrentFloor][endZ][endX] == 2)
	{
		int dx[] = { 0, 0, 1, -1 };
		int dz[] = { 1, -1, 0, 0 };
		for (int i = 0; i < 4; i++) {
			int nx = endX + dx[i];
			int nz = endZ + dz[i];
			if (nx >= 0 && nx < MAP_W && nz >= 0 && nz < MAP_H && LevelMap[g_CurrentFloor][nz][nx] != 2) {
				endX = nx; endZ = nz; break;
			}
		}
	}

	// 探索用データ
	std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;

	// 訪問済みリスト (vectorのvectorで確保)
	std::vector<std::vector<bool>> closedList(MAP_H, std::vector<bool>(MAP_W, false));
	std::vector<std::vector<Node>> nodes(MAP_H, std::vector<Node>(MAP_W));

	// スタートノード追加
	Node startNode = { startX, startZ, 0.0f, 0.0f, -1, -1 };
	openList.push(startNode);
	nodes[startZ][startX] = startNode;

	// 方向（上下左右）
	int dirX[] = { 0, 0, -1, 1 };
	int dirZ[] = { -1, 1, 0, 0 };

	bool found = false;

	while (!openList.empty())
	{
		Node current = openList.top();
		openList.pop();

		// ゴール到達
		if (current.x == endX && current.z == endZ)
		{
			found = true;
			break;
		}

		if (closedList[current.z][current.x]) continue;
		closedList[current.z][current.x] = true;

		// 隣接ノードを探索
		for (int i = 0; i < 4; i++)
		{
			int nextX = current.x + dirX[i];
			int nextZ = current.z + dirZ[i];

			// マップ範囲外チェック
			if (nextX < 0 || nextX >= MAP_W || nextZ < 0 || nextZ >= MAP_H) continue;

			// 壁判定 (2は壁) ※現在の階層を参照
			if (LevelMap[g_CurrentFloor][nextZ][nextX] == 2) continue;

			// 既に確定済みならスキップ
			if (closedList[nextZ][nextX]) continue;

			float newCost = current.cost + 1.0f;

			// ヒューリスティック（マンハッタン距離）
			float h = (float)(std::abs(endX - nextX) + std::abs(endZ - nextZ));

			// 新しいノードを作成して追加
			Node neighbor = { nextX, nextZ, newCost, h, current.x, current.z };
			openList.push(neighbor);

			// 親情報を記録（簡易版：未訪問なら更新）
			if (nodes[nextZ][nextX].parentX == 0 && nodes[nextZ][nextX].parentZ == 0)
			{
				nodes[nextZ][nextX] = neighbor;
			}
		}
	}

	// 経路復元 (ゴールから親を辿ってスタートまで戻る)
	if (found)
	{
		int cx = endX;
		int cz = endZ;

		// 無限ループ防止のため最大ステップ数を設ける
		int maxSteps = MAP_W * MAP_H;
		int steps = 0;

		while (cx != -1 && cz != -1 && steps < maxSteps)
		{
			// ワールド座標に変換して追加
			path.push_back({ GridToWorldX(cx), 0.0f, GridToWorldZ(cz) });

			Node& n = nodes[cz][cx];
			int px = n.parentX;
			int pz = n.parentZ;

			// スタート地点まで戻ったら終了
			if (cx == startX && cz == startZ) break;

			cx = px;
			cz = pz;
			steps++;
		}
	}

	return path;
}