#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "sprite.h"
#include "font.h"
using namespace DirectX;

// ==========================================
// チュートリアル画面
// ==========================================

void UI_Tutorial_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void UI_Tutorial_Finalize(void);
void UI_Tutorial_Update(void);
void UI_Tutorial_Draw(void);

bool UI_Tutorial_IsActive(void);

