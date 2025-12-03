//sprite.cpp
#include "sprite.h"
#include "shader.h"
#include "main.h"
#include "texture.h"
#include <cmath>

//グローバル変数
static constexpr int NUM_VERTEX = 6; // 使用できる最大頂点数
static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

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
//単一スプライト描画（汎用的になるように外に出す）
//----------------------------
void Sprite_Single_Draw(XMFLOAT2 pos, XMFLOAT2 size,float rot, XMFLOAT4 color, BLENDSTATE bstate, ID3D11ShaderResourceView* texture, FLIPTYPE2D flipType)
{
	// シェーダー開始
	Shader_Begin();

	// スクリーン座標用の射影行列を設定
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	// 2D描画用にマテリアル色を白に設定（テクスチャ　＊　白　＝　テクスチャ）
	Shader_SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// テクスチャ設定
	ID3D11ShaderResourceView* tex = texture;
	g_pContext->PSSetShaderResources(0, 1, &tex);
	SetBlendState(bstate);

	// 頂点データ
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex* v = (Vertex*)msr.pData;

	float halfX = size.x * 0.5f;
	float halfY = size.y * 0.5f;

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
		v[i].position = { rx + pos.x, ry + pos.y, 0.0f };
		v[i].normal = { 0.0f, 0.0f, 0.0f };
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
	// シェーダー開始
	Shader_Begin();

	// スクリーン空間用の直交投影を設定
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	// 2D描画用にマテリアル色を白に設定（テクスチャ × 白 = テクスチャ）
	Shader_SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// テクスチャ設定
	ID3D11ShaderResourceView* tex = texture;
	g_pContext->PSSetShaderResources(0, 1, &tex);
	SetBlendState(bstate);

	// 頂点データ
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex* v = (Vertex*)msr.pData;

	float halfX = size.x * 0.5f;
	float halfY = size.y * 0.5f;

	// 回転（度->ラジアン）
	float rotDeg = rot;
	float rad = XMConvertToRadians(rotDeg);
	float co = cosf(rad);
	float si = sinf(rad);

	// ローカル頂点（中心原点）
	float lx[4] = { -halfX, halfX, -halfX, halfX };
	float ly[4] = { -halfY, -halfY, halfY, halfY };

	// 回転・平行移動後の頂点座標計算
	for (int i = 0; i < 4; ++i) {
		float rx = lx[i] * co - ly[i] * si;
		float ry = lx[i] * si + ly[i] * co;
		v[i].position = { rx + pos.x, ry + pos.y, 0.0f };
		v[i].normal = { 0.0f, 0.0f, 0.0f };
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
//シンプルな四角形描画 (以前のユーティリティ、必要なら回転対応を追加して再有効化)
//----------------------------
//void Sprite_Draw(XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 color, BLENDSTATE bstate, ID3D11ShaderResourceView* g_Texture)
//{
//	// シェーダーを描画パイプラインに設定
//	Shader_Begin();
//
//	// 頂点シェーダーに変換行列を設定
//	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
//
//
//	g_pDevice = Direct3D_GetDevice();
//	g_pContext = Direct3D_GetDeviceContext();
//
//	//張るテクスチャの指定
//	Direct3D_GetDeviceContext()->PSSetShaderResources(0, 1, &g_Texture);
//	SetBlendState(bstate); //ブレンドを設定
//
//	//頂点バッファのロック
//	D3D11_MAPPED_SUBRESOURCE msr;
//	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
//
//	//頂点バッファへの仮想ポインタを取得
//	Vertex* v = (Vertex*)msr.pData;
//
//	v[0].position = { pos.x - (size.x / 2),pos.y - (size.y / 2), 0.0f };
//	v[0].normal = { 0.0f, 0.0f, 0.0f };
//	v[0].color = color;
//	v[0].texCoord = { 0.0f,0.0f };
//
//	v[1].position = { pos.x + (size.x / 2),pos.y - (size.y / 2), 0.0f };
//	v[1].normal = { 0.0f, 0.0f, 0.0f };
//	v[1].color = color;
//	v[1].texCoord = { 1.0f,0.0f };
//
//	v[2].position = { pos.x - (size.x / 2),pos.y + (size.y / 2), 0.0f };
//	v[2].normal = { 0.0f, 0.0f, 0.0f };
//	v[2].color = color;
//	v[2].texCoord = { 0.0f,1.0f };
//
//	v[3].position = { pos.x + (size.x / 2),pos.y + (size.y / 2), 0.0f };
//	v[3].normal = { 0.0f, 0.0f, 0.0f };
//	v[3].color = color;
//	v[3].texCoord = { 1.0f,1.0f };
//
//	//頂点バッファのロック解除
//	g_pContext->Unmap(g_pVertexBuffer, 0);
//
//	//指定の位置に指定のサイズ、色の四角形を描画する
//	UINT stride = sizeof(Vertex);
//	UINT offset = 0;
//	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
//
//	//ポリゴン描画方式の指定
//	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
//
//	// ポリゴン描画命令発行
//	g_pContext->Draw(4, 0);
//}