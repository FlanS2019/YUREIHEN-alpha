#include "result.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "font.h"
#include "UI.h"
#include "sound.h"
#include <sstream>
#include <iomanip>
using namespace DirectX;

// ①Spriteのインスタンス、ポインタ用意
static Sprite* g_pResultSprite = nullptr;
static Sprite* g_pPlus = nullptr;
static Sprite* g_pEqual = nullptr;
static Sprite* g_pkyou1 = nullptr;
static Sprite* g_pkyou2 = nullptr;
static Sprite* g_pkyou3 = nullptr;
// 数字表示用のNumberクラスに変更
static Number* g_pTimeNum = nullptr;
//static Number* g_pFloorNum = nullptr;
static Number* g_pComboNum = nullptr;  // 連鎖数表示用
static Number* g_pResultNum = nullptr;  // 結果スコア表示用（時間×コンボ）
// ラベル用フォント（「Time:」「Floor:」のテキスト部分）
static FontRenderer* g_pTimeLabelFont = nullptr;
static FontRenderer* g_pFloorLabelFont = nullptr;
static FontRenderer* g_pComboLabelFont = nullptr;  // 連鎖ラベル用
static float g_pResultTime = 0.0f;
static int g_pResultFloor = 1;
static int g_pResultCombo = 1;  // 連鎖数
//sound
static SoundData* g_pBGM = nullptr;

// スコアに応じた画像表示用ヘルパー関数
static int GetResultScore(void)
{
	return static_cast<int>(g_pResultTime) * g_pResultCombo;
}

void Result_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	hal::dout << "Result Initialize Called" << std::endl;
	// ②各種初期化
	g_pResultSprite = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },		//位置
		{ SCREEN_WIDTH , SCREEN_HEIGHT },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_NONE,								//BlendState
		L"asset\\yureihen\\Alpha_Tex\\siro.png"					//テクスチャパス
	);

	g_pPlus = new Sprite(
		{ SCREEN_WIDTH / 2.0f - 500, SCREEN_HEIGHT / 2.0f - 25 },		//位置
		{ 130.0f , 130.0f },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\yureihen\\kakeru.png"					//テクスチャパス
	);

	g_pEqual = new Sprite(
		{ SCREEN_WIDTH / 2.0f - 400.0f, SCREEN_HEIGHT / 2.0f - 25 },		//位置
		{ 130.0f , 130.0f },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\yureihen\\equal.png"					//テクスチャパス
	);

	// グローバルフォントデータを初期化
	Font_InitializeGlobalData();

	// タイム表示用数字スプライト
	// num.pngが0-9の数字を横に並べた画像と仮定（横10分割、縦1分割）
	g_pTimeNum = new Number(
		{ SCREEN_WIDTH / 2.0f - 340.0f, SCREEN_HEIGHT / 2.0f - 100.0f },	// 位置
		{ 60.0f, 60.0f },	// 1桁のサイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,	// 横10分割、縦1分割（0-9の数字）
		45.0f	// 桁間の間隔
	);

	//// 階層表示用数字スプライト
	//g_pFloorNum = new Number(
	//	{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },	// 位置
	//	{ 60.0f,60.0f },	// 1桁のサイズ
	//	{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
	//	BLENDSTATE_ALFA,
	//	L"asset\\texture\\num.png",
	//	5, 3,	// 横10分割、縦1分割
	//	45.0f	// 桁間の間隔
	//);

	// 連鎖(コンボ)表示用数字スプライト（x付き）
	g_pComboNum = new Number(
		{ SCREEN_WIDTH / 2.0f - 350, SCREEN_HEIGHT / 2.0f + 40 },	// 位置
		{ 60.0f, 60.0f },	// 1桁のサイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,// 横10分割、縦1分割（0-9の数字）
		45.0f
	);
	g_pComboNum->SetShowX(true);  // 「x」を表示

	// 結果スコア表示用数字スプライト（equal.pngの右側）
	g_pResultNum = new Number(
		{ SCREEN_WIDTH / 2.0f - 270.0f, SCREEN_HEIGHT / 2.0f - 25.0f },	// equal.pngの右側
		{ 60.0f, 60.0f },	// 1桁のサイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		45.0f
	);

	// タイマーラベル表示用フォント
	g_pTimeLabelFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f - 500.0f, SCREEN_HEIGHT / 2.0f - 100.0f },	// 位置
		60.0f,		// フォントサイズ
		0.0f,		// 回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		"Time:"
	);

	//// 階層ラベル表示用フォント
	//g_pFloorLabelFont = new FontRenderer(
	//	{ SCREEN_WIDTH / 2.0f - 100.0f, SCREEN_HEIGHT / 2.0f },	// 位置
	//	60.0f,		// フォントサイズ
	//	0.0f,		// 回転
	//	{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
	//	"Floor:"
	//);

	// 連鎖ラベル表示用フォント
	g_pComboLabelFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f - 500.f, SCREEN_HEIGHT / 2.0f + 50 },	// 位置
		60.0f,		// フォントサイズ
		0.0f,		// 回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		"Combo:"
	);

	// 凶、恐、虚の絵
	g_pkyou1 = new Sprite(//凶
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },		//位置
		{ 500,500 },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\yureihen\\kyou1.png"					//テクスチャパス
	);
	g_pkyou2 = new Sprite(//恐
		{ SCREEN_WIDTH / 2.0f + 140, SCREEN_HEIGHT / 2.0f },		//位置
		{ 500,500 },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\yureihen\\kyou2.png"					//テクスチャパス
	);
	g_pkyou3 = new Sprite(//虚
		{ SCREEN_WIDTH / 2.0f + 240, SCREEN_HEIGHT / 2.0f },		//位置
		{ 500,500 },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\yureihen\\kyou3.png"					//テクスチャパス
	);

	// サウンド再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM)
	{
		PlaySound(g_pBGM, true);
	}

	// ③既にセット済みの値を反映
	if (g_pTimeNum)
	{
		g_pTimeNum->SetNumber(static_cast<int>(g_pResultTime));
	}
	//// 既にセット済みの値を反映
	//if (g_pResultTime > 0.0f && g_pTimeNum)
	//{
	//	g_pTimeNum->SetNumber(static_cast<int>(g_pResultTime));
	//}

	//if (g_pResultFloor > 0 && g_pFloorNum)
	//{
	//	g_pFloorNum->SetNumber(g_pResultFloor);
	//}

	if (g_pResultCombo > 0 && g_pComboNum)
	{
		g_pComboNum->SetNumber(g_pResultCombo);
	}

	// 結果スコア（時間×コンボ）を計算して設定
	if (g_pResultNum)
	{
		int resultScore = GetResultScore();
		g_pResultNum->SetNumber(resultScore);
	}
}

