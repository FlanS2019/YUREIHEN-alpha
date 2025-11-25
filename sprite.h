#pragma once

#include <d3d11.h>
#include "direct3d.h"
#include "texture.h"
#include "component.h"
#include <DirectXMath.h>
using namespace DirectX;

// 頂点構造体
struct Vertex
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT3 normal;
	XMFLOAT4 color;    // 頂点カラー（R,G,B,A）
	XMFLOAT2 texCoord; // テクスチャ座標
};

// Sprite は 2D 用 Transform2D に準拠
class Sprite : public Transform2D
{
protected:
	XMFLOAT4 m_Color;    // スプライトの色
	BLENDSTATE m_BlendState; // ブレンドステート
	ID3D11ShaderResourceView* m_Texture; // テクスチャ
public:
	// pos: 中心位置, size: 幅・高さ, rotation: 角度(度)
	// texturePath: テクスチャファイルパス（LoadTexture内で読み込む）
	Sprite(const XMFLOAT2& pos, const XMFLOAT2& size, float rotation, const XMFLOAT4& color, BLENDSTATE bstate, const wchar_t* texturePath)
		: Transform2D(pos, rotation, size), m_Color(color), m_BlendState(bstate), m_Texture(nullptr)
	{
		if (texturePath) {
			m_Texture = LoadTexture(texturePath);
		}
	}

	~Sprite()
	{
		if (m_Texture) {
			m_Texture->Release();
			m_Texture = nullptr;
		}
	}

	XMFLOAT4 GetColor(void) const { return m_Color; }
	BLENDSTATE GetBlendState(void) const { return m_BlendState; }
	ID3D11ShaderResourceView* GetTexture(void) const { return m_Texture; }

	// インスタンスで描画する
	void Draw(void);
};


// プロトタイプ宣言
void Sprite_Initialize(void);
void Sprite_Finalize(void);
//void Sprite_Draw(XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 color, BLENDSTATE bstate, ID3D11ShaderResourceView* g_Texture);
