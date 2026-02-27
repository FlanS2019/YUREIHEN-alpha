#pragma once
/*==============================================================================
   デバッグモデルビューアシーン [debug_model_scene.h]
   asset\model フォルダ内の全 .fbx を自動列挙してグリッド表示する
==============================================================================*/

#include <d3d11.h>

void DebugModelScene_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void DebugModelScene_Update(void);
void DebugModelScene_Draw(void);
void DebugModelScene_Finalize(void);
