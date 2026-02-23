#pragma execution_character_set("utf-8")
#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
using namespace DirectX;
#include "shader.h"
#include "debug_ostream.h"
#include "game.h"
#include "field.h"
#include "texture.h"
#include "keyboard.h"
#include "scene.h"
#include "camera.h"
#include "sprite.h"
#include "UI.h"
#include "UI_PauseMenu.h"
#include "UI_Tutorial.h"
#include "ghost.h"
#include "furniture.h"
#include "fade.h"
#include "busters.h"
#include "Tutorial_Object.h"
#include "debugdraw.h"
#include "sound.h"
#include "minimap.h"
#include "mouse.h"
#include "light.h"
#include "model.h"
#include <windows.h>
#include <string>
#include <cmath>

static AmbientLight* g_pAmbientLight = nullptr;
static SoundData* g_pBGM = nullptr;

static int g_NextFloorID = -1;

bool* Game_GetEnbanTouchedPtr(void)
{
	return TutorialObject_GetEnbanTouchedPtr();
}

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pAmbientLight = new AmbientLight(XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));

	Camera_Initialize();
	Ghost_Initialize(pDevice, pContext);
	Field_Initialize(pDevice, pContext);
	UI_Initialize();
	Furniture_Initialize();
	Busters_Initialize();
	Minimap_Initialize();
	DebugDraw_Initialize();

	TutorialObject_Initialize();

	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) PlaySound(g_pBGM, true);

	UI_PauseMenu_Initialize(pDevice, pContext);
	UI_Tutorial_Initialize(pDevice, pContext);
}

void Game_Update(void)
{
	FADESTAT fadeState = GetFadeState();
	if (fadeState == FADE_MAX)
	{
		if (g_NextFloorID != -1) {
			LoadMapData(g_NextFloorID);
			GetGhost()->SetPos(XMFLOAT3(0, 0, 0));
			g_NextFloorID = -1;
			Fade_StartIn();
		}
		return;
	}
	if (fadeState != FADE_NONE) return;

	UI_PauseMenu_Update();
	if (UI_PauseMenu_IsPaused())
	{
		return;
	}

	if (Field_GetCurrentFloor() == 2)
	{
		UI_Tutorial_Update();
		TutorialObject_Update();


		if (UI_Tutorial_IsWaiting())
		{
			Camera_Update();
			Shader_SetCameraPos(GetCamera()->GetPos());
			Field_Update();
			Ghost_Update();
			Furniture_Update();
			return;
		}

		if (UI_Tutorial_IsActive())
		{
			return;
		}
	}

	Camera_Update();
	Shader_SetCameraPos(GetCamera()->GetPos());
	Field_Update();
	UI_Update();
	Furniture_Update();
	Ghost_Update();
#if !STOP_TIMER_BUSTER
	Busters_Update();
#endif

	DebugDraw_Update();
}

void Game_Draw(void)
{
	SetDepthTest(true);

	Shader_SetAmbientLight(g_pAmbientLight);
	Shader_ClearPointLights();
	Ghost_SetLight();
	Busters_SetLight();

	Field_Draw();
	Busters_Draw();

	if (Field_GetCurrentFloor() == 2)
	{
		TutorialObject_Draw();
	}

	Furniture_Draw();
	Ghost_Draw();
	DebugDraw_Draw();

	SetDepthTest(false);

	Sprite_BeginDraw2D();
	UI_Draw();
	Minimap_Draw();

	if (Field_GetCurrentFloor() == 2)
	{
		UI_Tutorial_Draw();
	}
	UI_PauseMenu_Draw();

	Sprite_EndDraw2D();
}

void Game_Finalize(void)
{
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}

	if (g_pAmbientLight) {
		delete g_pAmbientLight;
		g_pAmbientLight = nullptr;
	}

	UI_PauseMenu_Finalize();
	UI_Tutorial_Finalize();

	Camera_Finalize();
	Ghost_Finalize();
	Field_Finalize();
	UI_Finalize();
	Furniture_Finalize();
	Minimap_Finalize();
	Busters_Finalize();
	DebugDraw_Finalize();

	TutorialObject_Finalize();
}