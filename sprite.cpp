//sprite.cpp
#include "sprite.h"
#include "shader.h"
#include "shader_2d.h"
#include "texture.h"
#include "light.h"
#include "define.h"
#include <cmath>
using namespace DirectX;

//グローバル変数
static constexpr int NUM_VERTEX = 6; // 使用できる最大頂点数
static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 2D描画最適化用フラグ
static bool g_b2DBegun = false;
static AmbientLight g_SpriteAmbientLight(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
//----------------------------
//スプライト初期化
//----------------------------
void Sprite_Initialize()
{
	g_pDevice = Direct3D_GetDevice();

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;//<<<<<<<格納する最大頂点数
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);
}

//----------------------------
//スプライト終了
//----------------------------
void Sprite_Finalize()
{
	if (g_pVertexBuffer) {
		g_pVertexBuffer->Release();
		g_pVertexBuffer = nullptr;
	}
}

//----------------------------
// 2D描画開始（シェーダーセットアップを一度だけ実行）
//----------------------------
void Sprite_BeginDraw2D()
{
	if (g_b2DBegun) return;

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	static bool s_Shader2DInitialized = false;
	if (!s_Shader2DInitialized)
	{
		Shader2D_Initialize(g_pDevice, g_pContext);
		s_Shader2DInitialized = true;
	}

	Shader2D_BeginDefault();
	Shader2D_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, DRAW_SCREEN_WIDTH, DRAW_SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	// 2D描画用のライト設定（ライトなし・アンビエント白）を先に設定
	Shader_SetAmbientLight(&g_SpriteAmbientLight);
	Shader_SetPointLight(nullptr);
	Shader_FlushLights(); // ライトバッファをGPUに転送してからShader_Beginを呼ぶ

	Shader_Begin();
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, DRAW_SCREEN_WIDTH, DRAW_SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
	Shader_SetWorldMatrix(XMMatrixIdentity());
	Shader_SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	g_b2DBegun = true;
}

//----------------------------
// 2D描画終了（フラグをリセット）
//----------------------------
void Sprite_EndDraw2D()
{
	g_b2DBegun = false;
}

