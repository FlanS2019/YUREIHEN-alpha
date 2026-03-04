#pragma once

#include <d3d11.h>

void Vote_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Vote_Update(void);
void Vote_Draw(void);
void Vote_Finalize(void);
