#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

// ==========================================
// リトライメニュー（負けた時の確認画面）
// ==========================================

void UI_RetryMenu_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void UI_RetryMenu_Finalize(void);
void UI_RetryMenu_Update(void);
void UI_RetryMenu_Draw(void);

// リトライメニューを表示する
void UI_RetryMenu_Show(void);

// リトライメニューが表示中か
bool UI_RetryMenu_IsActive(void);
