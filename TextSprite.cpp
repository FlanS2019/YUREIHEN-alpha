// TextSprite.cpp - stb_truetype.hを使用したテキスト描画スプライト実装

#include "TextSprite.h"
#include "shader.h"
#include "main.h"
#include <cmath>
#include <cstdio>

// extern "C"でC関数として処理
extern "C" {
	#define STB_TRUETYPE_IMPLEMENTATION
	#include "stb_truetype.h"
}

// グローバル変数
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11Buffer* g_pVertexBuffer = nullptr;
static std::unordered_map<std::string, FontData*> g_FontCache;
static constexpr int NUM_TEXT_VERTEX = 1024;

//----------------------------
// TextSprite初期化
//----------------------------
void TextSprite_Initialize(void)
{
	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(TextVertex) * NUM_TEXT_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);
}

//----------------------------
// TextSprite終了
//----------------------------
void TextSprite_Finalize(void)
{
	for (auto& pair : g_FontCache) {
		FontData* fontData = pair.second;
		if (fontData) {
			if (fontData->bitmap) delete[] fontData->bitmap;
			if (fontData->cdata) delete[] fontData->cdata;
			if (fontData->texture) fontData->texture->Release();
			delete fontData;
		}
	}
	g_FontCache.clear();

	if (g_pVertexBuffer) {
		g_pVertexBuffer->Release();
		g_pVertexBuffer = nullptr;
	}
}

