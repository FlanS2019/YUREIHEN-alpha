// SpriteFont.cpp - DirectXTK SpriteFontリソース管理
#include "SpriteFont.h"

// 静的変数（フォントキャッシュ）
static std::unique_ptr<DirectX::SpriteBatch> g_SpriteBatch = nullptr;
static std::unordered_map<int, std::unique_ptr<DirectX::SpriteFont>> g_FontCache;

// 初期化
void SpriteFont_Init(void)
{
    ID3D11DeviceContext* context = Direct3D_GetDeviceContext();
    g_SpriteBatch = std::make_unique<DirectX::SpriteBatch>(context);
}

// 終了処理
void SpriteFont_Uninit(void)
{
    g_FontCache.clear();
    g_SpriteBatch.reset();
}

// フォント登録（マクロIDとパスを紐づけ）
void SpriteFont_FontInit(int fontId, const wchar_t* spriteFontPath)
{
    ID3D11Device* device = Direct3D_GetDevice();
    g_FontCache[fontId] = std::make_unique<DirectX::SpriteFont>(device, spriteFontPath);
}

// フォント取得
DirectX::SpriteFont* SpriteFont_GetFont(int fontId)
{
    auto it = g_FontCache.find(fontId);
    if (it != g_FontCache.end())
    {
        return it->second.get();
    }
    return nullptr;
}

// SpriteBatch取得
DirectX::SpriteBatch* SpriteFont_GetSpriteBatch(void)
{
    return g_SpriteBatch.get();
}
