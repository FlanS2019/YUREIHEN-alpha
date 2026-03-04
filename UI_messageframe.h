#pragma once
#include <d3d11.h>
#include "direct3d.h"
#include "sprite.h"
#include <DirectXMath.h>
#include <windows.h>
#include <vector>
using namespace DirectX;

#define FRAME_DEVIDE_X (4)
#define FRAME_DEVIDE_Y (1)
#define FRAME_LEFTUP_ROT (0.0f)
#define FRAME_RIGHTUP_ROT (90.0f)
#define FRAME_RIGHTBOTTOM_ROT (180.0f)
#define FRAME_LEFTBOTTOM_ROT (-90.0f)
#define FRAME_PATTAN_SIZE (100.0f)	//1マス当たりのサイズ
#define FRAME_DIGIT_PATTERN (4)	//何文字につき1マス増やすか

class MessageFrame : public SplitSprite
{
protected:
	bool m_Use;
	int m_DigitNum; // 表示する数値の桁数
	int m_PatternWidth; // 横棒を何回伸ばすか
	int m_OffsetX;
	XMFLOAT2 m_FramePos;
	Sprite* m_BG;
public:
	MessageFrame() = delete;
	MessageFrame(const XMFLOAT2& framepos, const wchar_t* texturePath)
		: SplitSprite(framepos, { FRAME_PATTAN_SIZE, FRAME_PATTAN_SIZE }, 0.0f, { 1.0f,1.0f,1.0f,1.0f }, BLENDSTATE_ALFA, texturePath, FRAME_DEVIDE_X, FRAME_DEVIDE_Y), m_DigitNum(), m_PatternWidth(), m_OffsetX(), m_Use(false), m_FramePos(framepos)
	{
		m_BG = new Sprite(framepos, {}, 0, { 0,0,0,0.5f }, BLENDSTATE_ALFA, L"asset\\texture\\fade.png");
	}

	void ShowFrame(int digit)
	{
		m_Use = true;
		m_DigitNum = digit;
		m_PatternWidth = (digit - 1) / FRAME_DIGIT_PATTERN + 1; // 1マスあたりの桁数で割って切り上げ
		if (m_PatternWidth <= 2)m_PatternWidth = 2; //最低2マス伸ばす

		// 枠全体の横幅の半分を引いてm_FramePosを中心に中央揃え
		float totalWidth = (m_PatternWidth + 2) * FRAME_PATTAN_SIZE;
		m_OffsetX = totalWidth / 2 - FRAME_PATTAN_SIZE / 2;

		m_BG->SetSize({ m_PatternWidth * FRAME_PATTAN_SIZE + FRAME_PATTAN_SIZE / 2, FRAME_PATTAN_SIZE * 2 - FRAME_PATTAN_SIZE / 2 });
	};

	void HideFrame()
	{
		m_Use = false;
		m_DigitNum = 0;
		m_PatternWidth = 0;
	}

	void Draw()override
	{
		if (!m_Use)return;

		m_BG->Draw();

		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < m_PatternWidth + 2; x++)
			{
				// 中央行の内側は描画しない
				if (y == 1 && x != 0 && x != m_PatternWidth + 1)
				{
					continue;
				}

				//一列目
				if (x == 0)
				{
					// 左列
					switch (y)
					{
					case 0: // 左上角
						m_TextureNumber = 0;
						m_Rotation = FRAME_LEFTUP_ROT;
						break;
					case 1: // 左縦棒
						m_TextureNumber = 1;
						m_Rotation = -90.0f;
						break;
					case 2: // 左下角
						m_TextureNumber = 0;
						m_Rotation = FRAME_LEFTBOTTOM_ROT;
						break;
					}
				}
				else if (x == m_PatternWidth + 1)
				{
					// 右列
					switch (y)
					{
					case 0: // 右上角
						m_TextureNumber = 0;
						m_Rotation = FRAME_RIGHTUP_ROT;
						break;
					case 1: // 右縦棒
						m_TextureNumber = 1;
						m_Rotation = 90.0f;
						break;
					case 2: // 右下角
						m_TextureNumber = 0;
						m_Rotation = FRAME_RIGHTBOTTOM_ROT;
						break;
					}
				}
				else if (y == 0)
				{
					//幽霊
					if (x == 1)
					{
						m_TextureNumber = 2;
					}
					//バスターず
					else if (x == m_PatternWidth)
					{
						m_TextureNumber = 3;
					}
					else
					{
						// 上横棒
						m_TextureNumber = 1;

					}
					m_Rotation = 0.0f;
				}
				else if (y == 2)
				{
					// 下横棒
					m_TextureNumber = 1;
					m_Rotation = 180.0f;
				}

				m_Position.x = m_FramePos.x + (FRAME_PATTAN_SIZE * x) - m_OffsetX;
				m_Position.y = m_FramePos.y + (FRAME_PATTAN_SIZE * (y - 1));

				Sprite_Split_Draw(m_Position, m_Scale, m_Rotation, m_Color, m_BlendState, m_Texture, m_DivideX, m_DivideY, m_TextureNumber);
			}
		}
	}
};