//----------------------------
//単一スプライト描画（汎用的になるように外に出す）
//----------------------------
void Sprite_Single_Draw(XMFLOAT2 pos, XMFLOAT2 size, float rot, XMFLOAT4 color, BLENDSTATE bstate, ID3D11ShaderResourceView* texture, FLIPTYPE2D flipType)
{
	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// Sprite_BeginDraw2D()が呼ばれていない場合は個別にセットアップ（互換性維持）
	if (!g_b2DBegun) {
		Shader_SetAmbientLight(&g_SpriteAmbientLight);
		Shader_SetPointLight(nullptr); // ライトを無効化
		Shader_FlushLights(); // ライトバッファをGPUに転送
		Shader_Begin();
		Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, DRAW_SCREEN_WIDTH, DRAW_SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
		Shader_SetWorldMatrix(XMMatrixIdentity());
		Shader_SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}

	// テクスチャ設定
	ID3D11ShaderResourceView* tex = texture;
	g_pContext->PSSetShaderResources(0, 1, &tex);
	SetBlendState(bstate);

	// 頂点データ
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex* v = (Vertex*)msr.pData;

	// HD座標→描画解像度へスケーリング
	float drawPosX = pos.x * DRAW_SCALE_X;
	float drawPosY = pos.y * DRAW_SCALE_Y;
	float halfX = size.x * DRAW_SCALE_X * 0.5f;
	float halfY = size.y * DRAW_SCALE_X * 0.5f;

	// 回転（度->ラジアン）
	float rotDeg = rot;
	float rad = XMConvertToRadians(rotDeg);
	float co = cosf(rad);
	float si = sinf(rad);

	// ローカル頂点（中心原点）
	float lx[4] = { -halfX, halfX, -halfX, halfX };
	float ly[4] = { -halfY, -halfY, halfY, halfY };

	// 回転と並進移動の頂点座標を計算
	for (int i = 0; i < 4; ++i) {
		float rx = lx[i] * co - ly[i] * si;
		float ry = lx[i] * si + ly[i] * co;
		v[i].position = { rx + drawPosX, ry + drawPosY, 0.0f };
		v[i].normal = { 0.0f, 0.0f, -1.0f }; // 法線を設定（NaN回避とライティング用）
		v[i].color = color;
	}

	// テクスチャ座標（フリップに対応）
	// flipTypeに応じてテクスチャ座標を反転
	float texCoordU[2] = { 0.0f, 1.0f };
	float texCoordV[2] = { 0.0f, 1.0f };

	// 左右反転（FLIPTYPE2D_HORIZONTAL）
	if (static_cast<unsigned char>(flipType) & static_cast<unsigned char>(FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL))
	{
		texCoordU[0] = 1.0f;
		texCoordU[1] = 0.0f;
	}

	// 上下反転（FLIPTYPE2D_VERTICAL）
	if (static_cast<unsigned char>(flipType) & static_cast<unsigned char>(FLIPTYPE2D::FLIPTYPE2D_VERTICAL))
	{
		texCoordV[0] = 1.0f;
		texCoordV[1] = 0.0f;
	}

	// テクスチャ座標を設定
	v[0].texCoord = { texCoordU[0], texCoordV[0] };
	v[1].texCoord = { texCoordU[1], texCoordV[0] };
	v[2].texCoord = { texCoordU[0], texCoordV[1] };
	v[3].texCoord = { texCoordU[1], texCoordV[1] };

	g_pContext->Unmap(g_pVertexBuffer, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	g_pContext->Draw(4, 0);
}

//----------------------------
//分割テクスチャ描画（テクスチャを分割して指定したパターンのみ描画）
//----------------------------
void Sprite_Split_Draw(XMFLOAT2 pos, XMFLOAT2 size, float rot, XMFLOAT4 color, BLENDSTATE bstate, ID3D11ShaderResourceView* texture, int divideX, int divideY, int textureNumber)
{
	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// Sprite_BeginDraw2D()が呼ばれていない場合は個別にセットアップ（互換性維持）
	if (!g_b2DBegun) {
		Shader_SetAmbientLight(&g_SpriteAmbientLight);
		Shader_SetPointLight(nullptr); // ライトを無効化
		Shader_FlushLights(); // ライトバッファをGPUに転送
		Shader_Begin();
		Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, DRAW_SCREEN_WIDTH, DRAW_SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
		Shader_SetWorldMatrix(XMMatrixIdentity());
		Shader_SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}

	// テクスチャ設定
	ID3D11ShaderResourceView* tex = texture;
	g_pContext->PSSetShaderResources(0, 1, &tex);
	SetBlendState(bstate);

	// 頂点データ
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex* v = (Vertex*)msr.pData;

	// HD座標→描画解像度へスケーリング
	float drawPosX = pos.x * DRAW_SCALE_X;
	float drawPosY = pos.y * DRAW_SCALE_Y;
	float halfX = size.x * DRAW_SCALE_X * 0.5f;
	float halfY = size.y * DRAW_SCALE_X * 0.5f;

	float rad = XMConvertToRadians(rot);
	float co = cosf(rad);
	float si = sinf(rad);

	float lx[4] = { -halfX, halfX, -halfX, halfX };
	float ly[4] = { -halfY, -halfY, halfY, halfY };

	for (int i = 0; i < 4; ++i) {
		float rx = lx[i] * co - ly[i] * si;
		float ry = lx[i] * si + ly[i] * co;
		v[i].position = { rx + drawPosX, ry + drawPosY, 0.0f };
		v[i].normal = { 0.0f, 0.0f, -1.0f }; // 法線を設定（NaN回避とライティング用）
		v[i].color = color;
	}

	// 分割されたテクスチャの対応する部分のUV座標を計算
	float texWidth = 1.0f / divideX;		// 1つのテクスチャの横幅
	float texHeight = 1.0f / divideY;		// 1つのテクスチャの縦幅

	// textureNumberから行・列を計算
	int col = textureNumber % divideX;
	int row = textureNumber / divideX;

	// テクスチャ座標の最小・最大値を計算
	float texMinU = col * texWidth;
	float texMaxU = (col + 1) * texWidth;
	float texMinV = row * texHeight;
	float texMaxV = (row + 1) * texHeight;

	// テクスチャ座標を設定
	v[0].texCoord = { texMinU, texMinV };
	v[1].texCoord = { texMaxU, texMinV };
	v[2].texCoord = { texMinU, texMaxV };
	v[3].texCoord = { texMaxU, texMaxV };

	g_pContext->Unmap(g_pVertexBuffer, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	g_pContext->Draw(4, 0);
}

//----------------------------
//単一穴あきスプライト描画
//----------------------------
void Sprite_Single_DrawHole(XMFLOAT2 pos, XMFLOAT2 size, float rot, XMFLOAT4 color, BLENDSTATE bstate,
	ID3D11ShaderResourceView* texture, XMFLOAT2 holeCenterPx, float holeRadiusPx, float holeSoftnessPx,
	FLIPTYPE2D flipType)
{
	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	static bool s_Shader2DInitialized = false;
	if (!s_Shader2DInitialized)
	{
		Shader2D_Initialize(g_pDevice, g_pContext);
		s_Shader2DInitialized = true;
	}

	Shader2D_SetUseHolePS(true);

	Shader2D_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, DRAW_SCREEN_WIDTH, DRAW_SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	// 穴のパラメータも描画解像度にスケーリング
	Shader2D_HoleParams hp{};
	hp.centerPx   = { holeCenterPx.x * DRAW_SCALE_X, holeCenterPx.y * DRAW_SCALE_Y };
	hp.radiusPx   = holeRadiusPx   * DRAW_SCALE_X;
	hp.softnessPx = holeSoftnessPx * DRAW_SCALE_X;
	Shader2D_SetHoleParams(hp);

	ID3D11ShaderResourceView* tex = texture;
	g_pContext->PSSetShaderResources(0, 1, &tex);
	SetBlendState(bstate);

	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex* v = (Vertex*)msr.pData;

	// HD座標→描画解像度へスケーリング
	float drawPosX = pos.x * DRAW_SCALE_X;
	float drawPosY = pos.y * DRAW_SCALE_Y;
	float halfX = size.x * DRAW_SCALE_X * 0.5f;
	float halfY = size.y * DRAW_SCALE_X * 0.5f;

	float rad = XMConvertToRadians(rot);
	float co = cosf(rad);
	float si = sinf(rad);

	float lx[4] = { -halfX, halfX, -halfX, halfX };
	float ly[4] = { -halfY, -halfY, halfY, halfY };

	for (int i = 0; i < 4; ++i) {
		float rx = lx[i] * co - ly[i] * si;
		float ry = lx[i] * si + ly[i] * co;
		v[i].position = { rx + drawPosX, ry + drawPosY, 0.0f };
		v[i].normal = { 0.0f, 0.0f, -1.0f };
		v[i].color = color;
	}

	float texCoordU[2] = { 0.0f, 1.0f };
	float texCoordV[2] = { 0.0f, 1.0f };

	if (static_cast<unsigned char>(flipType) & static_cast<unsigned char>(FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL))
	{
		texCoordU[0] = 1.0f;
		texCoordU[1] = 0.0f;
	}
	if (static_cast<unsigned char>(flipType) & static_cast<unsigned char>(FLIPTYPE2D::FLIPTYPE2D_VERTICAL))
	{
		texCoordV[0] = 1.0f;
		texCoordV[1] = 0.0f;
	}

	v[0].texCoord = { texCoordU[0], texCoordV[0] };
	v[1].texCoord = { texCoordU[1], texCoordV[0] };
	v[2].texCoord = { texCoordU[0], texCoordV[1] };
	v[3].texCoord = { texCoordU[1], texCoordV[1] };

	g_pContext->Unmap(g_pVertexBuffer, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	g_pContext->Draw(4, 0);

	// 重要：穴あきPSを無効化して戻す（他Spriteを消さない）
	Shader2D_SetUseHolePS(false);
}
