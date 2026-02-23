/*==============================================================================

   Direct3Dの初期化関連 [direct3d.cpp]
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include "direct3d.h"
#include "debug_ostream.h"
#include "shader.h"
#include "define.h"

#pragma comment(lib, "d3d11.lib")//DirectXのプログラムを追加する
// #pragma comment(lib, "dxgi.lib")

/* 各種インターフェース */
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;

/* バックバッファ関連 */
static ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
static ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;
static ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
static D3D11_TEXTURE2D_DESC g_BackBufferDesc{};
static D3D11_VIEWPORT g_Viewport{};////////////////追加

// ウィンドウのクライアントサイズ（描画先となる実ピクセル数）
static float g_ClientWidth  = DRAW_SCREEN_WIDTH;
static float g_ClientHeight = DRAW_SCREEN_HEIGHT;

static bool configureBackBuffer(); // バックバッファの設定・生成
static void releaseBackBuffer(); // バックバッファの解放


static float	bFactor[4] = { 0.0f,0.0f,0.0f,0.0f };
static ID3D11BlendState* bState[BLENDSTATE_MAX];
static ID3D11DepthStencilState* g_DepthStateEnable;
static ID3D11DepthStencilState* g_DepthStateDisable;
static ID3D11DepthStencilState* g_DepthStateReadOnly;




bool Direct3D_Initialize(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.Windowed = TRUE;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.BufferDesc.Width  = (UINT)DRAW_SCREEN_WIDTH;   // バックバッファを描画解像度に固定
    swap_chain_desc.BufferDesc.Height = (UINT)DRAW_SCREEN_HEIGHT;  // ウィンドウサイズに引きずられない
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;//0にしてみる
    swap_chain_desc.OutputWindow = hWnd;

	/*
	IDXGIFactory1* pFactory;
	CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	IDXGIAdapter1* pAdapter;
	pFactory->EnumAdapters1(1, &pAdapter); // セカンダリアダプタを取得
	pFactory->Release();
	DXGI_ADAPTER_DESC1 desc;
	pAdapter->GetDesc1(&desc); // アダプタの情報を取得して確認したい場合
	pAdapter->Release(); // D3D11CreateDeviceAndSwapChain()の第１引数に渡して利用し終わったら解放する
	*/

	UINT device_flags = 0;

#if defined(DEBUG) || defined(_DEBUG)
    //device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
 
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        device_flags,
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &g_pSwapChain,
        &g_pDevice,
        &feature_level,
        &g_pDeviceContext);

    if (FAILED(hr)) {
		MessageBox(hWnd, L"Direct3Dの初期化に失敗しました", L"エラー", MB_OK);
        return false;
    }

	if (!configureBackBuffer()) {
		MessageBox(hWnd, L"バックバッファの設定に失敗しました", L"エラー", MB_OK);
		return false;
	}

	// サンプラーステート設定
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;//異方性とというフィルターによる
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;  //テクスチャの縁で折り返さない
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;  //テクスチャの縁で折り返さない
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;//未使用
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	ID3D11SamplerState* samplerState = NULL;
	g_pDevice->CreateSamplerState(&samplerDesc, &samplerState);
	//サンプラーをシェーダーに設定
	g_pDeviceContext->PSSetSamplers(0, 1, &samplerState);
	


	// ブレンドステート設定
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//ブレンド無効
	blendDesc.RenderTarget[0].BlendEnable = FALSE;	//ブレンド無効
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_NONE]);

	//αブレンド
	blendDesc.RenderTarget[0].BlendEnable = TRUE;	//ブレンド有効
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_ALFA]);//<<ALPHA！

	//加算合成
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_ADD]);

	//減算合成
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_SUBTRACT;//<<<<表示色 = 背景 - ポリゴン
//	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;//<<<<表示色 = 背景 - ポリゴン
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_SUB]);

	SetBlendState(BLENDSTATE_ALFA);//デフォルト設定


	// 深度ステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	g_pDevice->CreateDepthStencilState(&depthStencilDesc, &g_DepthStateEnable);//深度有効ステート

	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 書き込み禁止
	g_pDevice->CreateDepthStencilState(&depthStencilDesc, &g_DepthStateReadOnly);

	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	g_pDevice->CreateDepthStencilState(&depthStencilDesc, &g_DepthStateDisable);//深度無効ステート

	g_pDeviceContext->OMSetDepthStencilState(g_DepthStateDisable, NULL); //デフォルト　深度無効


    return true;
}

void	SetDepthTest(bool flg)
{
	if (flg == true)
	{
		g_pDeviceContext->OMSetDepthStencilState(g_DepthStateEnable, NULL);
		Direct3D_SetViewport3D(); // 3D描画：アスペクト比を保ったビューポート
	}
	else
	{
		g_pDeviceContext->OMSetDepthStencilState(g_DepthStateDisable, NULL);
		Direct3D_SetViewport2D(); // 2D描画：バックバッファ全体
	}
}

