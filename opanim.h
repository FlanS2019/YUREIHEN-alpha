#pragma once
#include <d3d11.h>

void OpAnim_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void OpAnim_Finalize(void);
void OpAnim_Update();
void OpAnimDraw(void);

void UpdateInazuma(float elapsedSeconds);
void UpdateBasuta(float elapsedSeconds);
void UpdateYurei(float elapsedSeconds);
