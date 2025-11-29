#pragma once

#include <d3d11.h>
#include "direct3d.h"
#include "sprite.h"
#include <DirectXMath.h>
#include <windows.h>
using namespace DirectX;

#define CLOCK_POS_X (120.0f)
#define CLOCK_POS_Y (120.0f)
#define CLOCK_SIZE (300.0f)
#define CLOCK_MIN (0.0f)
#define CLOCK_MAX (60.0f)

class Timer : public SplitSprite
{
protected:
	float m_HandRot;
	float m_Time;	//経過時間
	float m_MinTime;
	float m_MaxTime;
	DWORD m_LastUpdateTime;  // 前回の更新時刻
	int m_UpdateInterval;    // 更新間隔（ミリ秒）
public:
	Timer(const XMFLOAT2& pos, const XMFLOAT2& size, const XMFLOAT4& col, BLENDSTATE bstate, const wchar_t* texturePath, int divideX, int divideY, float minT, float maxT, int updateIntervalMs = 100)
		: SplitSprite(pos, size, 0.0f, col, bstate, texturePath, divideX, divideY),
		m_HandRot(0.0f), m_Time(0.0f), m_MinTime(minT), m_MaxTime(maxT), m_LastUpdateTime(timeGetTime()), m_UpdateInterval(updateIntervalMs)
	{
	}

	bool Update()
	{
		bool isEnded = false;

		// 現在時刻を取得
		DWORD currentTime = timeGetTime();
		DWORD elapsedTime = currentTime - m_LastUpdateTime;

		// 指定されたミリ秒以上経過したら時間を加算
		if (elapsedTime >= static_cast<DWORD>(m_UpdateInterval))
		{
			m_Time += (float)elapsedTime / 1000.0f;  // ミリ秒から秒に変換
			m_LastUpdateTime = currentTime;
		}

		// 最大時間を超えないようにキャップ
		if (m_Time > m_MaxTime)
		{
			m_Time = m_MaxTime;
			isEnded = true;
		}

		// 0から1の正規化された値を計算
		float t = (m_Time - m_MinTime) / (m_MaxTime - m_MinTime);
		m_HandRot = t * 360.0f; // 360度回転

		return isEnded;
	}

	void Draw()
	{
		m_Rotation = 0.0f;
		m_TextureNumber = 0; // 時計本体のテクスチャ番号に変更
		SplitSprite::Draw(); // 時計本体
		m_Rotation = m_HandRot;
		m_TextureNumber = 1; // 針のテクスチャ番号に変更
		SplitSprite::Draw(); // 針
	}

	// ゲッター・セッター
	float GetTime() const { return m_Time; }
	void SetTime(float time) { m_Time = time; }
	void Reset() { m_Time = m_MinTime; m_LastUpdateTime = timeGetTime(); }
};

#define GAUGE_SIZE (500.0f)
#define GAUGE_BAR_PURPLE (1)
#define GAUGE_BAR_YELLOW (2)
#define GAUGE_BAR_FRAME (0)
#define GAUGE_OFFSET ((GAUGE_SIZE / 1000 ) * 160)

// Gaugeクラス テクスチャを伸縮して値を表現
class Gauge : public SplitSprite
{
protected:
	float m_Value;   // 現在の値
	float m_MaxValue;       // 最大値
	float m_MinValue;       // 最小値
public:
	Gauge(const XMFLOAT2& pos, const XMFLOAT2& size, const XMFLOAT4& col, BLENDSTATE bstate, const wchar_t* texturePath,
		int divideX, int divideY, float minValue, float maxValue, int gaugeTexNum = 1, int bgTexNum = 0)
		: SplitSprite(pos, size, 0.0f, col, bstate, texturePath, divideX, divideY),
		m_Value(minValue), m_MaxValue(maxValue), m_MinValue(minValue)
	{
	}

	void Draw()
	{
		//=================================================
		// バー背景描画（紫）
		//=================================================

		// 背景を描画（元々のサイズ）
		m_TextureNumber = GAUGE_BAR_PURPLE;
		SplitSprite::Draw();

		//=================================================
		// バー描画（黄色）
		//=================================================

		// ゲージバーを描画（左から右に増える）
		m_TextureNumber = GAUGE_BAR_YELLOW;
		float ratio = GetRatio(); // 0.0 ~ 1.0

		// 幅を計算（元のサイズの ratio 倍）
		float gaugeWidth = m_Scale.x * ratio;

		// スケール調整で幅を変更
		XMFLOAT2 originalScale = m_Scale;
		m_Scale.x = gaugeWidth;

		// 位置を調整（左端を基準にするため、左に移動）
		XMFLOAT2 originalPos = m_Position;
		m_Position.x = originalPos.x - (originalScale.x * 0.5f) + (gaugeWidth * 0.5f);
		m_Position.x += GAUGE_OFFSET * (1.0f - GetRatio()); // 微調整 
		SplitSprite::Draw();

		//=================================================
		// 枠描画
		//=================================================

		// 位置とスケールを元に戻す
		m_Position = originalPos;
		m_Scale = originalScale;

		// 枠を描画（元々のサイズ）
		m_TextureNumber = GAUGE_BAR_FRAME;
		SplitSprite::Draw();
	}

	// ゲッター・セッター
	float GetValue() const { return m_Value; }
	float GetMaxValue() const { return m_MaxValue; }
	float GetMinValue() const { return m_MinValue; }
	float GetRatio() const { return (m_Value - m_MinValue) / (m_MaxValue - m_MinValue); }
	void SetValue(float value) { m_Value = value; }
	void AddValue(float value)
	{
		if (m_Value + value < m_MinValue)
		{
			m_Value = m_MinValue;
		}
		else if (m_Value + value > m_MaxValue)
		{
			m_Value = m_MaxValue;
		}
		else
		{
			m_Value += value;
		}
	}
	void SetMaxValue(float maxValue) { m_MaxValue = maxValue; }
	void Reset() { m_Value = m_MaxValue; }
};

// UI初期化
void UI_Initialize(void);

// UI終了
void UI_Finalize(void);

// UI更新
void UI_Update(void);

// UI描画
void UI_Draw(void);