void SetDepthWrite(bool flg)
{
	if (flg)
	{
		g_pDeviceContext->OMSetDepthStencilState(g_DepthStateEnable, NULL);
	}
	else
	{
		g_pDeviceContext->OMSetDepthStencilState(g_DepthStateReadOnly, NULL);
	}
}

void Direct3D_Finalize()
{
	releaseBackBuffer();

	if (g_pSwapChain) {
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}

	if (g_pDeviceContext) {
		g_pDeviceContext->Release();
		g_pDeviceContext = nullptr;
	}
	
    if (g_pDevice) {
		g_pDevice->Release();
		g_pDevice = nullptr;
	}

	SAFE_RELEASE(g_DepthStateReadOnly);
	SAFE_RELEASE(g_DepthStateEnable);
	SAFE_RELEASE(g_DepthStateDisable);
}

void Direct3D_Clear()
{
	float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pRenderTargetView, clear_color);
	g_pDeviceContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// レンダーターゲットビューとデプスステンシルビューの設定/////////////追加
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// シェーダーの状態をリセット
	Shader_RefreshState();
}

void Direct3D_Present()
{
	// スワップチェーンの表示
	g_pSwapChain->Present(1, 0);
}

//////////////////////////////////////////////追加

ID3D11Device* Direct3D_GetDevice()
{
	return g_pDevice;
}

ID3D11DeviceContext* Direct3D_GetDeviceContext()
{
	return g_pDeviceContext;
}

unsigned int Direct3D_GetBackBufferWidth()
{
	return g_BackBufferDesc.Width;
}

unsigned int Direct3D_GetBackBufferHeight()
{
	return g_BackBufferDesc.Height;
}

// ウィンドウのクライアントサイズが変わったときに呼ぶ
void Direct3D_ResizeWindow(unsigned int clientW, unsigned int clientH)
{
	g_ClientWidth  = (clientW  > 0) ? static_cast<float>(clientW)  : 1.0f;
	g_ClientHeight = (clientH > 0) ? static_cast<float>(clientH) : 1.0f;
}

// 3D描画用：DRAW_SCREEN の縦横比を保ったレターボックス/ピラーボックスビューポートを設定
void Direct3D_SetViewport3D()
{
	const float targetAspect = DRAW_SCREEN_WIDTH / DRAW_SCREEN_HEIGHT;
	const float windowAspect = g_ClientWidth / g_ClientHeight;

	float vpW, vpH, vpX, vpY;

	if (windowAspect > targetAspect)
	{
		// ウィンドウが横長 → 縦に合わせて横をトリミング（ピラーボックス）
		// ウィンドウが横長なので縦が見切れずに収まり、横が余る…
		// 指示：「ウィンドウが横長→縦を見切れ」= 縦方向にはみ出させる
		// つまり横を画面幅いっぱいに使い、縦がはみ出す（縦の上下が見えない）
		vpW = g_ClientWidth;
		vpH = g_ClientWidth / targetAspect;
		vpX = 0.0f;
		vpY = (g_ClientHeight - vpH) * 0.5f;
	}
	else
	{
		// ウィンドウが縦長 → 横を見切れにする
		// 横方向にはみ出させる（横の左右が見えない）
		vpH = g_ClientHeight;
		vpW = g_ClientHeight * targetAspect;
		vpX = (g_ClientWidth - vpW) * 0.5f;
		vpY = 0.0f;
	}

	// バックバッファ座標系に変換（バックバッファは DRAW_SCREEN_WIDTH x DRAW_SCREEN_HEIGHT）
	float scaleX = static_cast<float>(g_BackBufferDesc.Width)  / g_ClientWidth;
	float scaleY = static_cast<float>(g_BackBufferDesc.Height) / g_ClientHeight;

	D3D11_VIEWPORT vp;
	vp.TopLeftX = vpX * scaleX;
	vp.TopLeftY = vpY * scaleY;
	vp.Width    = vpW * scaleX;
	vp.Height   = vpH * scaleY;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	g_pDeviceContext->RSSetViewports(1, &vp);
}

