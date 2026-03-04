#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "define.h"
#include "LoseAnim.h"
#include "mouse.h"
#include "sound.h"
#include <timeapi.h>
#include <cmath>
#pragma comment(lib, "winmm.lib")
using namespace DirectX;

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Lose Animation (負けアニメーション) — 描画のみ
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
static Sprite* g_LoseBgSprite = nullptr;        // 背景（LoseEDBasuta）
static Sprite* g_LoseGhostSprite = nullptr;     // ゴースト（LoseEDGhost）
static Sprite* g_LoseRopeSprite = nullptr;      // ロープ（LoseEDrope）
static Sprite* g_LoseAnimeLogoSprite = nullptr; // ロゴ（LoseAnimeLogo）
static Sprite* g_LoseVinetSprite = nullptr;     // ビネットオーバーレイ（LoseVinet）
static Sprite* g_Losehaikei = nullptr;     // 追加：背景（Losehaikei）

static DWORD g_LoseStartTime = 0;
static SoundData* g_pBGM = nullptr;

// ゴースト・ロープの基準位置（Initialize時に設定）
static float g_GhostBaseY = 0.0f;
static float g_RopeBaseY = 0.0f;
static float g_GhostBaseX = 0.0f;
static float g_RopeBaseX = 0.0f;

// ロゴ降下アニメーション用
static float g_LogoBaseX = 0.0f;
static float g_LogoBaseY = 0.0f;
static float g_LogoStartY = 0.0f;
static const float LOGO_DROP_DELAY = 0.5f;    // 降下開始までの遅延（秒）
static const float LOGO_DROP_DURATION = 1.2f;  // 降下にかける時間（秒）
static const float LOGO_TOTAL_ROTATION = 720.0f; // 降下中の総回転角（度）
static const float LOGO_TILT_DELAY = 0.15f;       // 着地後、傾き開始までの溜め（秒）
static const float LOGO_TILT_DURATION = 0.12f;    // 「かくっ」と傾く時間（秒）
static const float LOGO_TILT_ANGLE = 15.0f;       // 右斜め傾き角度（度）

float Animation_Lose_GetElapsedTime(void)
{
	if (g_LoseStartTime == 0) return 0.0f;
	return (timeGetTime() - g_LoseStartTime) / 1000.0f;
}

void Animation_Lose_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 背景（LoseEDBasuta）
	g_LoseBgSprite = new Sprite(
		{ SCREEN_WIDTH / 2 + 100, SCREEN_HEIGHT / 2 -50 },
		{ 1080, 1080 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\LoseAnim\\LoseEDBasuta.png"
	);

	// ゴースト・ロープの基準位置を記憶
	g_GhostBaseX = SCREEN_WIDTH / 2 + 100;
	g_GhostBaseY = SCREEN_HEIGHT / 2 - 50;
	g_RopeBaseX  = SCREEN_WIDTH / 2 + 100;
	g_RopeBaseY  = SCREEN_HEIGHT / 2 - 50;

	// ロープ（LoseEDrope）
	g_LoseRopeSprite = new Sprite(
		{ g_RopeBaseX, g_RopeBaseY },
		{ 1080, 1080 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\LoseAnim\\LoseEDrope.png"
	);

	// ゴースト（LoseEDGhost）
	g_LoseGhostSprite = new Sprite(
		{ g_GhostBaseX, g_GhostBaseY },
		{ 1080, 1080 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\LoseAnim\\LoseEDGhost.png"
	);

	// ロゴ（LoseAnimeLogo）— 画面外上部から降下するため初期Yを上に設定
	g_LogoBaseX = SCREEN_WIDTH / 2 - 90;
	g_LogoBaseY = SCREEN_HEIGHT / 2 + 50;
	g_LogoStartY = -600.0f;
	g_LoseAnimeLogoSprite = new Sprite(
		{ g_LogoBaseX, g_LogoStartY },
		{ 1028, 1028 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\LoseAnim\\LoseAnimeLogo.png"
	);

	// ビネットオーバーレイ（LoseVinet）
	g_LoseVinetSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ 1280, 1080 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.85f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\LoseAnim\\LoseVinet.PNG"
	);

	// ビネットオーバーレイ（LoseVinet）
	g_Losehaikei = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ 1536,947 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },//
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\LoseAnim\\haikei1.PNG"
	);

	// サウンド再生
	g_pBGM = LoadMP3("asset/sound/bgm/sad _loose_or result .mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}

	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);

	g_LoseStartTime = timeGetTime();
}

