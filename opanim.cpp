#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "Fade.h"
#include "shader.h"
#include "Sprite.h"
#include "Keyboard.h"
#include "direct3d.h"
#include "sound.h"
#include "texture.h"
#include "scene.h"
#include <DirectXMath.h>
#include <cmath>
#include <d3d11.h>
#include <cstdint>
#include <iostream>

using namespace DirectX;

// テクスチャ
static ID3D11ShaderResourceView* g_TexKuromurasaki = nullptr;	// 黒紫（背景）
static ID3D11ShaderResourceView* g_TexBikkuri = nullptr;	// ビックリ画像
static ID3D11ShaderResourceView* g_TexYakata = nullptr;		// 屋敷
static ID3D11ShaderResourceView* g_TexBasuta = nullptr;		// バスター
static ID3D11ShaderResourceView* g_TexYurei = nullptr;		// 幽霊
static ID3D11ShaderResourceView* g_TexInazuma = nullptr;		// 稲妻
static ID3D11ShaderResourceView* g_SolidTex = nullptr;		// 単色テクスチャ（フォールバック）

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

//基本パラメータ
static const XMFLOAT2 g_baseSize = { 500.0f, 500.0f };
static const float SCREEN_CENTER_X = SCREEN_WIDTH / 2.0f;
static const float SCREEN_CENTER_Y = SCREEN_HEIGHT / 2.0f;

//幽霊のみ別サイズ
static const XMFLOAT2 g_yureiSize = { 280.0f, 280.0f };

// 幽霊最初の拡大表示（開始からこの秒数だけ縮小して通常サイズへ戻す）
static const float g_yureiInitialEnlargeDuration = 4.0f;
static const float g_yureiInitialStartScale = 1.6f;

// 画面ズーム（幽霊を焦点にするカメラ）
static bool g_cameraZoomActive = false;
static float g_cameraTimer = 0.0f;
static const float g_cameraDuration = 4.0f;			// 幽霊出現からのズーム持続時間
static const float g_cameraStartScale = 1.0f;
static const float g_cameraTargetScale = 1.4f;		// 画面を何倍に拡大するか

//bikkuri2のサイズ
static const XMFLOAT2 g_bikkuriSize = { 100.0f, 100.0f };

//bikkuri2のオフセット位置
static XMFLOAT2 g_bikkuriOffset = { 0, -100 };

//屋敷と黒紫の位置・表示
static XMFLOAT2 g_yakataPos = { 0.0f, 0.0f };
static bool g_yakataInitialized = false;

//稲妻パラメータ
static float g_inazumaTimer = 0.0f;
static float g_inazumaNextTrigger = 0.2f;
static bool g_inazumaActive = false;
static float g_inazumaFlashAlpha = 0.0f;
static unsigned int g_inazumaSeed = 0xC0FFEEu;
static const XMFLOAT2 g_inazumaSize = { 1280.0f, 720.0f };	// 稲妻サイズ

//バスターパラメータ
static XMFLOAT2 g_basutaStartPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaCurrentPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaTargetPos = { 0.0f, 0.0f };
static float g_basutaAlpha = 0.0f;
static float g_basutaTimer = 0.0f;
static bool g_basutaMoving = false;
static const float g_basutaMoveSpeed = 140.0f;
static const float g_basutaMoveStartTime = 1.5f;

//幽霊パラメータ
static XMFLOAT2 g_yureiPos = { 0.0f, 0.0f };
static XMFLOAT2 g_yureiTargetPos = { 0.0f, 0.0f };
static XMFLOAT2 g_yureiCurrentPos = { 0.0f, 0.0f };
static float g_yureiAlpha = 0.0f;
static float g_yureiTimer = 0.0f;
static bool g_yureiReacting = false;
static bool g_yureiMoving = false;
static const float g_yureiAppearTime = 2.0f;
static const float g_yureiFadeDuration = 1.5f;
static const float g_yureiMoveSpeed = 100.0f;
static const float g_yureiReactDistance = 600.0f;

// --- 幽霊を一瞬だけ右向きにするためのフリップ制御 ---
static bool g_yureiFlipActive = false;
static float g_yureiFlipTimer = 0.0f;
static const float g_yureiFlipDuration = 0.5f; // 秒だけ右向きにする

//タイムライン
static float g_elapsedTime = 0.0f;
static bool g_fadeStarted = false;
static bool g_waitStarted = false;
static float g_waitTimer = 0.0f;

