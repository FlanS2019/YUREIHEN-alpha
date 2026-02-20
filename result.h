#pragma once

#include <d3d11.h>
#include "sprite.h"

void Result_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Result_Update(void);
void Result_Draw(void);
void Result_Finalize(void);

// タイマー結果をセット
void Result_SetTimerValue(float time);

// 階層をセット
void Result_SetFloor(int floor);

// 連鎖数をセット
void Result_SetCombo(int combo);