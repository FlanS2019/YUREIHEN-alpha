/*==============================================================================

   クリック可能フォント [ClickFont.cpp]

==============================================================================*/
#include "ClickFont.h"

ClickFont::ClickFont(XMFLOAT2 pos, float fontSize, float rotation,
	XMFLOAT4 normalColor, XMFLOAT4 hoverColor, const std::string& text)
	: FontRenderer(pos, fontSize, rotation, normalColor, text)
	, m_NormalColor(normalColor)
	, m_HoverColor(hoverColor)
	, m_HitSize({ 200.0f, fontSize })
	, m_IsHover(false)
	, m_WasLeftDown(false)
	, m_OnClick(nullptr)
{
}

bool ClickFont::HitTest(int mouseX, int mouseY) const
{
	const XMFLOAT2 pos = GetPos();
	const float halfW = m_HitSize.x * 0.5f;
	const float halfH = m_HitSize.y * 0.5f;

	// 判定は常にGetPos()を中心に行う（UI毎の座標系差異をClickFont側で吸収しない）
	return (mouseX >= (int)(pos.x - halfW) && mouseX <= (int)(pos.x + halfW)
		&& mouseY >= (int)(pos.y - halfH) && mouseY <= (int)(pos.y + halfH));
}

void ClickFont::Update()
{
	Mouse_State ms{};
	Mouse_GetState(&ms);

	const bool hover = HitTest(ms.x, ms.y);
	if (hover != m_IsHover) {
		m_IsHover = hover;
		SetColor(m_IsHover ? m_HoverColor : m_NormalColor);
	}

	const bool leftDown = ms.leftButton;
	const bool pressedThisFrame = (leftDown && !m_WasLeftDown);
	m_WasLeftDown = leftDown;

	if (pressedThisFrame && m_IsHover) {
		if (m_OnClick) {
			m_OnClick();
		}
	}
}
