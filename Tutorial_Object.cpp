#pragma execution_character_set("utf-8")
#include <cmath>
#include <DirectXMath.h>
using namespace DirectX;
#include "Tutorial_Object.h"
#include "sprite3d.h"
#include "field.h"
#include "ghost.h"
#include "UI_Tutorial.h"
#include "furniture.h"

// ==========================================
// 円盤（enban）Sprite3D
// ==========================================
static Sprite3D* g_pEnban = nullptr;
static bool      g_EnbanTouched = false;
static bool      g_PianoPossessed = false;

void TutorialObject_Initialize(void)
{
	g_pEnban = new Sprite3D(
		{ -5.0f, 0.5f, 17.0f },
		{ 4.0f, 1.0f, 4.0f },
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\enban.fbx"
	);

	g_EnbanTouched = false;
	g_PianoPossessed = false;
}

void TutorialObject_Update(void)
{
	if (!UI_Tutorial_IsWaiting()) return;

	Ghost* pGhost = GetGhost();
	if (!pGhost) return;

	if (g_pEnban && !g_EnbanTouched)
	{
		XMFLOAT3 gPos = pGhost->GetPos();
		XMFLOAT3 ePos = g_pEnban->GetPos();
		float dx = gPos.x - ePos.x;
		float dz = gPos.z - ePos.z;

		if (sqrtf(dx * dx + dz * dz) <= 0.8f)
		{
			g_EnbanTouched = true;
		}
	}

	if (!g_PianoPossessed)
	{
		if (pGhost->GetState() == GS_TRANSFORM)
		{
			int inRangeNum = pGhost->GetInRangeNum();
			Furniture* pFurniture = GetFurniture(inRangeNum);
			if (pFurniture && pFurniture->GetBlockID() == 62) // 62 is Piano
			{
				g_PianoPossessed = true;
			}
		}
	}
}

void TutorialObject_Draw(void)
{
	if (!g_pEnban) return;
	if (Field_GetCurrentFloor() != 2) return;

	g_pEnban->Draw();
}

void TutorialObject_Finalize(void)
{
	delete g_pEnban;
	g_pEnban = nullptr;
	g_EnbanTouched = false;
	g_PianoPossessed = false;
}

bool* TutorialObject_GetEnbanTouchedPtr(void)
{
	return &g_EnbanTouched;
}

bool* TutorialObject_GetPianoPossessedPtr(void)
{
	return &g_PianoPossessed;
}
