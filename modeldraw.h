#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"

using namespace DirectX;

void ModelDraw_Initialize(void);
void ModelDraw_Update(void);
void ModelDraw_Draw(void);
void ModelDraw_Finalize(void);