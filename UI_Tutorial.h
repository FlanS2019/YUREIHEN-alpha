#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <functional>
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

	// 条件待機モード:
	//   nullptr でない場合、このページのSPACEで次へ遷移する代わりに
	//   チュートリアルを一時停止してゲームに制御を返す。
	//   条件が true を返したとき自動的に次ページへ進む。
	std::function<bool()> waitCondition;
	bool autoWait = false; // trueのとき、ページ表示と同時に自動でテストプレイ開始
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

// 条件待機モードのチュートリアル再開（game.cpp等から呼ぶ）
void UI_Tutorial_ResumeFromWait(void);

// 条件待機中か取得
bool UI_Tutorial_IsWaiting(void);

