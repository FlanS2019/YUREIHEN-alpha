#include <directxmath.h>
#include "sprite.h"
#include "box.h"
#include "direct3d.h"
using namespace DirectX;

// 通常の箱（テクスチャアトラス用などUVが分割されているもの）
static Vertex3D Box_vdata[BOX_NUM_VERTEX] =
{
	//---------------前面--------------------------------
	{//０　左上
		XMFLOAT3(-0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.0f)//テクスチャ座標
	},
	{//１　右上
		XMFLOAT3(0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.0f)//テクスチャ座標
	},
	{//２　左下
		XMFLOAT3(-0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,1.0f)//テクスチャ座標
	},
	{//５　右下
		XMFLOAT3(0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,1.0f)//テクスチャ座標
	},
	//----------------------------------------------------
	//----------------------右側面------------------------
	{//６　左上
		XMFLOAT3(0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.0f)//テクスチャ座標
	},
	{//７　右上
		XMFLOAT3(0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.0f)//テクスチャ座標
	},
	{//８　左下
		XMFLOAT3(0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,1.0f)//テクスチャ座標
	},
	{//１１　右下
		XMFLOAT3(0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,1.0f)//テクスチャ座標
	},
	//----------------------------------------------------
	//------------------------底面------------------------
	{//12　左上
		XMFLOAT3(-0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.0f)//テクスチャ座標
	},
	{//13　右上
		XMFLOAT3(0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.0f)//テクスチャ座標
	},
	{//14　左下
		XMFLOAT3(-0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,1.0f)//テクスチャ座標
	},
	{//17　右下
		XMFLOAT3(0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,1.0f)//テクスチャ座標
	},
	//----------------------------------------------------
	//------------------------対面------------------------
	{//０　左上
		XMFLOAT3(0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.0f)//テクスチャ座標
	},
	{//１　右上
		XMFLOAT3(-0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.0f)//テクスチャ座標
	},
	{//２　左下
		XMFLOAT3(0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,1.0f)//テクスチャ座標
	},
	{//５　右下
		XMFLOAT3(-0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,1.0f)//テクスチャ座標
	},
	//----------------------------------------------------
	//----------------------左側面------------------------
	{//６　左上
		XMFLOAT3(-0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.0f)//テクスチャ座標
	},
	{//７　右上
		XMFLOAT3(-0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.0f)//テクスチャ座標
	},
	{//８　左下
		XMFLOAT3(-0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,1.0f)//テクスチャ座標
	},
	{//１１　右下
		XMFLOAT3(-0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,1.0f)//テクスチャ座標
	},
	//----------------------------------------------------
	//------------------------天面------------------------
	{//12　左上
		XMFLOAT3(0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.0f)//テクスチャ座標
	},
	{//13　右上
		XMFLOAT3(-0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.0f)//テクスチャ座標
	},
	{//14　左下
		XMFLOAT3(0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(0,1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,1.0f)//テクスチャ座標
	},
	{//17　右下
		XMFLOAT3(-0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(0,1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,1.0f)//テクスチャ座標
	},
	//----------------------------------------------------
};

//1ポリゴン=3頂点*2=1面＊6面
static UINT Box_idxdata[6 * 6] =
{
	0,1,2,2,1,3,
	4,5,6,6,5,7,
	8,9,10,10,9,11,
	12,13,14,14,13,15,
	16,17,18,18,17,19,
	20,21,22,22,21,23
};

// BOXの一体を作成する
void CreateBox(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
	ID3D11Buffer** ppVertexBuffer, ID3D11Buffer** ppIndexBuffer)
{
	//頂点バッファ作成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(Vertex3D) * BOX_NUM_VERTEX;//格納できる頂点数
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		
		HRESULT hr = pDevice->CreateBuffer(&bd, NULL, ppVertexBuffer);
		if (FAILED(hr) || *ppVertexBuffer == NULL)
		{
			MessageBox(NULL, L"頂点バッファの作成に失敗しました", L"エラー", MB_OK);
			return;
		}

		D3D11_MAPPED_SUBRESOURCE msr;
		hr = pContext->Map(*ppVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		if (FAILED(hr))
		{
			MessageBox(NULL, L"頂点バッファのMapに失敗しました", L"エラー", MB_OK);
			return;
		}
		
		Vertex3D* vertex = (Vertex3D*)msr.pData;
		CopyMemory(&vertex[0], &Box_vdata[0], sizeof(Vertex3D) * BOX_NUM_VERTEX);
		pContext->Unmap(*ppVertexBuffer, 0);
	}

	//インデックスバッファ作成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(UINT) * 6 * 6;//格納できる頂点数
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		
		HRESULT hr = pDevice->CreateBuffer(&bd, NULL, ppIndexBuffer);
		if (FAILED(hr) || *ppIndexBuffer == NULL)
		{
			MessageBox(NULL, L"インデックスバッファの作成に失敗しました", L"エラー", MB_OK);
			return;
		}

		D3D11_MAPPED_SUBRESOURCE msr;
		hr = pContext->Map(*ppIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		if (FAILED(hr))
		{
			MessageBox(NULL, L"インデックスバッファのMapに失敗しました", L"エラー", MB_OK);
			return;
		}
		
		UINT* index = (UINT*)msr.pData;
		CopyMemory(&index[0], &Box_idxdata[0], sizeof(UINT) * 6 * 6);
		pContext->Unmap(*ppIndexBuffer, 0);
	}
}

// 1枚の画像を全面に貼るためのシンプル箱モデル作成関数
void CreateSimpleBox(ID3D11Device* pDevice, ID3D11Buffer** ppVB, ID3D11Buffer** ppIB)
{
	// 1m四方の箱 (UV座標 0.0～1.0)
	Vertex3D vertices[] = {
		// 前面 (Z-)
		{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1,1,1,1 }, { 0.0f, 0.0f } },
		{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1,1,1,1 }, { 1.0f, 0.0f } },
		{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1,1,1,1 }, { 0.0f, 1.0f } },
		{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1,1,1,1 }, { 1.0f, 1.0f } },

		// 背面 (Z+)
		{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 1,1,1,1 }, { 0.0f, 0.0f } },
		{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 1,1,1,1 }, { 1.0f, 0.0f } },
		{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 1,1,1,1 }, { 0.0f, 1.0f } },
		{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 1,1,1,1 }, { 1.0f, 1.0f } },

		// 上面 (Y+)
		{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 1,1,1,1 }, { 0.0f, 0.0f } },
		{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 1,1,1,1 }, { 1.0f, 0.0f } },
		{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1,1,1,1 }, { 0.0f, 1.0f } },
		{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1,1,1,1 }, { 1.0f, 1.0f } },

		// 下面 (Y-)
		{ { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1,1,1,1 }, { 0.0f, 0.0f } },
		{ {  0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1,1,1,1 }, { 1.0f, 0.0f } },
		{ { -0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f, 0.0f }, { 1,1,1,1 }, { 0.0f, 1.0f } },
		{ {  0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f, 0.0f }, { 1,1,1,1 }, { 1.0f, 1.0f } },

		// 左面 (X-)
		{ { -0.5f,  0.5f,  0.5f }, { -1.0f, 0.0f, 0.0f }, { 1,1,1,1 }, { 0.0f, 0.0f } },
		{ { -0.5f,  0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 1,1,1,1 }, { 1.0f, 0.0f } },
		{ { -0.5f, -0.5f,  0.5f }, { -1.0f, 0.0f, 0.0f }, { 1,1,1,1 }, { 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 1,1,1,1 }, { 1.0f, 1.0f } },

		// 右面 (X+)
		{ {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1,1,1,1 }, { 0.0f, 0.0f } },
		{ {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, { 1,1,1,1 }, { 1.0f, 0.0f } },
		{ {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1,1,1,1 }, { 0.0f, 1.0f } },
		{ {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, { 1,1,1,1 }, { 1.0f, 1.0f } },
	};

	unsigned int indices[] = {
		0, 1, 2,  2, 1, 3, // 前
		4, 5, 6,  6, 5, 7, // 後
		8, 9, 10, 10, 9, 11, // 上
		12, 13, 14, 14, 13, 15, // 下
		16, 17, 18, 18, 17, 19, // 左
		20, 21, 22, 22, 21, 23  // 右
	};


	// 頂点バッファ作成
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex3D) * 24; // 頂点数24
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0; // デフォルト使用なのでCPUアクセス不要

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertices;
	HRESULT hr = pDevice->CreateBuffer(&bd, &sd, ppVB);
	if (FAILED(hr) || *ppVB == NULL)
	{
		MessageBox(NULL, L"SimpleBox頂点バッファの作成に失敗しました", L"エラー", MB_OK);
		return;
	}

	bd.ByteWidth = sizeof(unsigned int) * 36; // インデックス数36
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	sd.pSysMem = indices;
	hr = pDevice->CreateBuffer(&bd, &sd, ppIB);
	if (FAILED(hr) || *ppIB == NULL)
	{
		MessageBox(NULL, L"SimpleBoxインデックスバッファの作成に失敗しました", L"エラー", MB_OK);
		return;
	}
}