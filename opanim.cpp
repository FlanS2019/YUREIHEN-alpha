#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "Fade.h"
#include "shader.h"
#include "Sprite.h"
#include "Keyboard.h"
#include "direct3d.h"
#include "texture.h"
#include "scene.h"
#include <DirectXMath.h>
#include <cmath>
#include <d3d11.h>
#include <cstdint>

using namespace DirectX;

// テクスチャ
static ID3D11ShaderResourceView* g_TexKuromurasaki = nullptr;	// 黒紫（背景）
static ID3D11ShaderResourceView* g_TexYakata = nullptr;		// 屋敷
static ID3D11ShaderResourceView* g_TexBasuta = nullptr;		// バスター
static ID3D11ShaderResourceView* g_TexYurei = nullptr;		// 幽霊
static ID3D11ShaderResourceView* g_TexInazuma = nullptr;		// 稲妻
static ID3D11ShaderResourceView* g_SolidTex = nullptr;		// 単色テクスチャ

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 基本パラメータ
static const XMFLOAT2 g_baseSize = { 500.0f, 500.0f };
static const float SCREEN_CENTER_X = SCREEN_WIDTH / 2.0f;
static const float SCREEN_CENTER_Y = SCREEN_HEIGHT / 2.0f;

// 屋敷と黒紫の位置・表示
static XMFLOAT2 g_yakataPos = { 0.0f, 0.0f };
static bool g_yakataInitialized = false;

// 稲妻パラメータ
static float g_inazumaTimer = 0.0f;
static float g_inazumaNextTrigger = 0.5f;
static bool g_inazumaActive = false;
static float g_inazumaFlashAlpha = 0.0f;
static unsigned int g_inazumaSeed = 0xC0FFEEu;
static const XMFLOAT2 g_inazumaSize = { 1920.0f, 1080.0f };	// 稲妻サイズ

// バスターパラメータ
static XMFLOAT2 g_basutaStartPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaCurrentPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaTargetPos = { 0.0f, 0.0f };
static float g_basutaAlpha = 0.0f;
static float g_basutaTimer = 0.0f;
static bool g_basutaMoving = false;
static const float g_basutaMoveSpeed = 150.0f;
static const float g_basutaMoveStartTime = 1.5f;

// 幽霊パラメータ
static XMFLOAT2 g_yureiPos = { 0.0f, 0.0f };
static XMFLOAT2 g_yureiTargetPos = { 0.0f, 0.0f };
static float g_yureiAlpha = 0.0f;
static float g_yureiTimer = 0.0f;
static bool g_yureiReacting = false;
static bool g_yureiMoving = false;
static const float g_yureiAppearTime = 2.0f;
static const float g_yureiFadeDuration = 1.5f;
static const float g_yureiMoveSpeed = 100.0f;
static const float g_yureiReactDistance = 600.0f;

// タイムライン
static float g_elapsedTime = 0.0f;
static bool g_fadeStarted = false;
static bool g_waitStarted = false;
static float g_waitTimer = 0.0f;

// ランダム関数
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

// 単色テクスチャ作成
static ID3D11ShaderResourceView* CreateSolidSRV(ID3D11Device* device, uint32_t rgba)
{
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

void OpAnim_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	SetFPS(40);

	g_pDevice = pDevice;
	g_pContext = pContext;

	// テクスチャロード
	g_TexKuromurasaki = LoadTexture(L"asset\\yureihen\\kuromurasaki.png");
	g_TexYakata = LoadTexture(L"asset\\yureihen\\yakata_jimen1.png");
	g_TexBasuta = LoadTexture(L"asset\\yureihen\\basuta1.png");
	g_TexYurei = LoadTexture(L"asset\\yureihen\\yurei1.png");
	g_TexInazuma = LoadTexture(L"asset\\yureihen\\inazuma2.png");

	// 単色テクスチャ
	if (g_pDevice && !g_SolidTex)
	{
		uint32_t white = (255u) | (255u << 8) | (255u << 16) | (255u << 24);
		g_SolidTex = CreateSolidSRV(g_pDevice, white);
	}

	// 屋敷位置初期化
	g_yakataPos.x = SCREEN_CENTER_X - 150.0f;
	g_yakataPos.y = SCREEN_CENTER_Y;

	// 幽霊初期位置（屋敷の右下）
	g_yureiPos.x = g_yakataPos.x + 100.0f;
	g_yureiPos.y = g_yakataPos.y + 80.0f;

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
	if (g_SolidTex) { g_SolidTex->Release(); g_SolidTex = nullptr; }

	g_pDevice = nullptr;
	g_pContext = nullptr;

	SetFPS(60);
}

