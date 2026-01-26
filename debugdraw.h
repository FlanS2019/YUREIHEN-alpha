#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "anim_sprite3d.h"

using namespace DirectX;

void DebugDraw_Initialize(void);
void DebugDraw_Update(void);
void DebugDraw_Draw(void);
void DebugDraw_Finalize(void);

// スポットライト制御用関数
void DebugDraw_SetSpotLightColor(float r, float g, float b);
void DebugDraw_SetSpotLightHeight(float height);
void DebugDraw_EnableSpotLight(bool enable);
Light* DebugDraw_GetSpotLight(void);