// --- 幽霊フリップ解除後の待機と右方向移動制御 ---
static bool g_yureiWaitingAfterFlip = false;
static float g_yureiWaitTimer = 0.0f;
static const float g_yureiWaitDuration = 1.0f;	// フリップ解除後の待機時間
static const float g_yureiLeftMoveSpeed = -120.0f;	// 右方向移動速度

static SoundData* g_pBGM = nullptr;
//ランダム関数
static float Rand01()
{
	g_inazumaSeed = g_inazumaSeed * 1664525u + 1013904223u;
	return (float)(g_inazumaSeed & 0x00FFFFFFu) / (float)0x01000000u;
}

// Easing関数
static float EaseOutCubic(float t)
{
	if (t <= 0.0f) return 0.0f;
	if (t >= 1.0f) return 1.0f;
	float inv = 1.0f - t;
	return 1.0f - inv * inv * inv;
}

// 単色テクスチャ作成（device が null の場合は Direct3D_GetDevice を試す）
static ID3D11ShaderResourceView* CreateSolidSRV(ID3D11Device* device, uint32_t rgba)
{
	if (!device) device = Direct3D_GetDevice();
	if (!device) return nullptr;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = 1;
	desc.Height = 1;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = &rgba;
	sd.SysMemPitch = sizeof(rgba);

	ID3D11Texture2D* tex = nullptr;
	HRESULT hr = device->CreateTexture2D(&desc, &sd, &tex);
	if (FAILED(hr) || !tex) return nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* srv = nullptr;
	hr = device->CreateShaderResourceView(tex, &srvDesc, &srv);
	tex->Release();
	if (FAILED(hr)) { if (srv) srv->Release(); return nullptr; }
	return srv;
}

// LoadTexture が失敗したときに単色でフォールバックするユーティリティ
static ID3D11ShaderResourceView* LoadTextureOrFallback(const wchar_t* path, uint32_t fallbackRGBA)
{
	ID3D11ShaderResourceView* tex = LoadTexture(path);
	if (tex) return tex;

	// LoadTexture に失敗した場合は単色テクスチャを作成して返す
	if (!g_SolidTex)
	{
		g_SolidTex = CreateSolidSRV(g_pDevice, fallbackRGBA);
	}
	// 複数のフォールバックを同一 SRV で共有して OK
	return g_SolidTex;
}

void OpAnim_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 実行フレームレート
	SetFPS(40);

	g_pDevice = pDevice ? pDevice : Direct3D_GetDevice();
	g_pContext = pContext ? pContext : Direct3D_GetDeviceContext();

	// 単色フォールバック用の白色（RGBA）
	uint32_t white = (255u) | (255u << 8) | (255u << 16) | (255u << 24);

	// テクスチャロード（存在しない場合は単色フォールバック）
	g_TexYakata = LoadTextureOrFallback(L"asset\\yureihen\\yakata_jimen1.png", white);
	g_TexBasuta = LoadTextureOrFallback(L"asset\\yureihen\\Alpha_Tex\\basuta1.png", white);
	g_TexYurei = LoadTextureOrFallback(L"asset\\yureihen\\Alpha_Tex\\yurei1.png", white);
	g_TexInazuma = LoadTextureOrFallback(L"asset\\yureihen\\inazuma2.png", white);
	g_TexKuromurasaki = LoadTextureOrFallback(L"asset\\yureihen\\Alpha_Tex\\kuromurasaki.png", white);
	g_TexBikkuri = LoadTextureOrFallback(L"asset\\yureihen\\Alpha_Tex\\bikkuri2.png", white);
	// サウンド再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}

	// 屋敷初期化
	g_yakataPos.x = SCREEN_CENTER_X - 400.0f;
	g_yakataPos.y = SCREEN_CENTER_Y + 120.0f;

	// 幽霊初期位置（屋敷近く・右側）
	g_yureiPos.x = g_yakataPos.x + 80.0f;
	g_yureiPos.y = g_yakataPos.y - 150.0f;

	// バスター初期位置（画面右外）
	g_basutaStartPos.x = SCREEN_WIDTH + 200.0f;
	g_basutaStartPos.y = SCREEN_CENTER_Y + 50.0f;
	g_basutaCurrentPos = g_basutaStartPos;

	// バスター目標位置（屋敷の左側）
	g_basutaTargetPos.x = g_yakataPos.x - 150.0f;
	g_basutaTargetPos.y = g_yakataPos.y + 50.0f;

	// 幽霊目標位置（屋敷内奥）
	g_yureiTargetPos.x = g_yakataPos.x - 80.0f;
	g_yureiTargetPos.y = g_yakataPos.y - 60.0f;

	// 稲妻初期化
	g_inazumaNextTrigger = 0.5f + Rand01() * 1.5f;
	g_inazumaActive = false;

	g_yakataInitialized = true;
}