void Result_Update(void)
{
	// ③適当な処理　アニメーションなどもここで
	if (Keyboard_IsKeyDown(KK_SPACE))
	{
		StartFade(SCENE_TITLE);
	}
}

void Result_Draw(void)
{
	// ④Drawするだけでいい！！！！！！！
	g_pResultSprite->Draw();
	g_pPlus->Draw();
	g_pEqual->Draw();

	// スコアに応じた画像を表示（0-200：凶、201-400：恐、401-600：虚）
	int resultScore = GetResultScore();
	if (resultScore >= 0 && resultScore <= 200)
	{
		if (g_pkyou1)
			g_pkyou1->Draw();
	}
	else if (resultScore >= 201 && resultScore <= 400)
	{
		if (g_pkyou2)
			g_pkyou2->Draw();
	}
	else if (resultScore >= 401 && resultScore <= 600)
	{
		if (g_pkyou3)
			g_pkyou3->Draw();
	}

	// タイマーラベルを描画
	if (g_pTimeLabelFont)
	{
		g_pTimeLabelFont->Draw();
	}

	// タイマー数値を描画
	if (g_pTimeNum)
	{
		g_pTimeNum->Draw();
	}

	// 階層ラベルを描画
	if (g_pFloorLabelFont)
	{
		g_pFloorLabelFont->Draw();
	}

	//// 階層数値を描画
	//if (g_pFloorNum)
	//{
	//	g_pFloorNum->Draw();
	//}

	// 連鎖ラベルを描画
	if (g_pComboLabelFont)
	{
		g_pComboLabelFont->Draw();
	}

	// 連鎖数値を描画
	if (g_pComboNum)
	{
		g_pComboNum->Draw();
	}

	// 結果スコアを描画
	if (g_pResultNum)
	{
		g_pResultNum->Draw();
	}
}

void Result_Finalize(void)
{
	if (g_pResultSprite) {
		delete g_pResultSprite;
		g_pResultSprite = nullptr;
	}

	if (g_pTimeLabelFont) {
		delete g_pTimeLabelFont;
		g_pTimeLabelFont = nullptr;
	}

	if (g_pPlus) {
		delete g_pPlus;
		g_pPlus = nullptr;
	}

	if (g_pEqual) {
		delete g_pEqual;
		g_pEqual = nullptr;
	}

	if (g_pkyou1) {
		delete g_pkyou1;
		g_pkyou1 = nullptr;
	}
	if (g_pkyou2) {
		delete g_pkyou2;
		g_pkyou2 = nullptr;
	}

	if (g_pkyou3) {
		delete g_pkyou3;
		g_pkyou3 = nullptr;
	}

	if (g_pTimeNum) {
		delete g_pTimeNum;
		g_pTimeNum = nullptr;
	}

	if (g_pFloorLabelFont) {
		delete g_pFloorLabelFont;
		g_pFloorLabelFont = nullptr;
	}

	//if (g_pFloorNum) {
	//	delete g_pFloorNum;
	//	g_pFloorNum = nullptr;
	//}

	if (g_pComboLabelFont) {
		delete g_pComboLabelFont;
		g_pComboLabelFont = nullptr;
	}

	if (g_pComboNum) {
		delete g_pComboNum;
		g_pComboNum = nullptr;
	}

	if (g_pResultNum) {
		delete g_pResultNum;
		g_pResultNum = nullptr;
	}
	// BGM解放
	if (g_pBGM)
	{
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}


	Font_FinalizeGlobalData();
}

// タイマー結果をセット
void Result_SetTimerValue(float time)
{
	g_pResultTime = time;

	if (g_pTimeNum)
	{
		g_pTimeNum->SetNumber(static_cast<int>(time));
	}

	// 時間が更新されたら、結果スコアを再計算
	if (g_pResultNum)
	{
		int resultScore = GetResultScore();
		g_pResultNum->SetNumber(resultScore);
	}
}

//// 階層をセット
//void Result_SetFloor(int floor)
//{
//	g_pResultFloor = floor;
//
//	if (g_pFloorNum)
//	{
//		g_pFloorNum->SetNumber(floor);
//	}
//}

// 連鎖数をセット
void Result_SetCombo(int combo)
{
	g_pResultCombo = combo;

	if (g_pComboNum)
	{
		g_pComboNum->SetNumber(combo);
	}

	// コンボが更新されたら、結果スコアを再計算
	if (g_pResultNum)
	{
		int resultScore = GetResultScore();
		g_pResultNum->SetNumber(resultScore);
	}
}