void OpAnim_Update()
{
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

		// バスターフェードイン
		float fadeDuration = 2.0f;
		float fadeTime = g_elapsedTime - g_basutaMoveStartTime;
		g_basutaAlpha = fadeTime / fadeDuration;
		if (g_basutaAlpha > 1.0f) g_basutaAlpha = 1.0f;
	}

	// --- 幽霊更新 ---
	// バスターを見つけたら反応
	if (!g_yureiReacting && g_basutaAlpha > 0.5f)
	{
		float dx = g_basutaCurrentPos.x - g_yureiPos.x;
		float dy = g_basutaCurrentPos.y - g_yureiPos.y;
		float dist = sqrtf(dx * dx + dy * dy);

		if (dist < g_yureiReactDistance)
		{
			g_yureiReacting = true;
			g_yureiTimer = 0.0f;
		}
	}

	// 幽霊フェードイン（時間経過で自然に出現）
	if (g_elapsedTime >= g_yureiAppearTime)
	{
		float fadeTime = g_elapsedTime - g_yureiAppearTime;
		g_yureiAlpha = fadeTime / g_yureiFadeDuration;
		if (g_yureiAlpha > 1.0f) g_yureiAlpha = 1.0f;
	}

	// 幽霊が館に向かって移動
	if (g_yureiReacting)
	{
		g_yureiTimer += delta;
		float moveTime = 3.0f;
		float moveRatio = g_yureiTimer / moveTime;

		if (moveRatio >= 1.0f)
		{
			g_yureiPos = g_yureiTargetPos;
			moveRatio = 1.0f;
		}
		else
		{
			g_yureiPos.x = g_yureiPos.x + (g_yureiTargetPos.x - g_yureiPos.x) * moveRatio * delta * g_yureiMoveSpeed / 100.0f;
			g_yureiPos.y = g_yureiPos.y + (g_yureiTargetPos.y - g_yureiPos.y) * moveRatio * delta * g_yureiMoveSpeed / 100.0f;
		}

		// 幽霊フェードアウト（館へ入る）
		float fadeOutTime = moveTime - 0.5f;
		if (g_yureiTimer > fadeOutTime)
		{
			float outRatio = (g_yureiTimer - fadeOutTime) / 0.5f;
			g_yureiAlpha *= (1.0f - outRatio);
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

	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, screenWidth, screenHeight, 0.0f, 0.0f, 1.0f));
	SetBlendState(BLENDSTATE_ALFA);

	 //黒紫背景（
	if (g_TexKuromurasaki)
	{
		g_pContext->PSSetShaderResources(0, 1, &g_TexKuromurasaki);
		Sprite_Single_Draw(XMFLOAT2{ screenWidth * 0.5f, screenHeight * 0.5f },
			XMFLOAT2{ screenWidth, screenHeight }, 0.0f,
			XMFLOAT4{ 1,1,1,1 }, BLENDSTATE_ALFA, g_TexKuromurasaki);
	}

	// 屋敷
	if (g_TexYakata)
	{
		g_pContext->PSSetShaderResources(0, 1, &g_TexYakata);
		Sprite_Single_Draw(g_yakataPos, g_baseSize, 0.0f,
			XMFLOAT4{ 1,1,1,1 }, BLENDSTATE_ALFA, g_TexYakata);
	}

	// バスター
	if (g_TexBasuta && g_basutaAlpha > 0.0f)
	{
		g_pContext->PSSetShaderResources(0, 1, &g_TexBasuta);
		Sprite_Single_Draw(g_basutaCurrentPos, g_baseSize, 0.0f,
			XMFLOAT4{ 1,1,1,g_basutaAlpha }, BLENDSTATE_ALFA, g_TexBasuta);
	}

	// 幽霊
	if (g_TexYurei && g_yureiAlpha > 0.0f)
	{
		g_pContext->PSSetShaderResources(0, 1, &g_TexYurei);
		Sprite_Single_Draw(g_yureiPos, g_baseSize, 0.0f,
			XMFLOAT4{ 1,1,1,g_yureiAlpha }, BLENDSTATE_ALFA, g_TexYurei);
	}

	// 稲妻テクスチャ（画面右に常時表示、フラッシュ時に輝度UP）
	if (g_TexInazuma)
	{
		SetBlendState(BLENDSTATE_ADD);

		g_pContext->PSSetShaderResources(0, 1, &g_TexInazuma);
		float xPos = screenWidth - 100.0f;
		float yPos = screenHeight * 0.5f - 50.0f + sinf(g_elapsedTime * 0) * 30.0f;

		// フラッシュしていない時も薄く表示、フラッシュ時は明るく
		float inazumaAlpha = 1.15f + g_inazumaFlashAlpha * 0.65f;
		Sprite_Single_Draw(XMFLOAT2{ xPos, yPos }, g_inazumaSize, 0.0f,
			XMFLOAT4{ 0.9f, 0.9f, 1.0f, inazumaAlpha }, BLENDSTATE_ADD, g_TexInazuma);

		SetBlendState(BLENDSTATE_ALFA);
	}

	// 稲妻テクスチャ（画面右に表示）
	if (g_TexInazuma && g_inazumaFlashAlpha > 0.001f)
	{
		// 加算合成ブレンドを使用して、背景に光を加える
		SetBlendState(BLENDSTATE_ADD);

		g_pContext->PSSetShaderResources(0, 1, &g_TexInazuma);
		float xPos = screenWidth - 100.0f;
		float yPos = screenHeight * 0.5f - 50.0f + sinf(g_elapsedTime * 8.0f) * 30.0f;

		// より明るく、より目立つ色で描画
		Sprite_Single_Draw(XMFLOAT2{ xPos, yPos }, g_inazumaSize, 0.0f,
			XMFLOAT4{ 0.9f, 0.9f, 1.0f, g_inazumaFlashAlpha * 0.8f }, BLENDSTATE_ADD, g_TexInazuma);

		// ブレンドステートを戻す
		SetBlendState(BLENDSTATE_ALFA);
	}
}