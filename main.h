#pragma once
//==================================
//マクロ定義
//==================================
#include <SDKDDKVer.h> //利用できる最も上位のWindowsプラットフォームが定義される
#include <windows.h>
#include <algorithm>
#include "direct3d.h"
#include "scene.h"

#define CLASS_NAME L"DX21 Window"
#define WINDOW_CAPTION L"ポリゴン描画"
#define SCREEN_WIDTH (1280.0f)
#define SCREEN_HEIGHT (720.0f)
#define WIN32_LEAN_AND_MEAN	//32bitアプリには不要な情報を無視
#define FPS (60)

using namespace DirectX;

//==================================
//デバッグ用設定
//==================================
#define STOP_TIMER_BUSTER (false) //trueならタイマーとバスターズのupdateを停止させる
#define DIRECT_START (true) //trueならgameシーンから直接開始する
#define DEBUG_DRAW (false) //trueならdebugdraw機能を有効にする

//==================================
//プロトタイプ宣言
//==================================
//ウィンドウプロシージャ
//コールバック関数は他人が呼び出すもの
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// FPS設定関数
void SetFPS(int fps);
