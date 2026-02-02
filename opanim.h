#pragma once
#include <d3d11.h>

void OpAnim_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void OpAnim_Finalize(void);
void OpAnim_Update();
void OpAnimDraw(void);

static void UpdateInazuma(float elapsedSeconds);
static void UpdateBasuta(float elapsedSeconds);
static void UpdateYurei(float elapsedSeconds);
static void UpdateRain(float elapsedSeconds);
