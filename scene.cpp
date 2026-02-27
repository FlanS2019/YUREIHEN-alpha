#include "scene.h"
#include "game.h"
#include "animation.h"
#include "WinAnim.h"
#include "LoseAnim.h"
#include "LoseAnimED.h"
#include "direct3d.h"
#include "keyboard.h"
#include "texture.h"
#include "title.h"
#include "result.h"
#include "define.h"
#include "debug_model_scene.h"
using namespace DirectX;

//DIRECT_STARTがtrueの場合、最初からゲームシーンにする
#if DEBUG_MODEL_SCENE
static SCENE scene = SCENE_DEBUG_MODEL;
#elif DIRECT_START
static SCENE scene = SCENE_GAME;
#else
static SCENE scene = SCENE_ANM_LOGO;
#endif

void Init(void)
{
	switch (scene)
	{	
	case SCENE_TITLE:
		Title_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_GAME:
		Game_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_RESULT:
		Result_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;	
	case SCENE_ANM_LOGO:
		Animation_Logo_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_ANM_OP:
		Animation_Op_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_ANM_WIN:
		Animation_Win_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_ANM_LOSE:
		Animation_Lose_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_ANM_LOSE_ED:
		Animation_LoseED_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_DEBUG_MODEL:
		DebugModelScene_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	default:
		break;
	}
}

void Update(void)
{
	switch (scene)
	{
	case SCENE_TITLE:
		Title_Update();
		break;
	case SCENE_GAME:
		Game_Update();
		break;
	case SCENE_RESULT:
		Result_Update();
		break;
	case SCENE_ANM_LOGO:
		Animation_Logo_Update();
		break;
	case SCENE_ANM_OP:
		Animation_Op_Update();
		break;
	case SCENE_ANM_WIN:
		Animation_Win_Update();
		break;
	case SCENE_ANM_LOSE:
		Animation_Lose_Update();
		break;
	case SCENE_ANM_LOSE_ED:
		Animation_LoseED_Update();
		break;
	case SCENE_DEBUG_MODEL:
		DebugModelScene_Update();
		break;
	default:
		break;
	}
}

void Draw(void)
{
	switch (scene)
	{
	case SCENE_TITLE:
		Title_Draw();
		break;
	case SCENE_GAME:
		Game_Draw();
		break;
	case SCENE_RESULT:
		Result_Draw();
		break;
	case SCENE_ANM_LOGO:
		Animation_Logo_Draw();
		break;
	case SCENE_ANM_OP:
		Animation_Op_Draw();
		break;
	case SCENE_ANM_WIN:
		Animation_Win_Draw();
		break;
	case SCENE_ANM_LOSE:
		Animation_Lose_Draw();
		break;
	case SCENE_ANM_LOSE_ED:
		Animation_LoseED_Draw();
		break;
	case SCENE_DEBUG_MODEL:
		DebugModelScene_Draw();
		break;
	default:
		break;
	}
}

void Finalize(void)
{
	switch (scene)
	{
	case SCENE_TITLE:
		Title_Finalize();
		break;
	case SCENE_GAME:
		Game_Finalize();
		break;
	case SCENE_RESULT:
		Result_Finalize();
		break;
	case SCENE_ANM_LOGO:
		Animation_Logo_Finalize();
		break;
	case SCENE_ANM_OP:
		Animation_Op_Finalize();
		break;
	case SCENE_ANM_WIN:
		Animation_Win_Finalize();
		break;
	case SCENE_ANM_LOSE:
		Animation_Lose_Finalize();
		break;
	case SCENE_ANM_LOSE_ED:
		Animation_LoseED_Finalize();
		break;
	case SCENE_DEBUG_MODEL:
		DebugModelScene_Finalize();
		break;
	default:
		break;
	}
}

void SetScene(SCENE id)
{
	Finalize();

	scene = id;

	Init();
}

SCENE GetScene(void)
{
	return scene;
}
