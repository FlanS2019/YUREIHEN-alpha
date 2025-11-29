//UI.cpp

#include "UI.h"
#include "sprite.h"
#include "debug_ostream.h"
#include "keyboard.h"

// グローバル変数
static Timer* g_Clock = nullptr;
static Gauge* g_ScareGauge = nullptr;
Sprite* g_Reticle = nullptr;

//----------------------------
//UI初期化
//----------------------------
void UI_Initialize(void)
{
	// 時計の作成
	g_Clock = new Timer(
		{ CLOCK_POS_X, CLOCK_POS_Y },	// 位置
		{ CLOCK_SIZE, CLOCK_SIZE },		// サイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },			// 色
		BLENDSTATE_ALFA,				// BlendState
		L"asset\\texture\\clock.png",	// テクスチャパス
		2, 1,							// 分割数X, Y
		CLOCK_MIN, CLOCK_MAX			// 最小時間、最大時間
	);

	// 恐怖ゲージの作成
	g_ScareGauge = new Gauge(
		{ SCREEN_WIDTH - 270.0f, 70.0f },	// 位置
		{ GAUGE_SIZE, GAUGE_SIZE },			// サイズ
		{ 1.0f, 1.0f, 1.0f, 0.5f },			// 色
		BLENDSTATE_ALFA,					// BlendState
		L"asset\\texture\\gauge.png",		// テクスチャパス
		3, 1,								// 分割数X, Y
		0.0f, 100.0f,						// 最小値、最大値
		2, 0								// ゲージテクスチャ番号、背景テクスチャ番号
	);

	// 仮のレティクル
	g_Reticle = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },	//位置
		{ 30.0f, 30.0f },								//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 0.5f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\texture\\grass.png"					//テクスチャパス
	);

}

//----------------------------
//UI更新
//----------------------------
void UI_Update(void)
{
	// 時計の時間を更新
	if (g_Clock->Update())
	{
		hal::dout << "タイマーが終了しました" << std::endl;
	}

	//qキーでリセット
	if (Keyboard_IsKeyDownTrigger(KK_Q))
	{
		g_Clock->Reset();
		hal::dout << "Qキーでタイマーをリセットしました" << std::endl;
	}



	// テスト用：左矢印キーでゲージを減らす
	if (Keyboard_IsKeyDownTrigger(KK_LEFT))
	{
		g_ScareGauge->AddValue(-10.0f);
		hal::dout << "ゲージ値: " << g_ScareGauge->GetValue() << std::endl;
	}

	// テスト用：右矢印キーでゲージを増やす
	if (Keyboard_IsKeyDownTrigger(KK_RIGHT))
	{
		g_ScareGauge->AddValue(10.0f);
		hal::dout << "ゲージ値: " << g_ScareGauge->GetValue() << std::endl;
	}
}

//----------------------------
//UI描画
//----------------------------
void UI_Draw(void)
{
	g_Clock->Draw(); // 時計を描画
	g_ScareGauge->Draw(); // ゲージを描画
	g_Reticle->Draw();
}

//----------------------------
//UI終了
//----------------------------
void UI_Finalize(void)
{
	delete g_Clock;
	delete g_ScareGauge;
	delete g_Reticle;
}