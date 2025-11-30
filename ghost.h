#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Ghost_Update(void);
void Ghost_Draw(void);
void Ghost_Finalize(void);