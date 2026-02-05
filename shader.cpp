/*==============================================================================

   シェーダー [shader.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "shader.h"


static ID3D11VertexShader* g_pVertexShader = nullptr;//頂点シェーダー
static ID3D11InputLayout* g_pInputLayout = nullptr;//頂点レイアウト

// フィールド専用インスタンス描画用
static ID3D11VertexShader* g_pInstanceVertexShader = nullptr;
static ID3D11InputLayout* g_pInstanceInputLayout = nullptr;

ID3D11Buffer* g_pVSConstantBuffer = nullptr;//定数バッファ1個
static ID3D11PixelShader* g_pPixelShader = nullptr;//ピクセルシェーダー

static ID3D11Buffer* g_pLightConstantBuffer = nullptr;//定数バッファ1個
static ID3D11Buffer* g_pWorldConstantBuffer = nullptr;//定数バッファ1個
static ID3D11Buffer* g_pMaterialColorBuffer = nullptr;//マテリアル色バッファ
static ID3D11Buffer* g_pCameraPositionBuffer = nullptr;//カメラ位置バッファ

static LightData g_CurrentLightData;

// 現在設定されているシェーダーの種類を記録する
enum SHADER_TYPE {
	SHADER_TYPE_NONE,
	SHADER_TYPE_DEFAULT,
	SHADER_TYPE_INSTANCE
};
static SHADER_TYPE g_CurrentShaderType = SHADER_TYPE_NONE;

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 定数バッファの更新用ヘルパー関数
template<typename T>
static void UpdateConstantBuffer(ID3D11Buffer* buffer, const T& data)
{
	if (!buffer || !g_pContext) return;
	D3D11_MAPPED_SUBRESOURCE msr;
	if (SUCCEEDED(g_pContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) {
		memcpy(msr.pData, &data, sizeof(T));
		g_pContext->Unmap(buffer, 0);
	}
}

static void UpdateLightBuffer()
{
    UpdateConstantBuffer(g_pLightConstantBuffer, g_CurrentLightData);
}

bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr;

	if (!pDevice || !pContext) {
		hal::dout << "Shader_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return false;
	}

	g_pDevice = pDevice;
	g_pContext = pContext;

	std::ifstream ifs_vs("shader_vertex_2d.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, L"頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_2d.cso", L"エラー", MB_OK);
		return false;
	}

	ifs_vs.seekg(0, std::ios::end);
	std::streamsize filesize = ifs_vs.tellg();
	ifs_vs.seekg(0, std::ios::beg);

	unsigned char* vsbinary_pointer = new unsigned char[filesize];

	ifs_vs.read((char*)vsbinary_pointer, filesize);
	ifs_vs.close();

	hr = g_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		MessageBox(nullptr, L"頂点シェーダーの作成に失敗しました", L"エラー", MB_OK);
		delete[] vsbinary_pointer;
		return false;
	}


	// 頂点レイアウトの定義<<<<<<<NORMAL追加
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// 頂点レイアウトの作成
	hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsbinary_pointer, filesize, &g_pInputLayout);
	delete[] vsbinary_pointer;

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
		MessageBox(nullptr, L"頂点レイアウトの作成に失敗しました", L"エラー", MB_OK);
		return false;
	}

	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;            // 頻繁な更新に適した設定
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPUからの書き込みを許可

	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pWorldConstantBuffer);
	
	buffer_desc.ByteWidth = sizeof(LightData);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pLightConstantBuffer);

	// 定数バッファを初期状態（ライト無効）で初期化
	g_CurrentLightData = {};
	g_CurrentLightData.lightCount = 0;
	g_CurrentLightData.ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
	UpdateLightBuffer();

	// マテリアル色バッファの作成
	buffer_desc.ByteWidth = sizeof(XMFLOAT4);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pMaterialColorBuffer);

	// カメラ位置バッファの作成
	buffer_desc.ByteWidth = sizeof(XMFLOAT4);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pCameraPositionBuffer);

	// 事前コンパイル済みピクセルシェーダーの読み込み
	std::ifstream ifs_ps("shader_pixel_2d.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, L"ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_2d.cso", L"エラー", MB_OK);
		return false;
	}

	ifs_ps.seekg(0, std::ios::end);
	std::streamsize ps_filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[ps_filesize];
	ifs_ps.read((char*)psbinary_pointer, ps_filesize);
	ifs_ps.close();

	// ピクセルシェーダーの作成
	hr = g_pDevice->CreatePixelShader(psbinary_pointer, ps_filesize, nullptr, &g_pPixelShader);
	delete[] psbinary_pointer;

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
		MessageBox(nullptr, L"ピクセルシェーダーの作成に失敗しました", L"エラー", MB_OK);
		return false;
	}

	// --- フィールド専用インスタンスシェーダーの読み込み（任意） ---
	std::ifstream ifs_ivs("shader_field_instance.cso", std::ios::binary);
	if (ifs_ivs) {
		ifs_ivs.seekg(0, std::ios::end);
		std::streamsize ivs_filesize = ifs_ivs.tellg();
		ifs_ivs.seekg(0, std::ios::beg);
		unsigned char* ivs_pointer = new unsigned char[ivs_filesize];
		ifs_ivs.read((char*)ivs_pointer, ivs_filesize);
		ifs_ivs.close();

		hr = g_pDevice->CreateVertexShader(ivs_pointer, ivs_filesize, nullptr, &g_pInstanceVertexShader);

		if (SUCCEEDED(hr)) {
			// インスタンス描画用のレイアウト定義
			D3D11_INPUT_ELEMENT_DESC instLayout[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				// インスタンスデータ (スロット1)
				{ "INSTANCEWORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
				{ "INSTANCEWORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
				{ "INSTANCEWORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
				{ "INSTANCEWORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			};
			g_pDevice->CreateInputLayout(instLayout, ARRAYSIZE(instLayout), ivs_pointer, ivs_filesize, &g_pInstanceInputLayout);
		}
		delete[] ivs_pointer;
	}

	return true;
}

void Shader_Finalize()
{
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pVSConstantBuffer);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);

	SAFE_RELEASE(g_pInstanceVertexShader);
	SAFE_RELEASE(g_pInstanceInputLayout);

	SAFE_RELEASE(g_pWorldConstantBuffer);
	SAFE_RELEASE(g_pLightConstantBuffer);
	SAFE_RELEASE(g_pMaterialColorBuffer);

	SAFE_RELEASE(g_pCameraPositionBuffer);
}

void Shader_SetMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファに行列をセット
	UpdateConstantBuffer(g_pVSConstantBuffer, transpose);
}

void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファに行列をセット
	UpdateConstantBuffer(g_pWorldConstantBuffer, transpose);
}

void Shader_ClearPointLights()
{
	for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
	{
		g_CurrentLightData.pointLights[i] = {};
	}
	g_CurrentLightData.lightCount = 0;
	UpdateLightBuffer();
}

void Shader_AddPointLight(PointLight* light)
{
	if (!light)
	{
		return;
	}

	if (g_CurrentLightData.lightCount >= MAX_POINT_LIGHTS)
	{
		hal::dout << "Shader_AddPointLight() : MAX_POINT_LIGHTS(" << MAX_POINT_LIGHTS << ") を超えています" << std::endl;
		return;
	}

	PointLightData& dst = g_CurrentLightData.pointLights[g_CurrentLightData.lightCount];
	dst = {};
	dst.enable = light->enable;
	dst.position = light->position;
	dst.direction = light->direction;
	dst.diffuse = light->diffuse;
	dst.params = XMFLOAT4(light->range, light->intensity, 0.0f, 0.0f);

	++g_CurrentLightData.lightCount;
	UpdateLightBuffer();
}

void Shader_SetPointLight(PointLight* light)
{
	Shader_ClearPointLights();
	Shader_AddPointLight(light);
}

void Shader_SetAmbientLight(AmbientLight* ambient)
{
	if (ambient) {
		g_CurrentLightData.ambient = ambient->color;
	}
	else {
		g_CurrentLightData.ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
	}
	UpdateLightBuffer();
}

void Shader_SetMaterialColor(const DirectX::XMFLOAT4& color)
{
	// 定数バッファにマテリアル色をセット
	UpdateConstantBuffer(g_pMaterialColorBuffer, color);
}

void Shader_SetCameraPos(const DirectX::XMFLOAT3& pos)
{
	// 定数バッファにカメラ位置をセット（XMFLOAT4に変換）
	XMFLOAT4 cameraPos = { pos.x, pos.y, pos.z, 1.0f };
	UpdateConstantBuffer(g_pCameraPositionBuffer, cameraPos);
}

void Shader_Begin()
{
	if (g_CurrentShaderType == SHADER_TYPE_DEFAULT) return;
	g_CurrentShaderType = SHADER_TYPE_DEFAULT;

	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

	// 頂点レイアウトを描画パイプラインに設定
	g_pContext->IASetInputLayout(g_pInputLayout);

	// 定数バッファを描画パイプラインに設定
	g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer);
	g_pContext->VSSetConstantBuffers(1, 1, &g_pWorldConstantBuffer);  // ワールド行列をセット
	g_pContext->VSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);  // ライト情報を頂点シェーダーに設定

	// ピクセルシェーダーにもライト定数バッファとマテリアル色バッファを設定
	g_pContext->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);
	g_pContext->PSSetConstantBuffers(3, 1, &g_pMaterialColorBuffer);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pCameraPositionBuffer);
}

// フィールド専用インスタンス描画開始
void Shader_BeginInstance()
{
	if (g_CurrentShaderType == SHADER_TYPE_INSTANCE) return;
	g_CurrentShaderType = SHADER_TYPE_INSTANCE;

	if (!g_pInstanceVertexShader) { Shader_Begin(); return; }

	g_pContext->VSSetShader(g_pInstanceVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);
	g_pContext->IASetInputLayout(g_pInstanceInputLayout);

	g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer);
	g_pContext->VSSetConstantBuffers(1, 1, &g_pWorldConstantBuffer);
	g_pContext->VSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);

	g_pContext->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);
	g_pContext->PSSetConstantBuffers(3, 1, &g_pMaterialColorBuffer);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pCameraPositionBuffer);
}

// 毎フレームのリセット用（Clearなどで呼ぶ想定）
void Shader_RefreshState()
{
	g_CurrentShaderType = SHADER_TYPE_NONE;
}
