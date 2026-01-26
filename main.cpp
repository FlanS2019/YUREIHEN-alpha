//==================================================
//main.cpp
//制作者：鶴岡遼大
//制作日：2025/05/09
//==================================================

#include <SDKDDKVer.h> //利用できる最も上位のWindowsプラットフォームが定義される
#define WIN32_LEAN_AND_MEAN	//32bitアプリには不要な情報を無視
#include <windows.h>
#include <algorithm>
#include <chrono>
#include "scene.h"
#include "direct3d.h"
#include "shader.h"
#include "debug_ostream.h"
#include "main.h"
#include "keyboard.h"
#include "mouse.h"
#include "sprite.h"
#include "fade.h"
#include "sound.h"
#include "ghost.h"
#include <iostream>

//==================================
//グローバル変数
//==================================

//#ifndef _DEBUG
int g_CountFPS;
long long g_UpdateTime = 0;
long long g_DrawTime = 0;
wchar_t g_DebugStr[2048];
//#endif
static int g_TargetFPS = FPS;  // 目標FPS（デフォルトは FPS マクロの値）

#pragma comment(lib, "winmm.lib")

//==================================
//SetFPS関数
//==================================
void SetFPS(int fps)
{
	if (fps > 0)
	{
		g_TargetFPS = fps;
	}
}

//==================================
//メイン関数
//==================================
//==================================  
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	//フレームレート計測用変数
	DWORD dwExecLastTime;
	DWORD dwFPSLastTime;
	DWORD dwTitleUpdateTime;
	DWORD dwCurrentTime;
	DWORD dwFrameCount;

	HRESULT dummy = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

	//ウィンドウクラスの登録
	WNDCLASS wc;//構造体を定義
	ZeroMemory(&wc, sizeof(WNDCLASS));//構造体初期化
	wc.lpfnWndProc = WndProc;//初期化
	wc.lpszClassName = CLASS_NAME;//仕様書の名前
	wc.hInstance = hInstance;//このアプリのこと
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);//cursorの種類
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);//背景色
	RegisterClass(&wc);//構構造体をwindowsにセット

	//ウィンドウサイズの調整
	//クライアント領域（描画領域）のサイズを表す矩形
	RECT window_rect = { 0,0,(LONG)SCREEN_WIDTH,(LONG)SCREEN_HEIGHT };
	//ウィンドウスタイルの設定
	DWORD window_style = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME;
	//指定のクライアント領域＋ウィンドウスタイルでの全体のサイズを計算
	AdjustWindowRect(&window_rect, window_style, FALSE);
	//矩形の横と縦のサイズを計算
	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	//ウィンドウの作成
	HWND hWnd = CreateWindow(
		CLASS_NAME,		//作りたいウィンドウ
		WINDOW_CAPTION,	//ウィンドウに表示するタイトル
		window_style,//標準的なサイズのウィンドウ　サイズ変更禁止
		CW_USEDEFAULT,	//以下default
		CW_USEDEFAULT,
		window_width,//ウィンドウの幅
		window_height,//ウィンドウの高さ
		NULL,
		NULL,
		hInstance,	//アプリのハンドル
		NULL
	);

	ShowWindow(hWnd, nCmdShow);//引数に従って表示非表示

	//ウィンドウ内部の更新要求
	UpdateWindow(hWnd);
	Direct3D_Initialize(hWnd);

	// 全画面・ウィンドウ切替用のキー入力を受け取るために Direct3D_Initialize の後に行う
	// (スワップチェーンへのアクセスが必要な場合があるため)

	Keyboard_Initialize();
	Mouse_Initialize(hWnd);
	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
	Sprite_Initialize();
	Fade_Initialize();
	InitSound();
	Init();

	//メッセージループ
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	//フレームレート計測初期化
	timeBeginPeriod(1);	//タイマーの制度を設定　
	dwExecLastTime = dwFPSLastTime = dwTitleUpdateTime = timeGetTime();
	dwCurrentTime = dwFrameCount = 0;

	do
	{
		//終了メッセージが来るまでループ （Windowsからのメッセージはそのまま使えない）
		//while (GetMessage(&msg, NULL, 0, 0))　ゲ－ム向きではないらしい
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))	//余計なことをしないので早い
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg); //WndProcが呼び出される
		}
		//ゲームの処理
		else
		{
			dwCurrentTime = timeGetTime();

			if ((dwCurrentTime - dwFPSLastTime) >= 1000)
			{
				//#ifndef _DEBUG
				g_CountFPS = dwFrameCount;
				//#endif
				dwFPSLastTime = dwCurrentTime;
				dwFrameCount = 0;
			}

			if ((dwCurrentTime - dwExecLastTime) >= ((float)1000 / g_TargetFPS))
			{
				dwExecLastTime = dwCurrentTime;

				// Alt+Enterで全画面切り替え
				if (Keyboard_IsKeyDown(KK_LEFTALT) || Keyboard_IsKeyDown(KK_RIGHTALT))
				{
					if (Keyboard_IsKeyDownTrigger(KK_ENTER))
					{
						// 全画面切り替え処理
						static bool isFullScreen = false;
						isFullScreen = !isFullScreen;
						IDXGISwapChain* pSwapChain = nullptr;
						// direct3d.cpp からスワップチェーンを取得する手段がないため、
						// direct3d.h に extern か取得関数を追加する必要があるが、
						// ここでは一旦 Direct3D_Initialize 時の状態に任せるか、
						// DXGI の機能を利用する。
						// 既存のコードでは swap_chain_desc.Windowed = TRUE; で初期化されている。
					}
				}

				// 更新時間の計測
				auto startUpdate = std::chrono::high_resolution_clock::now();
				Fade_Update();
				Update();
				auto endUpdate = std::chrono::high_resolution_clock::now();
				g_UpdateTime = std::chrono::duration_cast<std::chrono::microseconds>(endUpdate - startUpdate).count();

				// 描画時間の計測
				auto startDraw = std::chrono::high_resolution_clock::now();
				Direct3D_Clear();//バッファのクリア

				Draw();
				Fade_Draw();

				Direct3D_Present();//バッファの表示
				auto endDraw = std::chrono::high_resolution_clock::now();
				g_DrawTime = std::chrono::duration_cast<std::chrono::microseconds>(endDraw - startDraw).count();

				long long totalTime = g_UpdateTime + g_DrawTime;

				// 16666usを超えた場合にGhostの位置を詳細にデバッグ出力
				if (totalTime > 16666)
				{
					if (Ghost* pGhost = GetGhost())
					{
						DirectX::XMFLOAT3 pos = pGhost->GetPos();
						hal::dout << "drop! Total: " << totalTime 
								  << "us (Upd: " << g_UpdateTime << "us, Drw: " << g_DrawTime 
								  << "us) | Ghost Pos: X=" << pos.x << ", Y=" << pos.y << ", Z=" << pos.z << std::endl;
					}
				}

				//#ifndef _DEBUG
				//ウィンドウキャプションへ情報を表示（0.2秒に1回更新）
				if ((dwCurrentTime - dwTitleUpdateTime) >= 200)
				{
					dwTitleUpdateTime = dwCurrentTime;
					swprintf(g_DebugStr, sizeof(g_DebugStr) / sizeof(wchar_t), L"FPS: %d | Total: %lldus | Upd: %lldus | Drw: %lldus", g_CountFPS, g_UpdateTime + g_DrawTime, g_UpdateTime, g_DrawTime);
					SetWindowText(hWnd, g_DebugStr);
				}
				//#endif

				keycopy();

				dwFrameCount++;
			}
		}

	} while (msg.message != WM_QUIT);//windowsから終了メッセージが来たらループ終了

	Finalize();
	UninitSound();
	Fade_Finalize();
	Sprite_Finalize();
	Shader_Finalize();
	Direct3D_Finalize();


	//終了
	return (int)msg.wParam;
}

