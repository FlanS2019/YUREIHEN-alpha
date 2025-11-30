#include "furniture.h"
#include "Camera.h"
#include "shader.h"
#include "keyboard.h"

Furniture* g_Furniture[FURNITURE_NUM]{};

void Furniture_Initialize(void)
{
	g_Furniture[0] = new Furniture(
		{ 0.0f, 1.0f, 0.0f },			//位置
		{ 1.0f, 1.0f, 1.0f },			//スケール
		{ 0.0f, 0.0f, 0.0f },			//回転（度）
		"asset\\model\\rockingchair.fbx"		//モデルパス
	);
}

void Furniture_Update(void)
{
	// Rキーで色を赤に変更
	if (Keyboard_IsKeyDownTrigger(KK_R))
	{
	    g_Furniture[0]->SetColor(1.0f, 0.0f, 0.0f, 1.0f);  // 赤
		hal::dout << "Furniture color changed to Red" << std::endl;
	}

	// Tキーで色をリセット（元の色に戻す）
	if (Keyboard_IsKeyDownTrigger(KK_T))
	{
		g_Furniture[0]->ResetColor();
		hal::dout << "Furniture color reset to original" << std::endl;
	}

	//各家具処理
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i])
		{
			// 通常の更新
			g_Furniture[i]->Update();
		}
	}
}

void Furniture_Draw(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i])
		{
			g_Furniture[i]->Draw();
		}
	}
}

void Furniture_Finalize(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i])
		{
			delete g_Furniture[i];
			g_Furniture[i] = nullptr;
		}
	}
}

// ゲッター関数：indexの範囲をチェック
Furniture* GetFurniture(int index)
{
	if (index >= 0 && index < FURNITURE_NUM)
	{
		return g_Furniture[index];
	}
	return nullptr;
}

// 指定された家具をジャンプさせる関数
void FurnitureJump(int index)
{
	if (index >= 0 && index < FURNITURE_NUM && g_Furniture[index])
	{
		g_Furniture[index]->Jump();
		hal::dout << "Furniture[" << index << "] jumped!" << std::endl;
	}
	else
	{
		hal::dout << "Invalid furniture index: " << index << std::endl;
	}
}

