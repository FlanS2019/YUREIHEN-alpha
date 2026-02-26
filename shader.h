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
void Shader_ClearPointLights();
void Shader_AddPointLight(PointLight* light);
void Shader_SetPointLight(PointLight* light);
void Shader_SetAmbientLight(AmbientLight* ambient);
void Shader_SetMaterialColor(const DirectX::XMFLOAT4& color);
void Shader_SetCameraPos(const DirectX::XMFLOAT3& pos);
void Shader_FlushLights(); // ライトバッファをGPUに転送（描画前に自動呼び出し）

void Shader_Begin();
void Shader_BeginSkinning(); // スキニングアニメーション描画専用
void Shader_SetBoneMatrices(const DirectX::XMMATRIX* matrices, unsigned int count);
void Shader_BeginInstance(); // インスタンス描画専用
void Shader_RefreshState();  // シェーダーの状態をリセット（毎フレーム呼ぶ）

#endif // SHADER_H
