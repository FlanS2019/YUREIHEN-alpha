/*==============================================================================
   チュートリアル用3Dモデル管理 [Tutorial_Object.h]
==============================================================================*/
#pragma once

#include <DirectXMath.h>
using namespace DirectX;

// チュートリアル用オブジェクト（円盤等）の初期化・更新・描画・終了
void TutorialObject_Initialize(void);
void TutorialObject_Update(void);
void TutorialObject_Draw(void);
void TutorialObject_Finalize(void);

// 円盤接触フラグのポインタを返す
bool* TutorialObject_GetEnbanTouchedPtr(void);

// ピアノ憑依フラグのポインタを返す
bool* TutorialObject_GetPianoPossessedPtr(void);
