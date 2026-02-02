/*==============================================================================

   日本語フォont描画システム実装 [font.cpp]
										  Author : Copilot
										  Date   : 2025/01/10
--------------------------------------------------------------------------------

==============================================================================*/
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "font.h"
#include "direct3d.h"
#include "shader.h"
#include "light.h"
#include "define.h"
#include <cstdlib>
#include <cstring>
#include <d3d11.h>
#include <d3dcompiler.h>
using namespace DirectX;

#pragma comment(lib, "d3dcompiler.lib")

// ==========================================
// グローバルフォントデータ
// ==========================================
static unsigned char* g_pGlobalFontData = nullptr;
static int g_GlobalFontDataSize = 0;

void Font_InitializeGlobalData()
{
	if (g_pGlobalFontData != nullptr) {
		return; // 既に初期化済み
	}

	// フォントファイル読み込み
	FILE* f = nullptr;
	fopen_s(&f, "asset/font/KaiseiDecol-Medium.ttf", "rb");
	if (!f) {
		OutputDebugStringA("Font_InitializeGlobalData: Failed to open font file\n");
		return;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	g_pGlobalFontData = (unsigned char*)malloc(size);
	if (!g_pGlobalFontData) {
		fclose(f);
		OutputDebugStringA("Font_InitializeGlobalData: Failed to allocate memory\n");
		return;
	}

	fread(g_pGlobalFontData, 1, size, f);
	fclose(f);
	g_GlobalFontDataSize = size;

	OutputDebugStringA("Font_InitializeGlobalData: Font data loaded successfully\n");
}

void Font_FinalizeGlobalData()
{
	if (g_pGlobalFontData) {
		free(g_pGlobalFontData);
		g_pGlobalFontData = nullptr;
		g_GlobalFontDataSize = 0;
	}
}

// ==========================================
// FontRenderer クラス実装
// ==========================================

FontRenderer::FontRenderer(XMFLOAT2 pos, float fontSize, float rotation,
	XMFLOAT4 color, const std::string& text)
	: Transform2D(pos, rotation, { 1.0f, 1.0f }), m_Color(color), m_Text(text),
	m_FontSize(fontSize),
	m_pTexture(nullptr), m_pSRV(nullptr),
	m_pVertexBuffer(nullptr),
	m_VertexCount(0), m_AtlasWidth(0), m_AtlasHeight(0),
	m_AtlasNextX(0), m_AtlasNextY(0), m_AtlasRowHeight(0),
	m_pAtlasData(nullptr), m_pFontInfo(nullptr),
	m_FontAscender(0), m_FontDescender(0)
{
	// シェーダー作成
	if (!CreateShaders()) {
		return;
	}

	// アトラステクスチャ生成
	if (!BakeAtlas()) {
		return;
	}

	OutputDebugStringA("FontRenderer: Initialized successfully\n");
}

FontRenderer::~FontRenderer() {
	if (m_pVertexBuffer) m_pVertexBuffer->Release();
	if (m_pSRV) m_pSRV->Release();
	if (m_pTexture) m_pTexture->Release();
	if (m_pAtlasData) free(m_pAtlasData);
	if (m_pFontInfo) free(m_pFontInfo);
	// グローバルフォントデータの削除はしない（複数のFontRendererで共有）
}

bool FontRenderer::CreateShaders() {
	// 既存のシェーダーシステムを使用するため、このメソッドは最小化
	return true;
}

bool FontRenderer::BakeAtlas() {
	ID3D11Device* pDevice = Direct3D_GetDevice();

	if (!pDevice) {
		OutputDebugStringA("FontRenderer::BakeAtlas: pDevice is nullptr\n");
		return false;
	}

	// グローバルフォントデータを確認
	if (g_pGlobalFontData == nullptr || g_GlobalFontDataSize == 0) {
		OutputDebugStringA("FontRenderer::BakeAtlas: Global font data not initialized\n");
		return false;
	}

	// stbtt フォント初期化
	m_pFontInfo = (struct stbtt_fontinfo*)malloc(sizeof(struct stbtt_fontinfo));
	if (!m_pFontInfo) {
		return false;
	}

	if (!stbtt_InitFont(m_pFontInfo, g_pGlobalFontData, 0)) {
		free(m_pFontInfo);
		m_pFontInfo = nullptr;
		return false;
	}

	// フォントメトリクスを取得
	stbtt_GetFontVMetrics(m_pFontInfo, &m_FontAscender, &m_FontDescender, nullptr);

	OutputDebugStringA("FontRenderer::BakeAtlas: Font kerning information not found - use FontRendererFT2 for kerning support\n");

	// アトラステクスチャ作成（動的グリフキャッシング用に大きめ）
	m_AtlasWidth = FONT_ATLAS_WIDTH;
	m_AtlasHeight = FONT_ATLAS_HEIGHT;
	m_pAtlasData = (unsigned char*)calloc(m_AtlasWidth * m_AtlasHeight, 1);

	if (!m_pAtlasData) {
		free(m_pFontInfo);
		m_pFontInfo = nullptr;
		return false;
	}

	m_AtlasNextX = 0;
	m_AtlasNextY = 0;
	m_AtlasRowHeight = 0;

	// DirectX11テクスチャ作成
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = m_AtlasWidth;
	desc.Height = m_AtlasHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// アトラスデータを RGBA に変換
	unsigned char* atlasRGBA = (unsigned char*)malloc(m_AtlasWidth * m_AtlasHeight * 4);
	if (!atlasRGBA) {
		free(m_pAtlasData);
		free(m_pFontInfo);
		m_pAtlasData = nullptr;
		m_pFontInfo = nullptr;
		return false;
	}

	for (int i = 0; i < m_AtlasWidth * m_AtlasHeight; i++) {
		atlasRGBA[i * 4 + 0] = 255;           // R
		atlasRGBA[i * 4 + 1] = 255;           // G
		atlasRGBA[i * 4 + 2] = 255;           // B
		atlasRGBA[i * 4 + 3] = 0;             // A
	}

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = atlasRGBA;
	initData.SysMemPitch = m_AtlasWidth * 4;

	HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pTexture);

	if (FAILED(hr)) {
		free(atlasRGBA);
		free(m_pAtlasData);
		free(m_pFontInfo);
		m_pAtlasData = nullptr;
		m_pFontInfo = nullptr;
		return false;
	}

	// シェーダーリソースビュー作成
	hr = pDevice->CreateShaderResourceView(m_pTexture, nullptr, &m_pSRV);
	if (FAILED(hr)) {
		m_pTexture->Release();
		m_pTexture = nullptr;
		free(atlasRGBA);
		free(m_pAtlasData);
		free(m_pFontInfo);
		m_pAtlasData = nullptr;
		m_pFontInfo = nullptr;
		return false;
	}

	// 頂点バッファ作成（ダイナミックバッファ）
	struct Vertex {
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT4 color;
		XMFLOAT2 texCoord;
	};

	Vertex vertices[] = {
		{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
	};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.ByteWidth = sizeof(vertices);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices;

	hr = pDevice->CreateBuffer(&vbDesc, &vbData, &m_pVertexBuffer);

	if (FAILED(hr)) {
		m_pSRV->Release();
		m_pTexture->Release();
		free(atlasRGBA);
		free(m_pAtlasData);
		free(m_pFontInfo);
		m_pAtlasData = nullptr;
		m_pFontInfo = nullptr;
		return false;
	}

	m_VertexCount = 4;

	OutputDebugStringA("FontRenderer::BakeAtlas: Success\n");
	free(atlasRGBA);

	return true;
}

// UTF-8 文字列をUnicode コードポイントにデコード
int FontRenderer::UTF8ToCodePoint(const std::string& text, size_t& index) {
	unsigned char c = (unsigned char)text[index];

	// 単一バイト文字（ASCII）
	if ((c & 0x80) == 0) {
		index++;
		return (int)c;
	}

	// 2バイト文字
	if ((c & 0xE0) == 0xC0) {
		int codepoint = ((c & 0x1F) << 6) | ((unsigned char)text[index + 1] & 0x3F);
		index += 2;
		return codepoint;
	}

	// 3バイト文字
	if ((c & 0xF0) == 0xE0) {
		int codepoint = ((c & 0x0F) << 12) | (((unsigned char)text[index + 1] & 0x3F) << 6) | ((unsigned char)text[index + 2] & 0x3F);
		index += 3;
		return codepoint;
	}

	// 4バイト文字
	if ((c & 0xF8) == 0xF0) {
		int codepoint = ((c & 0x07) << 18) | (((unsigned char)text[index + 1] & 0x3F) << 12) | (((unsigned char)text[index + 2] & 0x3F) << 6) | ((unsigned char)text[index + 3] & 0x3F);
		index += 4;
		return codepoint;
	}

	index++;
	return 0;
}

// アトラスにグリフを追加
bool FontRenderer::AddGlyphToAtlas(int glyphIndex) {
	if (!m_pFontInfo) {
		return false;
	}

	// キャッシュに既に存在する場合
	if (m_CharCache.find(glyphIndex) != m_CharCache.end()) {
		return true;
	}

	// キャッシュ満杯時は LRU グリフを削除
	if ((int)m_CharCache.size() >= FONT_MAX_CACHE_GLYPHS) {
		EvictLRUGlyph();
	}

	// グリフのバウンディングボックスを取得
	int x0, y0, x1, y1;
	stbtt_GetGlyphBitmapBox(m_pFontInfo, glyphIndex, stbtt_ScaleForPixelHeight(m_pFontInfo, m_FontSize), stbtt_ScaleForPixelHeight(m_pFontInfo, m_FontSize), &x0, &y0, &x1, &y1);

	int glyph_width = x1 - x0;
	int glyph_height = y1 - y0;

	// グリフがアトラスに収まらない場合の処理
	if (glyph_width == 0 || glyph_height == 0) {
		// 空グリフ
		CharInfo info = { 0, 0, 0, 0, 0, glyphIndex };
		m_CharCache[glyphIndex] = info;
		m_CacheLRU.push_back(glyphIndex);
		return true;
	}

	// アトラス内のスペースを確保
	if (m_AtlasNextX + glyph_width > m_AtlasWidth) {
		m_AtlasNextX = 0;
		m_AtlasNextY += m_AtlasRowHeight;
		m_AtlasRowHeight = 0;
	}

	if (m_AtlasNextY + glyph_height > m_AtlasHeight) {
		OutputDebugStringA("FontRenderer::AddGlyphToAtlas: Atlas is full\n");
		return false;
	}

	// グリフをビットマップにレンダリング
	float scale = stbtt_ScaleForPixelHeight(m_pFontInfo, m_FontSize);
	unsigned char* glyph_bitmap = (unsigned char*)malloc(glyph_width * glyph_height);
	stbtt_MakeGlyphBitmap(m_pFontInfo, glyph_bitmap, glyph_width, glyph_height, glyph_width, scale, scale, glyphIndex);

	// アトラスデータに書き込み
	for (int y = 0; y < glyph_height; y++) {
		for (int x = 0; x < glyph_width; x++) {
			int atlas_idx = (m_AtlasNextY + y) * m_AtlasWidth + (m_AtlasNextX + x);
			m_pAtlasData[atlas_idx] = glyph_bitmap[y * glyph_width + x];
		}
	}

	// グリフ情報を保存
	CharInfo info;
	info.x0 = (float)m_AtlasNextX;
	info.y0 = (float)m_AtlasNextY;
	info.x1 = (float)(m_AtlasNextX + glyph_width);
	info.y1 = (float)(m_AtlasNextY + glyph_height);

	// アドバンス幅を取得
	int advance_width, left_side_bearing;
	stbtt_GetGlyphHMetrics(m_pFontInfo, glyphIndex, &advance_width, &left_side_bearing);
	info.xadvance = (float)advance_width * scale;
	info.glyphIndex = glyphIndex;

	m_CharCache[glyphIndex] = info;
	m_CacheLRU.push_back(glyphIndex);

	// アトラス位置を更新
	m_AtlasNextX += glyph_width;
	m_AtlasRowHeight = (std::max)(m_AtlasRowHeight, glyph_height);

	free(glyph_bitmap);

	// テクスチャを更新
	UpdateAtlasTexture();

	return true;
}

// LRU グリフを削除
void FontRenderer::EvictLRUGlyph() {
	if (m_CacheLRU.empty()) {
		return;
	}

	int lru_glyph = m_CacheLRU.front();
	m_CacheLRU.pop_front();
	m_CharCache.erase(lru_glyph);
}

// アトラステクスチャを更新
void FontRenderer::UpdateAtlasTexture() {
	ID3D11DeviceContext* pContext = Direct3D_GetDeviceContext();
	if (!pContext || !m_pTexture) {
		return;
	}

	// アトラスデータを RGBA に変換
	unsigned char* atlasRGBA = (unsigned char*)malloc(m_AtlasWidth * m_AtlasHeight * 4);
	for (int i = 0; i < m_AtlasWidth * m_AtlasHeight; i++) {
		atlasRGBA[i * 4 + 0] = 255;           // R
		atlasRGBA[i * 4 + 1] = 255;           // G
		atlasRGBA[i * 4 + 2] = 255;           // B
		atlasRGBA[i * 4 + 3] = m_pAtlasData[i];  // A
	}

	D3D11_MAPPED_SUBRESOURCE msr;
	HRESULT hr = pContext->Map(m_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	if (SUCCEEDED(hr)) {
		memcpy(msr.pData, atlasRGBA, m_AtlasWidth * m_AtlasHeight * 4);
		pContext->Unmap(m_pTexture, 0);
	}

	free(atlasRGBA);
}

void FontRenderer::Draw() {
	ID3D11DeviceContext* pContext = Direct3D_GetDeviceContext();

	if (!m_pVertexBuffer || !m_pSRV || !m_pFontInfo) {
		return;
	}

	// シェーダー開始
	Shader_Begin();

	// スクリーン座標用の射影行列を設定
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	// 2D描画用にワールド行列をリセット
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// マテリアル色を設定
	Shader_SetMaterialColor(m_Color);

	// ライトを無効化
	Shader_SetPointLight(nullptr);

	// テクスチャ設定
	pContext->PSSetShaderResources(0, 1, &m_pSRV);
	SetBlendState(BLENDSTATE_ALFA);

	float scale = stbtt_ScaleForPixelHeight(m_pFontInfo, m_FontSize);

	// テキスト全体の幅を計算（描画時と同じロジックで）
	float text_width = 0.0f;
	size_t temp_i = 0;
	int prev_glyph = -1;
	while (temp_i < m_Text.length()) {
		int codepoint = UTF8ToCodePoint(m_Text, temp_i);

		if (codepoint <= 0) {
			continue;
		}

		int glyph_index = stbtt_FindGlyphIndex(m_pFontInfo, codepoint);
		if (glyph_index < 0) {
			continue;
		}

		// スペース判定
		if (codepoint == 0x0020 || codepoint == 0x3000) {
			int advance_width, left_side_bearing;
			stbtt_GetGlyphHMetrics(m_pFontInfo, glyph_index, &advance_width, &left_side_bearing);
			text_width += (float)advance_width * scale;
			prev_glyph = glyph_index;
			continue;
		}

		// グリフをアトラスに追加
		if (!AddGlyphToAtlas(glyph_index)) {
			continue;
		}

		CharInfo& info = m_CharCache[glyph_index];
		
		// カーニング値を取得
		int kerning = 0;
		if (prev_glyph >= 0) {
			kerning = stbtt_GetGlyphKernAdvance(m_pFontInfo, prev_glyph, glyph_index);
		}
		
		// グリフメトリクスを取得
		int advance_width, left_side_bearing;
		stbtt_GetGlyphHMetrics(m_pFontInfo, glyph_index, &advance_width, &left_side_bearing);

		// グリフのバウンディングボックスを取得してマージンを計算
		int x0, y0, x1, y1;
		stbtt_GetGlyphBitmapBox(m_pFontInfo, glyph_index, scale, scale, &x0, &y0, &x1, &y1);
		float actual_glyph_width = (float)(x1 - x0);
		float margin = actual_glyph_width * FONT_MARGIN_RATIO;
		
		text_width += (float)advance_width * scale + (float)kerning * scale + margin;
		prev_glyph = glyph_index;
	}

	// テキストの開始X座標を中央から左へずらす
	float start_x = m_Position.x - text_width / 2.0f;
	float current_x = start_x;
	float current_y = m_Position.y;

	// フォント全体のメトリクス（スケール適用）
	float font_line_height = (float)(m_FontAscender - m_FontDescender) * scale;
	float ascender_scaled = (float)m_FontAscender * scale;
	float descender_scaled = (float)m_FontDescender * scale;

	size_t i = 0;
	int prev_glyph_draw = -1;
	int char_count = 0;
	while (i < m_Text.length()) {
		int codepoint = UTF8ToCodePoint(m_Text, i);

		if (codepoint <= 0) {
			continue;
		}

		// グリフ ID を取得
		int glyph_index = stbtt_FindGlyphIndex(m_pFontInfo, codepoint);
		if (glyph_index < 0) {
			continue;
		}

		// 半角スペース（U+0020）の場合
		if (codepoint == 0x0020) {
			int advance_width, left_side_bearing;
			stbtt_GetGlyphHMetrics(m_pFontInfo, glyph_index, &advance_width, &left_side_bearing);
			current_x += (float)advance_width * scale;
			prev_glyph_draw = glyph_index;
			continue;
		}

		// 全角スペース（U+3000）の場合
		if (codepoint == 0x3000) {
			int advance_width, left_side_bearing;
			stbtt_GetGlyphHMetrics(m_pFontInfo, glyph_index, &advance_width, &left_side_bearing);
			current_x += (float)advance_width * scale;
			prev_glyph_draw = glyph_index;
			continue;
		}

		// グリフをアトラスに追加
		if (!AddGlyphToAtlas(glyph_index)) {
			continue;
		}

		CharInfo& info = m_CharCache[glyph_index];

		// カーニング値を取得して適用
		int kerning = 0;
		if (prev_glyph_draw >= 0) {
			kerning = stbtt_GetGlyphKernAdvance(m_pFontInfo, prev_glyph_draw, glyph_index);
		}

		// グリフメトリクスを取得
		int advance_width, left_side_bearing;
		stbtt_GetGlyphHMetrics(m_pFontInfo, glyph_index, &advance_width, &left_side_bearing);

		// グリフのバウンディングボックスを取得
		int x0, y0, x1, y1;
		stbtt_GetGlyphBitmapBox(m_pFontInfo, glyph_index, scale, scale, &x0, &y0, &x1, &y1);

		// 実際のグリフサイズ（ピクセル単位）
		float actual_glyph_width = (float)(x1 - x0);
		float actual_glyph_height = (float)(y1 - y0);

		// グリフ幅に基づいたマージンを計算
		float margin = actual_glyph_width * FONT_MARGIN_RATIO;

		// テクスチャ座標（0.0～1.0範囲に正規化）
		float u0 = info.x0 / (float)m_AtlasWidth;
		float v0 = info.y0 / (float)m_AtlasHeight;
		float u1 = info.x1 / (float)m_AtlasWidth;
		float v1 = info.y1 / (float)m_AtlasHeight;

		// Y座標計算
		float y_offset = current_y + y0 + m_FontSize * FONT_OFFSET_Y;

		// 描画サイズ
		float char_pixel_width = actual_glyph_width;
		float char_pixel_height = actual_glyph_height;

		// 4つの頂点を設定
		D3D11_MAPPED_SUBRESOURCE msr;
		pContext->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

		struct Vertex {
			XMFLOAT3 position;
			XMFLOAT3 normal;
			XMFLOAT4 color;
			XMFLOAT2 texCoord;
		};

		Vertex* v = (Vertex*)msr.pData;

		v[0].position = { current_x, y_offset, 0.0f };
		v[0].texCoord = { u0, v0 };
		v[0].normal = { 0.0f, 0.0f, 0.0f };
		v[0].color = m_Color;

		v[1].position = { current_x + char_pixel_width, y_offset, 0.0f };
		v[1].texCoord = { u1, v0 };
		v[1].normal = { 0.0f, 0.0f, 0.0f };
		v[1].color = m_Color;

		v[2].position = { current_x, y_offset + char_pixel_height, 0.0f };
		v[2].texCoord = { u0, v1 };
		v[2].normal = { 0.0f, 0.0f, 0.0f };
		v[2].color = m_Color;

		v[3].position = { current_x + char_pixel_width, y_offset + char_pixel_height, 0.0f };
		v[3].texCoord = { u1, v1 };
		v[3].normal = { 0.0f, 0.0f, 0.0f };
		v[3].color = m_Color;

		pContext->Unmap(m_pVertexBuffer, 0);

		// 頂点バッファ設定
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// DrawCall
		pContext->Draw(m_VertexCount, 0);

		// 次の文字の位置へ移動
		// LSB を考慮した正確な位置計算
		float lsb_scaled = (float)left_side_bearing * scale;
		
		// 前のグリフの右端から次のグリフの左端までの距離を計算
		current_x += (float)advance_width * scale + (float)kerning * scale + margin;
		prev_glyph_draw = glyph_index;
		char_count++;
	}
}

void FontRenderer::SetText(const std::string& text) {
	m_Text = text;
	// テキスト変更時は再度グリフをキャッシュに登録する必要がある
	// Draw() 時に自動的に処理される
}