// 2D描画用：バックバッファ内に「16:9の中央領域」だけのビューポートを設定
// ウィンドウが横長（例: 16:10）なら上下に黒帯、縦長なら左右に黒帯
void Direct3D_SetViewport2D()
{
	const float targetAspect = DRAW_SCREEN_WIDTH / DRAW_SCREEN_HEIGHT;
	const float windowAspect = g_ClientWidth / g_ClientHeight;

	float vpW, vpH, vpX, vpY;

	if (windowAspect > targetAspect)
	{
		// ウィンドウが横長 → 縦に合わせ、左右に黒帯（ピラーボックス）
		vpH = g_ClientHeight;
		vpW = g_ClientHeight * targetAspect;
		vpX = (g_ClientWidth - vpW) * 0.5f;
		vpY = 0.0f;
	}
	else
	{
		// ウィンドウが縦長 → 横に合わせ、上下に黒帯（レターボックス）
		vpW = g_ClientWidth;
		vpH = g_ClientWidth / targetAspect;
		vpX = 0.0f;
		vpY = (g_ClientHeight - vpH) * 0.5f;
	}

	// バックバッファ座標系に変換
	float scaleX = static_cast<float>(g_BackBufferDesc.Width)  / g_ClientWidth;
	float scaleY = static_cast<float>(g_BackBufferDesc.Height) / g_ClientHeight;

	D3D11_VIEWPORT vp;
	vp.TopLeftX = vpX * scaleX;
	vp.TopLeftY = vpY * scaleY;
	vp.Width    = vpW * scaleX;
	vp.Height   = vpH * scaleY;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	g_pDeviceContext->RSSetViewports(1, &vp);
}

////////////////////////////////////////////////////////




bool configureBackBuffer()
{
    HRESULT hr;

    ID3D11Texture2D* back_buffer_pointer = nullptr;

	// バックバッファの取得
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer_pointer);

    if (FAILED(hr)) {
		hal::dout << "バックバッファの取得に失敗しました" << std::endl;
        return false;
    }

	// バックバッファのレンダーターゲットビューの生成
	hr = g_pDevice->CreateRenderTargetView(back_buffer_pointer, nullptr, &g_pRenderTargetView);

    if (FAILED(hr)) {
        back_buffer_pointer->Release();
        hal::dout << "バックバッファのレンダーターゲットビューの生成に失敗しました" << std::endl;
        return false;
    }

	// バックバッファの状態（情報）を取得
    back_buffer_pointer->GetDesc(&g_BackBufferDesc);

	back_buffer_pointer->Release(); // バックバッファのポインタは不要なので解放

	// デプスステンシルバッファの生成
	D3D11_TEXTURE2D_DESC depth_stencil_desc{};
	depth_stencil_desc.Width = g_BackBufferDesc.Width;
	depth_stencil_desc.Height = g_BackBufferDesc.Height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_stencil_desc.SampleDesc.Count = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;
	hr = g_pDevice->CreateTexture2D(&depth_stencil_desc, nullptr, &g_pDepthStencilBuffer);

	if (FAILED(hr)) {
		hal::dout << "デプスステンシルバッファの生成に失敗しました" << std::endl;
		return false;
	}

	// デプスステンシルビューの生成
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
	depth_stencil_view_desc.Format = depth_stencil_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;
	depth_stencil_view_desc.Flags = 0;
	hr = g_pDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &depth_stencil_view_desc, &g_pDepthStencilView);

	if (FAILED(hr)) {
		hal::dout << "デプスステンシルビューの生成に失敗しました" << std::endl;
		return false;
	}


	// ビューポートの設定/////////////////////追加
	g_Viewport.TopLeftX = 0.0f;
	g_Viewport.TopLeftY = 0.0f;
	g_Viewport.Width = static_cast<FLOAT>(g_BackBufferDesc.Width);
	g_Viewport.Height = static_cast<FLOAT>(g_BackBufferDesc.Height);
	g_Viewport.MinDepth = 0.0f;
	g_Viewport.MaxDepth = 1.0f;
	g_pDeviceContext->RSSetViewports(1, &g_Viewport); // ビューポートの設定
	////////////////////////////////////////////追加


    return true;
}

void releaseBackBuffer()
{
	if (g_pRenderTargetView) {
		g_pRenderTargetView->Release();
		g_pRenderTargetView = nullptr;
	}

	if (g_pDepthStencilBuffer) {
		g_pDepthStencilBuffer->Release();
		g_pDepthStencilBuffer = nullptr;
	}

	if (g_pDepthStencilView) {
		g_pDepthStencilView->Release();
		g_pDepthStencilView = nullptr;
	}
}

//以下の関数を一番下へ追加
void SetBlendState(BLENDSTATE blend)
{

	g_pDeviceContext->OMSetBlendState(bState[blend], bFactor, 0xffffffff);

}

void Direct3D_Resize(UINT width, UINT height)
{
	// 新しい幅と高さを設定
	g_BackBufferDesc.Width = width;
	g_BackBufferDesc.Height = height;

	// バックバッファとデプスステンシルバッファを再設定
	releaseBackBuffer();
	configureBackBuffer();

	// ビューポートの設定
	g_Viewport.TopLeftX = 0.0f;
	g_Viewport.TopLeftY = 0.0f;
	g_Viewport.Width = static_cast<FLOAT>(width);
	g_Viewport.Height = static_cast<FLOAT>(height);
	g_Viewport.MinDepth = 0.0f;
	g_Viewport.MaxDepth = 1.0f;
	g_pDeviceContext->RSSetViewports(1, &g_Viewport);
}
