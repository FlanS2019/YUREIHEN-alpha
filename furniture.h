#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "sprite3d.h"

using namespace DirectX;

// Furniture定数
#define FURNITURE_NUM (1)
#define FURNITURE_DETECTION_RANGE (5.0f) // Ghost検出範囲

// Furniture クラス - 色を変更できる
class Furniture : public Sprite3D
{
private:
	XMFLOAT4 m_Color;           // 現在の色（R, G, B, A）
	XMFLOAT4 m_OriginalColor;   // 元の色（リセット用）
	bool m_UseOriginalColor;    // 元の色を使用するかどうかのフラグ

	// ジャンプ用メンバ
	bool m_IsJumping;           // ジャンプ中フラグ
	float m_JumpVelocityY;      // Y方向の速度
	const float m_Gravity;      // 重力加速度
	const float m_JumpPower;    // ジャンプ力

public:
	Furniture(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
		: Sprite3D(pos, scale, rot, pass), m_Color(1.0f, 1.0f, 1.0f, 1.0f), 
		  m_OriginalColor(ModelGetAverageMaterialColor(m_Model)), m_UseOriginalColor(true),
		  m_IsJumping(false), m_JumpVelocityY(0.0f), m_Gravity(0.01f), m_JumpPower(0.2f)
	{
	}

	~Furniture() = default;

	// 色を設定
	void SetColor(const XMFLOAT4& color)
	{
		m_Color = color;
		m_UseOriginalColor = false;  // カスタム色を使用中
	}

	// 色を設定（R, G, B, A）
	void SetColor(float r, float g, float b, float a = 1.0f)
	{
		m_Color = XMFLOAT4(r, g, b, a);
		m_UseOriginalColor = false;  // カスタム色を使用中
	}

	// 色を取得
	XMFLOAT4 GetColor(void) const
	{
		return m_Color;
	}

	// 元の色を設定（初期化時に呼び出す）
	void SetOriginalColor(const XMFLOAT4& color)
	{
		m_OriginalColor = color;
	}

	// 元の色を設定（R, G, B, A）
	void SetOriginalColor(float r, float g, float b, float a = 1.0f)
	{
		m_OriginalColor = XMFLOAT4(r, g, b, a);
	}

	// 色をリセット（元の色に戻す）
	void ResetColor(void)
	{
		m_UseOriginalColor = true;
	}

	// 色を変更するメソッド（R, G, B, A 個別指定）
	void SetColorRed(float r) { m_Color.x = r; m_UseOriginalColor = false; }
	void SetColorGreen(float g) { m_Color.y = g; m_UseOriginalColor = false; }
	void SetColorBlue(float b) { m_Color.z = b; m_UseOriginalColor = false; }
	void SetColorAlpha(float a) { m_Color.w = a; m_UseOriginalColor = false; }

	// 色を取得するメソッド（R, G, B, A 個別取得）
	float GetColorRed(void) const { return m_Color.x; }
	float GetColorGreen(void) const { return m_Color.y; }
	float GetColorBlue(void) const { return m_Color.z; }
	float GetColorAlpha(void) const { return m_Color.w; }

	// ジャンプメソッド
	void Jump(void)
	{
		if (!m_IsJumping)
		{
			m_IsJumping = true;
			m_JumpVelocityY = m_JumpPower;
		}
	}

	// ジャンプ状態の更新（Update内で呼ぶ）
	void Update(void)
	{
		if (m_IsJumping)
		{
			// Y位置を更新
			XMFLOAT3 pos = GetPos();
			pos.y += m_JumpVelocityY;
			SetPos(pos);

			// 速度に重力を適用
			m_JumpVelocityY -= m_Gravity;

			// 地面に着地したかチェック（Y <= 初期位置）
			if (pos.y <= 1.0f)  // 初期位置がY=1.0fと仮定
			{
				pos.y = 1.0f;
				SetPos(pos);
				m_IsJumping = false;
				m_JumpVelocityY = 0.0f;
			}
		}
	}

	// Draw メソッドをオーバーライドして、Furniture の色を反映
	void Draw(void) override
	{
		if (m_Model)
		{
			// 使用する色を決定
			XMFLOAT4 drawColor = m_UseOriginalColor ? m_OriginalColor : m_Color;

			// モデル描画時に Furniture の色を渡す（色置き換えモード）
			ModelDraw(
				m_Model,
				GetPos(),
				GetRot(),
				GetScale(),
				drawColor,    // 現在の色または元の色を使用
				true          // useColorReplace = true（色置き換えモード）
			);
		}
		else
		{
			hal::dout << "Furniture::Draw() : モデルが読み込まれていません。" << std::endl;
		}
	}
};

void Furniture_Initialize(void);
void Furniture_Update(void);
void Furniture_Draw(void);
void Furniture_Finalize(void);

// ゲッター関数
Furniture* GetFurniture(int index);

// ジャンプ関数
void FurnitureJump(int index);