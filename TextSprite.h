// TextSprite.h - stb_truetype.hを使用したテキスト描画スプライトクラス
#pragma once

#include <d3d11.h>
#include "direct3d.h"
#include "texture.h"
#include "component.h"
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <memory>

// stb_truetypeの宣言のみ（実装はTextSprite.cppで1回だけ行う）
#include "stb_truetype.h"

using namespace DirectX;

struct TextVertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT4 color;
	XMFLOAT2 texCoord;
};

struct FontData
{
	unsigned char* bitmap;
	int bitmapWidth;
	int bitmapHeight;
	stbtt_bakedchar* cdata;
	ID3D11ShaderResourceView* texture;
	float fontSize;

	// フォント幅情報
	int charRangeStart;       // サポート文字範囲開始
	int charRangeEnd;         // サポート文字範囲終了
};

void TextSprite_Initialize(void);
void TextSprite_Finalize(void);
FontData* TextSprite_LoadFont(const char* fontPath, float fontSize, int bitmapSize = 512);
void TextSprite_RenderText(XMFLOAT2 pos, XMFLOAT2 scale, float rotation, XMFLOAT4 color, 
							const std::wstring& text, FontData* fontData, BLENDSTATE bstate);

// TextSpriteクラス - Spriteと互換のシンプルなテキスト描画
class TextSprite : public Transform2D
{
protected:
	std::wstring m_Text;
	XMFLOAT4 m_Color;
	BLENDSTATE m_BlendState;
	FontData* m_FontData;
	float m_LineHeight;

public:
	TextSprite(const XMFLOAT2& pos, const XMFLOAT2& scale, float rotation,
		const XMFLOAT4& color, BLENDSTATE bstate, const std::wstring& text, FontData* fontData)
		: Transform2D(pos, rotation, scale)
		, m_Text(text)
		, m_Color(color)
		, m_BlendState(bstate)
		, m_FontData(fontData)
		, m_LineHeight(fontData ? fontData->fontSize : 16.0f)
	{
	}

	~TextSprite() = default;

	void SetText(const std::wstring& text) { m_Text = text; }
	std::wstring GetText(void) const { return m_Text; }

	XMFLOAT4 GetColor(void) const { return m_Color; }
	void SetColor(const XMFLOAT4& color) { m_Color = color; }

	BLENDSTATE GetBlendState(void) const { return m_BlendState; }

	void SetFontData(FontData* fontData) { m_FontData = fontData; }
	FontData* GetFontData(void) const { return m_FontData; }

	void Draw()
	{
		if (m_FontData) {
			TextSprite_RenderText(m_Position, m_Scale, m_Rotation, m_Color, m_Text, m_FontData, m_BlendState);
		}
	}

	float GetTextWidth(void) const;
};
