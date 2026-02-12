#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "HoleSprite.h"
#include "ClickFont.h"
using namespace DirectX;

// ==========================================
// チュートリアルページ構造体
// ==========================================
struct TutorialPage
{
	XMFLOAT2 holeCenter;                     // HoleSpriteの穴位置
	float    holeRadius;                     // 穴の半径
	std::vector<Sprite*>       sprites;      // 表示スプライト群
	std::vector<FontRenderer*> fonts;        // 表示テキスト群
};

// ==========================================
// チュートリアル画面
// ==========================================

void UI_Tutorial_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void UI_Tutorial_Finalize(void);
void UI_Tutorial_Update(void);
void UI_Tutorial_Draw(void);

bool UI_Tutorial_IsActive(void);

// ポーズメニュー等からチュートリアルを開閉する
void UI_Tutorial_Start(void);
void UI_Tutorial_End(void);