void OpAnim_Finalize(void)
{
	if (g_TexKuromurasaki) { g_TexKuromurasaki->Release(); g_TexKuromurasaki = nullptr; }
	if (g_TexYakata) { g_TexYakata->Release(); g_TexYakata = nullptr; }
	if (g_TexBasuta) { g_TexBasuta->Release(); g_TexBasuta = nullptr; }
	if (g_TexYurei) { g_TexYurei->Release(); g_TexYurei = nullptr; }
	if (g_TexInazuma) { g_TexInazuma->Release(); g_TexInazuma = nullptr; }

	// g_SolidTex は共有で作成しているため、ここで解放しておく
	if (g_SolidTex) { g_SolidTex->Release(); g_SolidTex = nullptr; }

	g_pDevice = nullptr;
	g_pContext = nullptr;

	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}


	SetFPS(40);
}

void OpAnim_Update()
{
	// fixed delta 60fps に合わせる
	const float delta = 1.0f / 60.0f;
	g_elapsedTime += delta;

	// --- 稲妻更新 ---
	g_inazumaNextTrigger -= delta;
	if (g_inazumaNextTrigger <= 0.0f && !g_inazumaActive)
	{
		g_inazumaActive = true;
		g_inazumaFlashAlpha = 0.7f + Rand01() * 0.3f;
		g_inazumaTimer = 0.0f;
	}

	if (g_inazumaActive)
	{
		g_inazumaTimer += delta;
		float duration = 0.08f + Rand01() * 0.12f;
		float fade = 1.0f - EaseOutCubic(g_inazumaTimer / duration);
		g_inazumaFlashAlpha *= fade;

		if (g_inazumaTimer >= duration)
		{
			g_inazumaActive = false;
			g_inazumaFlashAlpha = 0.0f;
			g_inazumaNextTrigger = 1.0f + Rand01() * 2.5f;
		}
	}

	// --- バスター更新 ---
	if (g_elapsedTime >= g_basutaMoveStartTime && !g_basutaMoving)
	{
		g_basutaMoving = true;
		g_basutaTimer = 0.0f;
	}

	if (g_basutaMoving)
	{
		g_basutaTimer += delta;
		float dx = g_basutaTargetPos.x - g_basutaStartPos.x;
		float dy = g_basutaTargetPos.y - g_basutaStartPos.y;
		float totalDist = sqrtf(dx * dx + dy * dy);
		if (totalDist <= 0.001f) totalDist = 0.001f;
		float moveAmount = g_basutaMoveSpeed * g_basutaTimer;
		float ratio = moveAmount / totalDist;

		if (ratio >= 1.0f)
		{
			g_basutaCurrentPos = g_basutaTargetPos;
			ratio = 1.0f;
		}
		else
		{
			g_basutaCurrentPos.x = g_basutaStartPos.x + dx * ratio;
			g_basutaCurrentPos.y = g_basutaStartPos.y + dy * ratio;
		}

		// バスター上下揺れ
		g_basutaCurrentPos.y += sinf(g_elapsedTime * 3.5f) * 8.0f;

		// バスターフェードイン（時間ベース）
		float fadeDuration = 2.0f;
		float fadeTime = g_elapsedTime - g_basutaMoveStartTime;
		g_basutaAlpha = fadeTime / fadeDuration;
		if (g_basutaAlpha > 1.0f) g_basutaAlpha = 1.0f;
		if (g_basutaAlpha < 0.0f) g_basutaAlpha = 0.0f;
	}

	// --- 幽霊更新 ---
	if (!g_yureiReacting && g_elapsedTime > 3.5f)
	{
		float dx = g_basutaCurrentPos.x - g_yureiPos.x;
		float dy = g_basutaCurrentPos.y - g_yureiPos.y;
		float dist = sqrtf(dx * dx + dy * dy);

		if (dist < g_yureiReactDistance)
		{
			g_yureiReacting = true;
			g_yureiTimer = 0.0f;

			// ここで一瞬だけ右向き（水平反転）にするためのフラグをセット
			if (!g_yureiFlipActive)
			{
				g_yureiFlipActive = true;
				g_yureiFlipTimer = 0.05f;
				std::cout << "幽霊が右を向いた" << std::endl;
			}
		}
	}

	// 幽霊の上下揺れ
	g_yureiCurrentPos = g_yureiPos;
	g_yureiCurrentPos.y += sinf(g_elapsedTime * 4.5f) * 8.0f;

	if (g_elapsedTime >= g_yureiAppearTime)
	{
		float fadeTime = g_elapsedTime - g_yureiAppearTime;
		g_yureiAlpha = fadeTime / g_yureiFadeDuration;
		if (g_yureiAlpha > 1.0f) g_yureiAlpha = 1.0f;

		// 幽霊出現時にカメラズームを開始（1度だけ）
		if (!g_cameraZoomActive && g_cameraTimer == 0.0f)
		{
			g_cameraZoomActive = true;
			g_cameraTimer = 0.0f;
		}
	}

	// 幽霊が館に向かって移動
	if (g_yureiReacting)
	{
		g_yureiTimer += delta;

		// 待機中は位置更新なし
		if (!g_yureiWaitingAfterFlip && !g_yureiFlipActive)
		{
			g_yureiPos.x += g_yureiLeftMoveSpeed * delta;
		}
		// 幽霊フェードアウト（館へ入る）
		float fadeOutTime = 5.0f;
		if (g_yureiTimer > fadeOutTime)
		{
			float outRatio = (g_yureiTimer - fadeOutTime) / 0.5f;
			if (outRatio > 1.0f) outRatio = 1.0f;
			g_yureiAlpha *= (1.0f - outRatio);
		}
	}

	// 幽霊のフリップタイマー更新
	if (g_yureiFlipActive)
	{
		g_yureiFlipTimer += delta;
		if (g_yureiFlipTimer >= g_yureiFlipDuration)
		{
			g_yureiFlipActive = false;
			g_yureiFlipTimer = 0.0f;
			g_yureiWaitingAfterFlip = true;
			g_yureiWaitTimer = 0.0f;
		}
	}

	// フリップ解除後の待機時間管理
	if (g_yureiWaitingAfterFlip && g_yureiReacting)
	{
		g_yureiWaitTimer += delta;
		if (g_yureiWaitTimer >= g_yureiWaitDuration)
		{
			g_yureiWaitingAfterFlip = false;
		}
	}

	// カメラズームのタイマー更新（幽霊出現時に開始）
	if (g_cameraZoomActive)
	{
		g_cameraTimer += delta;
		if (g_cameraTimer >= g_cameraDuration)
		{
			// ズーム終了（必要ならループやホールドに変更可）
			g_cameraZoomActive = false;
			// タイマーを固定しておくと後で参照可能
			g_cameraTimer = g_cameraDuration;
		}
	}

	// --- フェードアウトと次シーン遷移 ---
	if (!g_fadeStarted && g_elapsedTime > 8.0f && GetFadeState() == FADE_NONE)
	{
		g_fadeStarted = true;
		StartFade(SCENE_GAME);
	}

	if (g_fadeStarted && !g_waitStarted && GetFadeState() == FADE_NONE)
	{
		g_waitStarted = true;
		g_waitTimer = 0.0f;
	}

	if (g_waitStarted)
	{
		g_waitTimer += delta;
		if (g_waitTimer >= 1.5f) SetScene(SCENE_TITLE);
	}

	if (Keyboard_IsKeyDownTrigger(KK_E)) SetScene(SCENE_TITLE);
}

