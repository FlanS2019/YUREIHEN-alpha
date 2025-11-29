/*==============================================================================

   ポリゴン描画 [game.cpp]

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "main.h"
#include "shader.h"
#include "debug_ostream.h"
#include "game.h"
#include "field.h"
#include "texture.h"
#include "keyboard.h"
#include "scene.h"
#include "camera.h"
#include "modeldraw.h"
#include "sprite.h"
#include "UI.h"

Light* MainLight;

Sprite* g_TestSprite = nullptr;

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Game_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	MainLight = new Light
	(TRUE,
		XMFLOAT4(0.0f, -10.0f, -10.0f, 1.0f),	//向き
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//光の色
		XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)	//環境光
	);

	// ②各種初期化
	g_TestSprite = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },	//位置
		{ 30.0f, 30.0f },								//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 0.5f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\texture\\grass.png"					//テクスチャパス
	);

	Camera_Initialize();
	Field_Initialize(pDevice, pContext);
	ModelDraw_Initialize();
	UI_Initialize();
}

void Game_Update(void)
{
	Camera_Update();
	Field_Update();
	ModelDraw_Update();
	UI_Update();
}

void Game_Draw(void)
{
	MainLight->SetEnable(true);
	Shader_SetLight(MainLight);

	//3D描画なら常に有効にする
	SetDepthTest(true);

	Field_Draw();
	ModelDraw_Draw();

	SetDepthTest(false);
	MainLight->SetEnable(false);
	Shader_SetLight(MainLight);

	//2D描画処理ここから
	g_TestSprite->Draw();
	UI_Draw();
}

void Game_Finalize(void)
{
	delete MainLight;
	delete g_TestSprite;

	Camera_Finalize();
	Field_Finalize();
	ModelDraw_Finalize();
	UI_Finalize();
}