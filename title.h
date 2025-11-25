#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

void Title_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Title_Update(void);
void Title_Draw(void);
void Title_Finalize(void);