void Animation_Lose_Update(void)
{
	float elapsed = Animation_Lose_GetElapsedTime();

	// ゴースト浮遊：ゆっくり上下に揺れる
	if (g_LoseGhostSprite)
	{
		float bobY = sinf(elapsed * 1.2f) * 8.0f;          // 振幅8px、周期約5.2秒
		float swayX = sinf(elapsed * 0.7f) * 3.0f;          // 横にわずかに揺れる
		g_LoseGhostSprite->SetPosX(g_GhostBaseX + swayX);
		g_LoseGhostSprite->SetPosY(g_GhostBaseY + bobY);
	}

	// ロープ揺れ：ゴーストの動きに少し遅れて追従（位相をずらす）
	if (g_LoseRopeSprite)
	{
		float ropeBobY = sinf(elapsed * 1.2f - 0.4f) * 5.0f;  // 振幅5px、ゴーストより小さく遅れる
		float ropeSwayX = sinf(elapsed * 0.7f - 0.3f) * 2.0f;  // 横揺れも少し遅延
		g_LoseRopeSprite->SetPosX(g_RopeBaseX + ropeSwayX);
		g_LoseRopeSprite->SetPosY(g_RopeBaseY + ropeBobY);
	}

	// ロゴ降下：回転しながら上から降りてくる
	if (g_LoseAnimeLogoSprite)
	{
		if (elapsed < LOGO_DROP_DELAY)
		{
			g_LoseAnimeLogoSprite->SetPosY(g_LogoStartY);
			g_LoseAnimeLogoSprite->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
			g_LoseAnimeLogoSprite->SetRot(0.0f);
		}
		else
		{
			float dropElapsed = elapsed - LOGO_DROP_DELAY;
			if (dropElapsed < LOGO_DROP_DURATION)
			{
				float t = dropElapsed / LOGO_DROP_DURATION;
				// ease-out（減速して着地）
				float eased = 1.0f - (1.0f - t) * (1.0f - t);
				// Y座標：画面外上部 → 目標位置
				float currentY = g_LogoStartY + (g_LogoBaseY - g_LogoStartY) * eased;
				g_LoseAnimeLogoSprite->SetPosY(currentY);
				// 回転：降下中に回る
				g_LoseAnimeLogoSprite->SetRot(LOGO_TOTAL_ROTATION * eased);
				// フェードイン
				float alpha = t;
				if (alpha > 1.0f) alpha = 1.0f;
				g_LoseAnimeLogoSprite->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
			}
			else
			{
				// 降下完了後：「かくっ」と右斜めに傾く
				g_LoseAnimeLogoSprite->SetPosY(g_LogoBaseY);
				g_LoseAnimeLogoSprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

				float tiltElapsed = dropElapsed - LOGO_DROP_DURATION;
				if (tiltElapsed < LOGO_TILT_DELAY)
				{
					// 溜め：回転停止したまま一瞬止まる
					g_LoseAnimeLogoSprite->SetRot(LOGO_TOTAL_ROTATION);
				}
				else if (tiltElapsed < LOGO_TILT_DELAY + LOGO_TILT_DURATION)
				{
					// 「かくっ」と傾く
					float tt = (tiltElapsed - LOGO_TILT_DELAY) / LOGO_TILT_DURATION;
					// ease-out で素早く傾いて止まる
					float easedTilt = 1.0f - (1.0f - tt) * (1.0f - tt);
					g_LoseAnimeLogoSprite->SetRot(LOGO_TOTAL_ROTATION + LOGO_TILT_ANGLE * easedTilt);
				}
				else
				{
					// 傾き完了：固定
					g_LoseAnimeLogoSprite->SetRot(LOGO_TOTAL_ROTATION + LOGO_TILT_ANGLE);
				}
			}
		}
	}
	//自動でタイトルに戻る（約4秒後）
	if(elapsed > 4.0f)
	{
		StartFade(SCENE_TITLE);
	}
	// スペースキーでタイトルへ
	if (Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		StartFade(SCENE_TITLE);
	}
}

void Animation_Lose_Draw(void)
{
	if (g_Losehaikei)     g_Losehaikei->Draw();     // 背景
	if (g_LoseBgSprite)        g_LoseBgSprite->Draw();        // 背景
	if (g_LoseRopeSprite)      g_LoseRopeSprite->Draw();      // ロープ
	if (g_LoseGhostSprite)     g_LoseGhostSprite->Draw();     // ゴースト
	if (g_LoseAnimeLogoSprite) g_LoseAnimeLogoSprite->Draw();	// ロゴ
	if (g_LoseVinetSprite)     g_LoseVinetSprite->Draw();     // ビネット（最前面）
}

void Animation_Lose_Finalize(void)
{
	delete g_LoseBgSprite;        g_LoseBgSprite = nullptr;
	delete g_LoseGhostSprite;     g_LoseGhostSprite = nullptr;
	delete g_LoseRopeSprite;      g_LoseRopeSprite = nullptr;
	delete g_LoseAnimeLogoSprite; g_LoseAnimeLogoSprite = nullptr;
	delete g_LoseVinetSprite;     g_LoseVinetSprite = nullptr;
	delete g_Losehaikei;     g_Losehaikei = nullptr;

	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}
}
