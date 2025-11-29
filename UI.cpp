//UI.cpp

#include "UI.h"
#include "sprite.h"
#include "debug_ostream.h"
#include "keyboard.h"

// グローバル変数
static Timer* g_Clock = nullptr;
static Gauge* g_ScareGauge = nullptr;
Sprite* g_Reticle = nullptr;
static Number* g_TimeScore = nullptr;
static DWORD g_LastScoreUpdateTime = 0;

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
		{ SCREEN_WIDTH - 270.0f, 70.0f },				// 位置
		{ GAUGE_SIZE, GAUGE_SIZE },						// サイズ
		{ 1.0f, 1.0f, 1.0f, 0.5f },						// 色
		BLENDSTATE_ALFA,								// BlendState
		L"asset\\texture\\gauge.png",					// テクスチャパス
		3, 1,											// 分割数X, Y
		0.0f, 100.0f,									// 最小値、最大値
		2, 0											// ゲージテクスチャ番号、背景テクスチャ番号
	);

	// 仮のレティクル
	g_Reticle = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },	//位置
		{ 30.0f, 30.0f },								//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\texture\\grass.png"					//テクスチャパス
	);

	// スコア表示用テクスチャの読み込み
	g_TimeScore = new Number(
		{ SCREEN_WIDTH - 110.0f, 170.0f },// 位置
		{ 70.0f, 70.0f },								// サイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },						// RGBA
		BLENDSTATE_ALFA,								// BlendState
		L"asset\\texture\\num.png",						// テクスチャパス
		5, 3,											// 分割数X, Y
		55.0f											// 桁間隔												
	);

	g_TimeScore->SetShowX(true); // 倍数接頭子「x」を表示

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

	// スコア更新処理
	DWORD currentTime = GetTickCount64();
	if (currentTime - g_LastScoreUpdateTime >= 1000) // 1秒ごとに更新
	{
		g_TimeScore->AddNumber(1); // スコアを加算
		g_LastScoreUpdateTime = currentTime;
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
	g_TimeScore->Draw(); // スコアを描画
}

//----------------------------
//UI終了
//----------------------------
void UI_Finalize(void)
{
	delete g_Clock;
	delete g_ScareGauge;
	delete g_Reticle;
	delete g_TimeScore;
}