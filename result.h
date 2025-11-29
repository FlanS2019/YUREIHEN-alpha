#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

void Result_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Result_Update(void);
void Result_Draw(void);
void Result_Finalize(void);