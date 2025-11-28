#include "modeldraw.h"
#include "Camera.h"
#include "shader.h"
#include "keyboard.h"
#include "sprite3d.h"

// ①モデルを入れる変数を宣言
Sprite3D* g_Ghost = NULL;

void ModelDraw_Initialize(void)
{
	// ②変数を初期化
	g_Ghost = new Sprite3D(
		{ 0.0f, 2.0f, 0.0f },			//位置
		{ 1.0f, 1.0f, 1.0f },			//スケール
		{ 0.0f, 0.0f, 0.0f },			//回転（度）
		"asset\\model\\ghost.fbx"		//モデルパス
	);
}

void ModelDraw_Update(void)
{
	// ③処理
	// スペースキーが押されたらy軸回転
	if (Keyboard_IsKeyDown(KK_I))
	{
		g_Ghost->AddRotY(1.0f);
		if (g_Ghost->GetRotY() >= 360.0f)
		{
			g_Ghost->SetRotY(0.0f);
		}
	}
}

void ModelDraw_Draw(void)
{
	// ④モデル描画
	g_Ghost->Draw();
}

void ModelDraw_Finalize(void)
{
	// ⑤終了処理
	delete g_Ghost;
}

