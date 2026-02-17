#pragma execution_character_set("utf-8")

#ifndef SHADER_2D_H
#define SHADER_2D_H

#include <d3d11.h>
#include <DirectXMath.h>

// 2D専用シェーダー管理
// 既存の3D/共通シェーダー(shader.*)とは分離して扱う

struct Shader2D_HoleParams
{
    DirectX::XMFLOAT2 centerPx; // 画面座標(ピクセル)
    float radiusPx;
    float softnessPx; // 0ならパキッと。数pxでぼかし
};

bool Shader2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader2D_Finalize();

void Shader2D_BeginDefault();
void Shader2D_BeginHole();

void Shader2D_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);
void Shader2D_SetHoleParams(const Shader2D_HoleParams& params);

// 追加：2D描画中にPSだけ切替（他のSpriteが巻き込まれない対策）
void Shader2D_SetUseHolePS(bool enable);

#endif // SHADER_2D_H
