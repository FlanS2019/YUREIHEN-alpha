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
static Sprite* g_pkyou1 = nullptr;
static Sprite* g_pkyou2 = nullptr;
static Sprite* g_pkyou3 = nullptr;
static Sprite* g_pResult_gakubuti = nullptr;
// 数字表示用のNumberクラスに変更
static Number* g_pTimeNum = nullptr;
static Number* g_pComboNum = nullptr;  // 連鎖数表示用
static Number* g_pResultNum = nullptr;  // 結果スコア表示用（時間×コンボ）
// ラベル用フォント（「Time:」「Combo:」のテキスト部分）
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

// 2桁の場合のみ末尾に0を追加して表示用数値を返す
static int GetDisplayTime(float time)
{
	int t = static_cast<int>(time);
	if (t >= 10 && t <= 99)
	{
		return t * 10;
	}
	return t;
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

	g_pResult_gakubuti = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 20 },		//位置
		{ 795, 795 },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\yureihen\\Result\\Result_gakubuti.png"					//テクスチャパス
	);

	// グローバルフォントデータを初期化
	Font_InitializeGlobalData();

	// -------------------------------------------------------
	// Normal画像（凶・恐・虚）の下にTime/Comboを並べる
	// Normal画像の下端を基準にする
	// Normal画像位置: 中央右寄り (SCREEN_WIDTH/2.0f + 10, SCREEN_HEIGHT/2.0f)
	// Normal下端: SCREEN_HEIGHT/2.0f + 100（サイズ200の半分）
	// -------------------------------------------------------

	// タイム表示用数字スプライト（Normalの下・左列）
	g_pTimeNum = new Number(
		{ SCREEN_WIDTH / 2.0f + 110.0f, SCREEN_HEIGHT / 2.0f + 135.0f },	// 位置
		{ 35.0f, 35.0f },	// 1桁のサイズ（縮小）
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,	// 横10分割、縦1分割（0-9の数字）
		28.0f,	// 桁間の間隔
		2		// 最小2桁表示
	);

	// 連鎖(コンボ)表示用数字スプライト（Timeの下）
	g_pComboNum = new Number(
		{ SCREEN_WIDTH / 2.0f + 110.0f, SCREEN_HEIGHT / 2.0f + 180.0f },	// 位置
		{ 35.0f, 35.0f },	// 1桁のサイズ（縮小）
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,	// 横10分割、縦1分割（0-9の数字）
		28.0f
	);
	g_pComboNum->SetShowX(true);  // 「x」を表示

	// 結果スコア表示用数字スプライト（Scoreの二本線の上・中央）
	g_pResultNum = new Number(
		{ SCREEN_WIDTH / 2.0f + 10.0f, SCREEN_HEIGHT / 2.0f + 230.0f },	// 二本線付近
		{ 50.0f, 50.0f },	// 1桁のサイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		40.0f
	);

	// タイマーラベル表示用フォント（Normalの下・左）
	g_pTimeLabelFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f - 50.0f, SCREEN_HEIGHT / 2.0f + 130.0f },	// 位置
		35.0f,		// フォントサイズ（縮小）
		0.0f,		// 回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		"Time:"
	);

	// 連鎖ラベル表示用フォント（Timeの下）
	g_pComboLabelFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f - 50.0f, SCREEN_HEIGHT / 2.0f + 175.0f },	// 位置
		35.0f,		// フォントサイズ（縮小）
		0.0f,		// 回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// 色（白）
		"Combo:"
	);

	// 凶、恐、虚の絵（位置は変更なし）
	g_pkyou1 = new Sprite(//凶 低いスコア0-200
		{ SCREEN_WIDTH / 2.0f + 10, SCREEN_HEIGHT / 2.0f },		//位置
		{ 200, 200 },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\yureihen\\Result\\Result_Normal.png"					//テクスチャパス
	);
	g_pkyou2 = new Sprite(//恐 中くらいスコア201-400
		{ SCREEN_WIDTH / 2.0f + 10, SCREEN_HEIGHT / 2.0f },		//位置
		{ 200, 200 },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\yureihen\\Result\\Result_Good.png"					//テクスチャパス
	);
	g_pkyou3 = new Sprite(//虚 401-600 高いスコア
		{ SCREEN_WIDTH / 2.0f + 10, SCREEN_HEIGHT / 2.0f },		//位置
		{ 500, 500 },	//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\yureihen\\Result\\Result_Excellent.png"					//テクスチャパス
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
		g_pTimeNum->SetNumber(GetDisplayTime(g_pResultTime));
	}

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
	g_pResult_gakubuti->Draw();

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

	// 結果スコアを描画（二本線の位置）
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

	if (g_pResult_gakubuti) {
		delete g_pResult_gakubuti;
		g_pResult_gakubuti = nullptr;
	}

	if (g_pTimeNum) {
		delete g_pTimeNum;
		g_pTimeNum = nullptr;
	}

	if (g_pFloorLabelFont) {
		delete g_pFloorLabelFont;
		g_pFloorLabelFont = nullptr;
	}

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
		g_pTimeNum->SetNumber(GetDisplayTime(time));
	}

	// 時間が更新されたら、結果スコアを再計算
	if (g_pResultNum)
	{
		int resultScore = GetResultScore();
		g_pResultNum->SetNumber(resultScore);
	}
}

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