//==================================
//ウィンドウプロシージャ
//メッセージループ内で呼び出し
//==================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//HGDIOBJ hbrWhite, hbrGray;

	//HDC hdc;			//デバイスコンテキスト
	//PAINTSTRUCT ps;		//ウィンドウ画面の大きさなど描画関連の情報

	Mouse_ProcessMessage(uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_ACTIVATEAPP:

		break;
	case WM_SIZE:
		// ウィンドウサイズが変更された（全画面化など）場合にバックバッファを再構成する
		if (wParam != SIZE_MINIMIZED)
		{
			// Direct3D 側のバックバッファサイズ更新処理を呼び出す必要がある
			// 現状の Direct3D_Initialize 等では対応していないため、
			// 必要に応じて ResizeBuffer などを実装する。
		}
		break;
	case WM_SYSKEYDOWN:
		// Alt+Enterキーの全画面切り替え無効化（手動制御に変更）
		if (wParam == VK_RETURN && (lParam & 0x20000000))
		{
			// Alt+Enterの全画面切り替えを無視（イベントを処理してreturnする）
			return 0;
		}
		Keyboard_ProcessMessage(uMsg, wParam, lParam);
		break;
	case WM_KEYUP:
		Keyboard_ProcessMessage(uMsg, wParam, lParam);
		break;
	case WM_SYSKEYUP:
		Keyboard_ProcessMessage(uMsg, wParam, lParam);
		break;
		//case WM_PAINT:	//ウィンドウ表示の命令
		//	hdc = BeginPaint(hWnd, &ps);//描画に関する情報を受け取る
		//	EndPaint(hWnd, &ps);	//表示完了　hdcを開放する
		//	return 0;
		//	break;
	case WM_KEYDOWN:	//キーが押された
		//if (wParam == VK_ESCAPE)
		//{
		//	//ウィンドウを閉じたいリクエストをWindowsに送る
		//	SendMessage(hWnd, WM_CLOSE, 0, 0);
		//}
		Keyboard_ProcessMessage(uMsg, wParam, lParam);

		break;
	case WM_CLOSE:
		hal::dout << "終了確認\n" << std::endl;

		//if (MessageBox(hWnd, "本当に終了してよろしいですか", "確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK)
		if(true)
		{
			//OKが押された
			DestroyWindow(hWnd);
		}
		else
		{
			//終わらない
			return 0;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	//必要のないメッセージは適当に処理するらしい
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
