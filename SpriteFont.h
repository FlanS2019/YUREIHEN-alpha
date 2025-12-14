// SpriteFont.h - DirectXTK SpriteFontラッパークラス
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "DirectXTK/Inc/SpriteBatch.h"
#include "DirectXTK/Inc/SpriteFont.h"
#include "component.h"
#include "direct3d.h"
#include <memory>
#include <string>
#include <unordered_map>

using namespace DirectX;

// フォント識別用マクロ定義（ユーザーが追加可能）
#define FONT_DEFAULT        0
#define FONT_KAISEIDECOL_M  1
#define FONT_KAISEIDECOL_B  2
// 必要に応じて追加...

// プロトタイプ宣言
void SpriteFont_Init(void);
void SpriteFont_Uninit(void);
void SpriteFont_FontInit(int fontId, const wchar_t* spriteFontPath);
DirectX::SpriteFont* SpriteFont_GetFont(int fontId);
DirectX::SpriteBatch* SpriteFont_GetSpriteBatch(void);

// SpriteFontクラス - Sprite互換のフォント描画クラス
class SpriteFont2D : public Transform2D
{
private:
    int m_FontId;
    std::wstring m_Text;
    XMFLOAT4 m_TextColor;

public:
    // コンストラクタ（Transform2D初期化、フォントID、初期テキスト）
    SpriteFont2D(const XMFLOAT2& pos, float rotation, const XMFLOAT2& scale,
                 int fontId, const std::wstring& text = L"")
        : Transform2D(pos, rotation, scale)
        , m_FontId(fontId)
        , m_Text(text)
        , m_TextColor({ 1, 1, 1, 1 })
    {
    }

    ~SpriteFont2D() = default;

    // フォントIDを設定
    void SetFontId(int fontId) { m_FontId = fontId; }
    int GetFontId() const { return m_FontId; }

    // テキストを設定
    void SetText(const std::wstring& text) { m_Text = text; }
    std::wstring GetText() const { return m_Text; }

    // 色を設定（Sprite互換）
    void SetColor(const XMFLOAT4& color) { m_TextColor = color; }
    XMFLOAT4 GetColor() const { return m_TextColor; }

    // 描画（Sprite互換）
    void Draw()
    {
        DirectX::SpriteFont* font = SpriteFont_GetFont(m_FontId);
        DirectX::SpriteBatch* batch = SpriteFont_GetSpriteBatch();
        if (m_Text.empty() || !font || !batch) return;

        batch->Begin();
        font->DrawString(
            batch,
            m_Text.c_str(),
            XMFLOAT2(m_Position.x, m_Position.y),
            XMLoadFloat4(&m_TextColor),
            XMConvertToRadians(m_Rotation),
            XMFLOAT2(0, 0),
            XMFLOAT2(m_Scale.x, m_Scale.y)
        );
        batch->End();
    }

    // テキストのサイズを取得
    XMFLOAT2 MeasureString() const
    {
        DirectX::SpriteFont* font = SpriteFont_GetFont(m_FontId);
        if (!font || m_Text.empty()) return XMFLOAT2(0, 0);
        XMVECTOR size = font->MeasureString(m_Text.c_str());
        XMFLOAT2 result;
        XMStoreFloat2(&result, size);
        return result;
    }
};
