#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "direct3d.h"
#include "shader.h"

using namespace DirectX;

// ƒuƒƒbƒN‚Ìí—Ş
enum FIELD_TYPE
{
	FIELD_NONE = 0,
	FIELD_BOX,			// •ÇE°
	FIELD_STAIRS_UP,	// ã‚èŠK’i (“¥‚Ş‚Æã‚ÌŠK‚Ö)
	FIELD_STAIRS_DOWN,	// ‰º‚èŠK’i (“¥‚Ş‚Æ‰º‚ÌŠK‚Ö)
	FIELD_MAX
};

class MAPDATA
{
public:
	XMFLOAT3 pos;
	FIELD_TYPE no;
	float rotY;
};

void Field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Field_Finalize(void);
void Field_Draw(void);
void Field_Update(void);

// ”»’è—p
bool Field_IsFloor(float x, float z);
FIELD_TYPE Field_GetBlockType(float x, float z);
bool Field_IsWall(float x, float z);
bool Field_IsWall(float x, float y, float z);
float Field_GetFloorY(float x, float y, float z);

std::vector<XMFLOAT3> Field_FindPath(XMFLOAT3 start, XMFLOAT3 end);

// š’Ç‰Á: ŠK‘w‘€ì—p
void Field_ChangeFloor(int floorIndex); // w’è‚µ‚½ŠK‘w‚ÉØ‚è‘Ö‚¦‚é
int Field_GetCurrentFloor(void);        // Œ»İ‚ÌŠK‘w‚ğæ“¾