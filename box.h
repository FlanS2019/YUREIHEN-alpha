#pragma once
#include "component.h"

#define BOX_NUM_VERTEX (24)


class Box : public Transform3D, public BoxCollider
{

public:
	Box(XMFLOAT3 pos, XMFLOAT3 size, bool isTrigger) :
		Transform3D(pos), BoxCollider(size, isTrigger)
	{
	}
};

void CreateBox(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
	ID3D11Buffer** ppVertexBuffer, ID3D11Buffer** ppIndexBuffer);

void CreateSimpleBox(ID3D11Device* pDevice, ID3D11Buffer** ppVB, ID3D11Buffer** ppIB);
