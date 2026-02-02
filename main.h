#pragma once
//==================================
//マクロ定義
//==================================
#include <SDKDDKVer.h> //利用できる最も上位のWindowsプラットフォームが定義される
#include <windows.h>

//==================================
//プロトタイプ宣言
//==================================
//ウィンドウプロシージャ
//コールバック関数は他人が呼び出すもの
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// FPS設定関数
void SetFPS(int fps);