void OpAnimDraw(void)
{
	const float screenWidth = (float)Direct3D_GetBackBufferWidth();
	const float screenHeight = (float)Direct3D_GetBackBufferHeight();

	// カメラ（投影行列）を幽霊にフォーカスしてズームする
	if (g_cameraZoomActive)
	{
		// スケール（イージング）
		float t = g_cameraTimer / g_cameraDuration;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;
		float eased = EaseOutCubic(t);
		float scale = g_cameraStartScale + (g_cameraTargetScale - g_cameraStartScale) * eased;

		// ズーム時の表示領域（スケールにより小さくなる）
		float viewW = screenWidth / scale;
		float viewH = screenHeight / scale;

		// 中心は幽霊の現在スクリーン座標（スプライト座標系と同じ想定）
		float centerX = g_yureiCurrentPos.x;
		float centerY = g_yureiCurrentPos.y;

		float left = centerX - viewW * 0.5f;
		float right = centerX + viewW * 0.5f;
		float top = centerY - viewH * 0.5f;
		float bottom = centerY + viewH * 0.5f;

		Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(left, right, bottom, top, 0.0f, 1.0f));
	}
	else
	{
		// 通常（画面全体）
		Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, screenWidth, screenHeight, 0.0f, 0.0f, 1.0f));
	}

	SetBlendState(BLENDSTATE_ALFA);

	// 背景（黒紫）
	if (g_TexKuromurasaki)
	{
		g_pContext->PSSetShaderResources(0, 1, &g_TexKuromurasaki);
		Sprite_Single_Draw(XMFLOAT2{ screenWidth * 0.5f, screenHeight * 0.5f },
			XMFLOAT2{ screenWidth, screenHeight }, 0.0f,
			XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f }, BLENDSTATE_ALFA, g_TexKuromurasaki);
	}

	// 稲妻（オーバーレイ）
	if (g_TexInazuma)
	{
		SetBlendState(BLENDSTATE_ADD);
		g_pContext->PSSetShaderResources(0, 1, &g_TexInazuma);

		float baseAlpha = 0.5f;
		float flashBoost = g_inazumaFlashAlpha * 0.9f;
		float inazumaAlpha = baseAlpha + flashBoost;
		if (inazumaAlpha > 1.5f) inazumaAlpha = 1.5f;

		Sprite_Single_Draw(XMFLOAT2{ screenWidth * 0.5f, screenHeight * 0.5f },
			XMFLOAT2{ screenWidth, screenHeight }, 0.0f,
			XMFLOAT4{ 0.95f, 0.95f, 1.0f, inazumaAlpha }, BLENDSTATE_ADD, g_TexInazuma);

		SetBlendState(BLENDSTATE_ALFA);
	}

	// 屋敷
	if (g_TexYakata)
	{
		g_pContext->PSSetShaderResources(0, 1, &g_TexYakata);
		Sprite_Single_Draw(g_yakataPos, g_baseSize, 0.0f,
			XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f }, BLENDSTATE_ALFA, g_TexYakata);
	}

	// バスター（前景）
	if (g_TexBasuta && g_basutaAlpha > 0.0f)
	{
		g_pContext->PSSetShaderResources(0, 1, &g_TexBasuta);
		Sprite_Single_Draw(g_basutaCurrentPos, g_baseSize, 0.0f,
			XMFLOAT4{ 1.0f, 1.0f, 1.0f, g_basutaAlpha }, BLENDSTATE_ALFA, g_TexBasuta);
	}

	// 幽霊（右向きフリップを反映）
	if (g_TexYurei && g_yureiAlpha > 0.0f)
	{
		// 出現（g_yureiAppearTime）からの経過で拡大→通常サイズへ戻す（スプライト固有の拡大）
		float yureiScale = 1.0f;
		float timeSinceAppear = g_elapsedTime - g_yureiAppearTime;
		if (timeSinceAppear < 0.0f) timeSinceAppear = 0.0f;
		if (timeSinceAppear < g_yureiInitialEnlargeDuration)
		{
			float t = timeSinceAppear / g_yureiInitialEnlargeDuration;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;
			float eased = EaseOutCubic(t);
			// eased==0 -> start scale, eased==1 -> normal scale(1.0f)
			yureiScale = 1.0f + (g_yureiInitialStartScale - 1.0f) * (1.0f - eased);
		}
		XMFLOAT2 yureiDrawSize = { g_yureiSize.x * yureiScale, g_yureiSize.y * yureiScale };

		g_pContext->PSSetShaderResources(0, 1, &g_TexYurei);
		FLIPTYPE2D flipType = FLIPTYPE2D::FLIPTYPE2D_NONE;
		if (g_yureiFlipActive) flipType = FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL;
		Sprite_Single_Draw(g_yureiCurrentPos, yureiDrawSize, 0.0f,
			XMFLOAT4{ 1.0f, 1.0f, 1.0f, g_yureiAlpha }, BLENDSTATE_ALFA, g_TexYurei, flipType);
	}

	// bikkuri（幽霊が右を向いた瞬間に表示）
	// 幽霊が左向きになった瞬間に消すため、表示条件は g_yureiFlipActive のみに変更
	if (g_TexBikkuri && g_yureiFlipActive && g_yureiAlpha > 0.0f)
	{
		XMFLOAT2 bikkuriPos = { g_yureiCurrentPos.x + g_bikkuriOffset.x, g_yureiCurrentPos.y + g_bikkuriOffset.y };
		g_pContext->PSSetShaderResources(0, 1, &g_TexBikkuri);
		Sprite_Single_Draw(bikkuriPos, g_bikkuriSize, 0.0f,
			XMFLOAT4{ 1.0f, 1.0f, 1.0f, g_yureiAlpha }, BLENDSTATE_ALFA, g_TexBikkuri);
	}
}