//----------------------------
// フォント読み込み
//----------------------------
FontData* TextSprite_LoadFont(const char* fontPath, float fontSize, int bitmapSize)
{
	std::string cacheKey = std::string(fontPath) + "_" + std::to_string((int)fontSize);
	auto it = g_FontCache.find(cacheKey);
	if (it != g_FontCache.end()) {
		return it->second;
	}

	FontData* fontData = new FontData();
	fontData->fontSize = fontSize;
	fontData->bitmapWidth = bitmapSize;
	fontData->bitmapHeight = bitmapSize;
	fontData->texture = nullptr;
	fontData->charRangeStart = 32;
	fontData->charRangeEnd = 127;

	FILE* f;
	fopen_s(&f, fontPath, "rb");
	if (!f) {
		delete fontData;
		return nullptr;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	unsigned char* fontBuffer = new unsigned char[size];
	fread(fontBuffer, 1, size, f);
	fclose(f);

	fontData->bitmap = new unsigned char[bitmapSize * bitmapSize];
	fontData->cdata = new stbtt_bakedchar[96];

	stbtt_BakeFontBitmap(
		fontBuffer, 0,
		fontSize,
		fontData->bitmap, bitmapSize, bitmapSize,
		32, 96, fontData->cdata
	);

	// ビットマップをDirect3D テクスチャに変換
	// グレースケール画像をRGBA形式に変換
	unsigned char* rgbaBuffer = new unsigned char[bitmapSize * bitmapSize * 4];
	for (int i = 0; i < bitmapSize * bitmapSize; ++i) {
		rgbaBuffer[i * 4 + 0] = fontData->bitmap[i];  // R
		rgbaBuffer[i * 4 + 1] = fontData->bitmap[i];  // G
		rgbaBuffer[i * 4 + 2] = fontData->bitmap[i];  // B
		rgbaBuffer[i * 4 + 3] = fontData->bitmap[i];  // A
	}

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = bitmapSize;
	texDesc.Height = bitmapSize;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = rgbaBuffer;
	initData.SysMemPitch = bitmapSize * 4;

	ID3D11Texture2D* texture2D = nullptr;
	if (FAILED(g_pDevice->CreateTexture2D(&texDesc, &initData, &texture2D))) {
		delete[] fontBuffer;
		delete[] fontData->bitmap;
		delete[] fontData->cdata;
		delete[] rgbaBuffer;
		delete fontData;
		return nullptr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	if (FAILED(g_pDevice->CreateShaderResourceView(texture2D, &srvDesc, &fontData->texture))) {
		texture2D->Release();
		delete[] fontBuffer;
		delete[] fontData->bitmap;
		delete[] fontData->cdata;
		delete[] rgbaBuffer;
		delete fontData;
		return nullptr;
	}

	texture2D->Release();
	delete[] fontBuffer;
	delete[] rgbaBuffer;

	g_FontCache[cacheKey] = fontData;

	return fontData;
}

//----------------------------
// テキスト描画
//----------------------------
void TextSprite_RenderText(XMFLOAT2 pos, XMFLOAT2 scale, float rotation, XMFLOAT4 color,
							const std::wstring& text, FontData* fontData, BLENDSTATE bstate)
{
	if (!fontData || !fontData->texture || text.empty()) {
		return;
	}

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	Shader_Begin();
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
	Shader_SetMaterialColor(color);

	ID3D11ShaderResourceView* tex = fontData->texture;
	g_pContext->PSSetShaderResources(0, 1, &tex);
	SetBlendState(bstate);

	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	TextVertex* vertices = (TextVertex*)msr.pData;

	float rad = XMConvertToRadians(rotation);
	float co = cosf(rad);
	float si = sinf(rad);

	int vertexCount = 0;
	float currentX = 0.0f;

	for (size_t i = 0; i < text.length() && vertexCount < NUM_TEXT_VERTEX - 4; ++i) {
		wchar_t wc = text[i];
		// 文字範囲をチェック
		if (wc < fontData->charRangeStart || wc > fontData->charRangeEnd) {
			continue;
		}
		unsigned char c = static_cast<unsigned char>(wc);

		stbtt_bakedchar* b = fontData->cdata + (c - fontData->charRangeStart);

		float xPos = currentX + b->xoff;
		float yPos = b->yoff;
		float xPos2 = xPos + (b->x1 - b->x0);
		float yPos2 = yPos + (b->y1 - b->y0);

		float u0 = (float)b->x0 / fontData->bitmapWidth;
		float v0 = (float)b->y0 / fontData->bitmapHeight;
		float u1 = (float)b->x1 / fontData->bitmapWidth;
		float v1 = (float)b->y1 / fontData->bitmapHeight;

		float scaledX1 = xPos * scale.x;
		float scaledY1 = yPos * scale.y;
		float scaledX2 = xPos2 * scale.x;
		float scaledY2 = yPos2 * scale.y;

		float rx = scaledX1 * co - scaledY1 * si;
		float ry = scaledX1 * si + scaledY1 * co;
		vertices[vertexCount].position = { rx + pos.x, ry + pos.y, 0.0f };
		vertices[vertexCount].normal = { 0.0f, 0.0f, 0.0f };
		vertices[vertexCount].color = color;
		vertices[vertexCount].texCoord = { u0, v0 };
		vertexCount++;

		rx = scaledX2 * co - scaledY1 * si;
		ry = scaledX2 * si + scaledY1 * co;
		vertices[vertexCount].position = { rx + pos.x, ry + pos.y, 0.0f };
		vertices[vertexCount].normal = { 0.0f, 0.0f, 0.0f };
		vertices[vertexCount].color = color;
		vertices[vertexCount].texCoord = { u1, v0 };
		vertexCount++;

		rx = scaledX1 * co - scaledY2 * si;
		ry = scaledX1 * si + scaledY2 * co;
		vertices[vertexCount].position = { rx + pos.x, ry + pos.y, 0.0f };
		vertices[vertexCount].normal = { 0.0f, 0.0f, 0.0f };
		vertices[vertexCount].color = color;
		vertices[vertexCount].texCoord = { u0, v1 };
		vertexCount++;

		rx = scaledX2 * co - scaledY2 * si;
		ry = scaledX2 * si + scaledY2 * co;
		vertices[vertexCount].position = { rx + pos.x, ry + pos.y, 0.0f };
		vertices[vertexCount].normal = { 0.0f, 0.0f, 0.0f };
		vertices[vertexCount].color = color;
		vertices[vertexCount].texCoord = { u1, v1 };
		vertexCount++;

		currentX += b->xadvance;
	}

	g_pContext->Unmap(g_pVertexBuffer, 0);

	if (vertexCount > 0) {
		UINT stride = sizeof(TextVertex);
		UINT offset = 0;
		g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
		g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		g_pContext->Draw(vertexCount, 0);
	}
}

//----------------------------
// TextSprite::GetTextWidth
//----------------------------
float TextSprite::GetTextWidth(void) const
{
	if (!m_FontData || m_Text.empty()) {
		return 0.0f;
	}

	float width = 0.0f;
	for (wchar_t wc : m_Text) {
		unsigned char c = static_cast<unsigned char>(wc);

		if (c >= m_FontData->charRangeStart && c <= m_FontData->charRangeEnd) {
			stbtt_bakedchar* b = m_FontData->cdata + (c - m_FontData->charRangeStart);
			width += b->xadvance;
		}
	}

	return width * m_Scale.x;
}
