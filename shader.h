/*==============================================================================

   シェーダー [shader.h]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER_H
#define	SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"


bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader_Finalize();

void Shader_SetMatrix(const DirectX::XMMATRIX& matrix);

void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader_SetPointLight(PointLight* light);
void Shader_SetAmbientLight(AmbientLight* ambient);
void Shader_SetMaterialColor(const DirectX::XMFLOAT4& color);
void Shader_SetCameraPos(const DirectX::XMFLOAT3& pos);

void Shader_Begin();
void Shader_BeginInstance(); // インスタンス描画専用
void Shader_RefreshState();  // シェーダーの状態をリセット（毎フレーム呼ぶ）

#endif // SHADER_H
