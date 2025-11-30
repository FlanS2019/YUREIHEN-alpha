#pragma once

#include <d3d11.h>
#include "direct3d.h"
#include "texture.h"
#include "component.h"
#include "model.h"
#include "debug_ostream.h"
#include <DirectXMath.h>
using namespace DirectX;

class Sprite3D : public Transform3D
{
protected:
	MODEL* m_Model;
	XMFLOAT3 m_ModelSize;
public:
	Sprite3D(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
		: Transform3D(pos, rot, scale), m_Model()
	{
		m_Model = ModelLoad(pass);
		m_ModelSize = ModelGetSize(m_Model);

		//サイズをデバッグ出力
		hal::dout << "Model Size: (" << m_ModelSize.x << ", " << m_ModelSize.y << ", " << m_ModelSize.z << ")" << std::endl;
	}
	~Sprite3D()
	{
		ModelRelease(m_Model);
	}

	virtual void Draw(void)
	{
		if (m_Model)
		{
			// モデル描画
			ModelDraw(
				m_Model,
				GetPos(),
				GetRot(),
				GetScale(),
				XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)  // デフォルト色は白
			);
		}
		else
		{
			hal::dout << "Sprite3D::Draw() : モデルが読み込まれていません。" << std::endl;
		}
	}

	XMFLOAT3 GetModelSize(void) const { return m_ModelSize; }
	XMFLOAT3 GetDisplaySize(void) const 
	{ 
		XMFLOAT3 scale = GetScale();
		return XMFLOAT3(
			m_ModelSize.x * scale.x,
			m_ModelSize.y * scale.y,
			m_ModelSize.z * scale.z
		);
	}

	XMFLOAT4 GetModelColor(void) const { return ModelGetAverageMaterialColor(m_Model); }
};