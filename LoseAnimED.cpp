#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "LoseAnimED.h"
#include "sound.h"
#include <timeapi.h>
#include <cmath>	// 揺れ用の sinf
#pragma comment(lib, "winmm.lib")
using namespace DirectX;

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;

Sprite* g_LoseEDBasuta = nullptr;		// バスターズ
Sprite* g_LoseEDGhost = nullptr;		// 幽霊
Sprite* g_LoseEDrope = nullptr;		// ロープ

static SoundData* g_pBGM = nullptr;

// 揺れパラメータ
static float g_ElapsedTime = 0.0f;		// 経過時間
static XMFLOAT2 g_GhostBasePos = { 0.0f, 0.0f };	// ゴーストの基本位置
static float g_RopeRightEndX = 0.0f;	// ロープ右端の固定X座標（青い点）
static float g_RopeBaseY = 0.0f;		// ロープのベースY座標
static float g_RopeHalfWidth = 0.0f;	// ロープの表示幅の半分

void Animation_LoseED_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	//バスターズスプライト
	g_LoseEDBasuta = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1080, 1080 },								// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\LoseAnim\\LoseEDBasuta.png"		// テクスチャパス
	);

	//ゴーストスプライト
	g_LoseEDGhost = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1280, 1080 },								// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\LoseAnim\\LoseEDGhost.png"		// テクスチャパス
	);
	g_GhostBasePos = g_LoseEDGhost->GetPos();	// 基本位置を保存

	//ロープスプライト
	g_LoseEDrope = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1280, 1080 },								// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\LoseAnim\\LoseEDrope.png"		// テクスチャパス
	);

	// ロープ右端の固定位置を計算・保存
	XMFLOAT2 ropePos = g_LoseEDrope->GetPos();
	XMFLOAT2 ropeScale = g_LoseEDrope->GetScale();
	float ropeWidth = 1280.0f;	// ロープのテクスチャサイズ

	g_RopeHalfWidth = (ropeWidth * ropeScale.x) * 0.5f;	// ロープ表示幅の半分
	g_RopeRightEndX = ropePos.x + g_RopeHalfWidth;	// 右端のX座標（スクリーン座標）
	g_RopeBaseY = ropePos.y;	// ベースY座標を保存

	// サウンド再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}

	g_ElapsedTime = 0.0f;
}
void Animation_LoseED_Update(void)
{
	const float delta = 1.0f / 60.0f;	// 60FPS想定
	g_ElapsedTime += delta;

	// 揺れパラメータ
	const float swayFrequency = 1.5f;	// 揺れの周波数（Hz）
	const float swayAmplitude = 12.0f;	// 揺れの幅（ピクセル）

	// ゴーストの上下揺れ
	if (g_LoseEDGhost)
	{
		float swayOffset = sinf(g_ElapsedTime * swayFrequency * 3.14159265f * 2.0f) * swayAmplitude;
		g_LoseEDGhost->SetPos({ g_GhostBasePos.x, g_GhostBasePos.y + swayOffset });
	}

	// ロープの左端を幽霊の上下に追従させる（右端固定）
	if (g_LoseEDrope)
	{
		// 幽霊の現在位置を取得
		XMFLOAT2 ghostCurrentPos = g_LoseEDGhost->GetPos();

		// 幽霊の上下動のオフセット量を計算
		float ghostSwayOffset = ghostCurrentPos.y - g_GhostBasePos.y;

		// ロープの中心X座標：右端をg_RopeRightEndXに固定
		float newRopeCenterX = g_RopeRightEndX - g_RopeHalfWidth;

		// ロープのY位置：幽霊に追従
		float newRopeCenterY = g_RopeBaseY + ghostSwayOffset;

		// ロープの位置を更新
		g_LoseEDrope->SetPos({ newRopeCenterX, newRopeCenterY });
	}
}
void Animation_LoseED_Draw(void)
{
	if (g_LoseEDBasuta) g_LoseEDBasuta->Draw();		//バスターズ
	if (g_LoseEDGhost) g_LoseEDGhost->Draw();		//幽霊
	if (g_LoseEDrope) g_LoseEDrope->Draw();		//ロープ
}
void Animation_LoseED_Finalize(void)
{
	delete g_LoseEDBasuta; g_LoseEDBasuta = nullptr;//バスターズ
	delete g_LoseEDGhost; g_LoseEDGhost = nullptr;//幽霊
	delete g_LoseEDrope; g_LoseEDrope = nullptr;//ロープ

	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}
}