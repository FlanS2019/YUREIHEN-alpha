#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "direct3d.h"
#include "shader.h"
using namespace DirectX;

// ブロックの種類
enum FIELD_TYPE
{
	FIELD_NONE = 0,
	FIELD_BOX,			// 壁・床
	FIELD_STAIRS_UP,	// 上り階段 (踏むと上の階へ)
	FIELD_STAIRS_DOWN,	// 下り階段 (踏むと下の階へ)
	FIELD_MAX
};

class MAPDATA
{
public:
	XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
	FIELD_TYPE no = FIELD_NONE;
	float rotY = 0.0f;
	bool isHidden = false;

	int blockID = 0;

	float currentScale = 1.0f;

	bool drawFace[6] = { true, true, true, true, true, true };
};

void Field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Field_Finalize(void);
void Field_Draw(void);
void Field_Update(void);

void LoadMapData(int floor);
// 判定用
FIELD_TYPE Field_GetBlockType(float x, float z);
bool Field_IsWall(float x, float z);
bool Field_IsWall(float x, float y, float z);
bool Field_IsOuterWall(float x, float z);
float Field_GetFloorY(float x, float y, float z);

float Field_CalculateRotationFromMarker(float x, float y, float z);

bool Field_CheckWallBetween(XMFLOAT3 start, XMFLOAT3 end);

std::vector<XMFLOAT3> Field_FindPath(XMFLOAT3 start, XMFLOAT3 end);

// 階層操作用
void Field_ChangeFloor(int floorIndex); // 指定した階層に切り替える
int Field_GetCurrentFloor(void);        // 現在の階層を取得

// 指定フロアのSTAIRS_UPブロックのワールド座標を返す（見つからない場合は {0,0,0}）
XMFLOAT3 Field_GetStairsUpWorldPos(int floor);
