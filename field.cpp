#include "field.h"
#include "texture.h"
#include "Camera.h"
#include "sprite.h"

//グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
//頂点バッファ
static ID3D11Buffer* g_VertexBuffer = NULL;
//インデックスバッファ
static ID3D11Buffer* g_IndexBuffer = NULL;
//テクスチャ変数
static ID3D11ShaderResourceView* g_Texture;

//箱回転
XMFLOAT3 rotateBox = XMFLOAT3(0, 0, 0);

#define BOX_NUM_VERTEX (24)

//BOX作成関数
void CreateBox();

static Vertex3D Box_vdata[BOX_NUM_VERTEX] =
{
	//---------------前面--------------------------------
	{//０　左上
		XMFLOAT3(-0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//１　右上
		XMFLOAT3(0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	{//２　左下
		XMFLOAT3(-0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//５　右下
		XMFLOAT3(0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
	},
	//----------------------------------------------------
	//----------------------右側面------------------------
	{//６　左上
		XMFLOAT3(0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//７　右上
		XMFLOAT3(0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	{//８　左下
		XMFLOAT3(0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//１１　右下
		XMFLOAT3(0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
	},
	//----------------------------------------------------
	//------------------------底面------------------------
	{//12　左上
		XMFLOAT3(-0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//13　右上
		XMFLOAT3(0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
	},
	{//14　左下
		XMFLOAT3(-0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.75f)//テクスチャ座標
	},
	{//17　右下
		XMFLOAT3(0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.75f)//テクスチャ座標
	},
	//----------------------------------------------------
	//------------------------対面------------------------
	{//０　左上
		XMFLOAT3(0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//１　右上
		XMFLOAT3(-0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	{//２　左下
		XMFLOAT3(0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//５　右下
		XMFLOAT3(-0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
	},
	//----------------------------------------------------
	//----------------------左側面------------------------
	{//６　左上
		XMFLOAT3(-0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//７　右上
		XMFLOAT3(-0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	{//８　左下
		XMFLOAT3(-0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//１１　右下
		XMFLOAT3(-0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
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
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//17　右下
		XMFLOAT3(-0.5f,0.5f,0.5f),//頂点座標

		XMFLOAT3(0,1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	//----------------------------------------------------
};

//1ポリゴン=3頂点*2=1面＊6面
static UINT  Box_idxdata[6 * 6] =
{
	0,1,2,2,1,3,
	4,5,6,6,5,7,
	8,9,10,10,9,11,
	12,13,14,14,13,15,
	16,17,18,18,17,19,
	20,21,22,22,21,23
};
//マップデータ
MAPDATA Map[] =
{
	//1行目
	XMFLOAT3(1.0f, 2.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f, 1.0f), FIELD_BOX,

	//2行目			   
	XMFLOAT3(1.0f, 2.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f, 2.0f), FIELD_BOX,

	//3行目			   	 
	XMFLOAT3(1.0f, 2.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f, 3.0f), FIELD_BOX,

	//4行目			   	 
	XMFLOAT3(1.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(8.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(9.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(10.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(11.0f, 2.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(12.0f, 2.0f, 4.0f), FIELD_BOX,

	//5行目			   	 
	XMFLOAT3(1.0f, 2.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 3.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 3.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(8.0f,1.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(9.0f,1.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(10.0f,1.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(11.0f,1.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(12.0f,1.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(13.0f, 2.0f, 5.0f), FIELD_BOX,

	//6行目			   	 
	XMFLOAT3(3.0f, 3.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 3.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(7.0f,1.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(8.0f,1.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(10.0f,1.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(12.0f,1.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(13.0f,1.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(14.0f, 2.0f, 6.0f), FIELD_BOX,

	//7行目			   	 
	XMFLOAT3(3.0f, 3.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 3.0f, 7.0f), FIELD_BOX,
	XMFLOAT3( 6.0f,1.0f, 7.0f), FIELD_BOX,
	XMFLOAT3( 7.0f,1.0f, 7.0f), FIELD_BOX,
	XMFLOAT3( 9.0f,1.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(11.0f,1.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(13.0f,1.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(14.0f,1.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(15.0f, 2.0f, 7.0f), FIELD_BOX,


	//8行目			   	 
	XMFLOAT3(3.0f, 3.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 3.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(7.0f,1.0f,  8.0f), FIELD_BOX,
	XMFLOAT3(8.0f,1.0f,  8.0f), FIELD_BOX,
	XMFLOAT3(10.0f,1.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(12.0f,1.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(13.0f,1.0f, 8.0f), FIELD_BOX,

	XMFLOAT3(14.0f, 2.0f, 8.0f), FIELD_BOX,

	//9行目			   	 
	XMFLOAT3(1.0f, 2.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 3.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 3.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f, 9.0f), FIELD_BOX,
	XMFLOAT3( 8.0f,1.0f, 9.0f), FIELD_BOX,
	XMFLOAT3( 9.0f,1.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(10.0f,1.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(11.0f,1.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(12.0f,1.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(13.0f, 2.0f, 9.0f), FIELD_BOX,

	//10行目			   	 
	XMFLOAT3(1.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(8.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(9.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(10.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(11.0f, 2.0f,10.0f), FIELD_BOX,
	XMFLOAT3(12.0f, 2.0f,10.0f), FIELD_BOX,

	//11行目			   	 
	XMFLOAT3(1.0f, 2.0f,11.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f,11.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f,11.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f,11.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f,11.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f,11.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f,11.0f), FIELD_BOX,

	//12行目			   	 
	XMFLOAT3(1.0f, 2.0f,12.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f,12.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f,12.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f,12.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f,12.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f,12.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f,12.0f), FIELD_BOX,

	//13行目			   	 
	XMFLOAT3(1.0f, 2.0f,13.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 2.0f,13.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 2.0f,13.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 2.0f,13.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 2.0f,13.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 2.0f,13.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 2.0f,13.0f), FIELD_BOX,
	XMFLOAT3(2.0f, -1.0f, 5.0f), FIELD_MAX, //MAPデータ終了
};

void Field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Texture = LoadTexture(L"asset\\texture\\grass.png");

	// ブロックの作成
	for (int i = 0; i < FIELD_MAX; i++)
	{
		switch (Map[i].no)
		{
		case FIELD_BOX:
			CreateBox();
			break;
		}
	}
}

void Field_Update(void)
{
	//rotateBox.x += 0.5f;
	//rotateBox.y += 0.5f;
}

void Field_Draw(void)
{
	// シェーダーを使ってパイプライン設定
	Shader_Begin();

	// プロジェクション・ビュー行列
	XMMATRIX View = GetCamera()->GetView();
	XMMATRIX Projection = GetCamera()->GetProjection();
	XMMATRIX VP = View * Projection;

	// マップの描画
	int i = 0;
	while (Map[i].no != FIELD_MAX)
	{
		// スケーリング行列
		XMMATRIX ScalingMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);

		// 平行移動行列
		XMMATRIX TranslationMatrix = XMMatrixTranslation(
			Map[i].pos.x, Map[i].pos.y, Map[i].pos.z);

		// 回転行列
		XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(rotateBox.x),
			XMConvertToRadians(rotateBox.y),
			XMConvertToRadians(rotateBox.z));

		// モデル行列
		XMMATRIX Model = ScalingMatrix * RotationMatrix * TranslationMatrix;

		// 変換行列(WVP)
		XMMATRIX WVP = Model * VP; // (W * V * Projection)

		// シェーダーに変換行列をセット
		Shader_SetWorldMatrix(Model);
		Shader_SetMatrix(WVP);

		// テクスチャ
		g_pContext->PSSetShaderResources(0, 1, &g_Texture);

		// 頂点バッファ
		UINT stride = sizeof(Vertex3D); // 1個分のデータサイズ
		UINT offset = 0;
		g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);

		// インデックスバッファ
		g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// プリミティブトポロジ
		g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 描画
		g_pContext->DrawIndexed(6 * 6, 0, 0);

		i++;
	}
}

void Field_Finalize(void)
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_IndexBuffer);
	SAFE_RELEASE(g_Texture);
}

// BOXの一体を作成する
void CreateBox()
{
	//頂点バッファ作成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(Vertex3D) * BOX_NUM_VERTEX;//格納できる頂点数
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Direct3D_GetDevice()->CreateBuffer(&bd, NULL, &g_VertexBuffer);

		//頂点バッファをt頂点バッファに設定
		D3D11_MAPPED_SUBRESOURCE msr;
		g_pContext->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		Vertex3D* vertex = (Vertex3D*)msr.pData;
		//頂点データコピー
		CopyMemory(&vertex[0], &Box_vdata[0], sizeof(Vertex3D) * BOX_NUM_VERTEX);
		//コピー完了
		g_pContext->Unmap(g_VertexBuffer, 0);
	}

	//インデックスバッファ作成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(UINT) * 6 * 6;//格納できる頂点数
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Direct3D_GetDevice()->CreateBuffer(&bd, NULL, &g_IndexBuffer);


		//インデックスバッファに書き込み
		D3D11_MAPPED_SUBRESOURCE msr;
		g_pContext->Map(g_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		UINT* index = (UINT*)msr.pData;

		//インデックスデータコピー
		CopyMemory(&index[0], &Box_idxdata[0], sizeof(UINT) * 6 * 6);
		g_pContext->Unmap(g_IndexBuffer, 0);
	}
}
