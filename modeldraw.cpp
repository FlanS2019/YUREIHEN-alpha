#include "modeldraw.h"
#include "Camera.h"
#include "shader.h"
#include "keyboard.h"
#include "sprite3d.h"

Sprite3D* g_ModelDraw = NULL;

void ModelDraw_Initialize(void)
{
	g_ModelDraw = new Sprite3D(
		{ 0.0f, 0.0f, 0.0f },			//位置
		{ 1.0f, 1.0f, 1.0f },			//スケール
		{ 0.0f, 0.0f, 0.0f },			//回転（度）
		"asset\\model\\ghost.fbx"		//モデルパス
	);

}

void ModelDraw_Update(void)
{

}

void ModelDraw_Draw(void)
{
	g_ModelDraw->Draw();
}

void ModelDraw_Finalize(void)
{
	delete g_ModelDraw;
}

