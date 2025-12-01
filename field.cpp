#include "field.h"
#include "texture.h"
#include "Camera.h"
#include "sprite.h"
#include "box.h"

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
	
//マップデータ
MAPDATA Map[] =
{
	//15×21のマップにボックスを配置（X: -7 to 7, Z: -10 to +10）
	// Z=-10, 1行目
	XMFLOAT3(-7.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -10.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -10.0f), FIELD_BOX,

	// Z=-9
	XMFLOAT3(-7.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -9.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -9.0f), FIELD_BOX,

	// Z=-8
	XMFLOAT3(-7.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -8.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -8.0f), FIELD_BOX,

	// Z=-7
	XMFLOAT3(-7.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -7.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -7.0f), FIELD_BOX,

	// Z=-6
	XMFLOAT3(-7.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -6.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -6.0f), FIELD_BOX,

	// Z=-5
	XMFLOAT3(-7.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -5.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -5.0f), FIELD_BOX,

	// Z=-4
	XMFLOAT3(-7.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -4.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -4.0f), FIELD_BOX,

	// Z=-3
	XMFLOAT3(-7.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -3.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -3.0f), FIELD_BOX,

	// Z=-2
	XMFLOAT3(-7.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -2.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -2.0f), FIELD_BOX,

	// Z=-1
	XMFLOAT3(-7.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, -1.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, -1.0f), FIELD_BOX,

	// Z=0
	XMFLOAT3(-7.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 0.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 0.0f), FIELD_BOX,

	// Z=1
	XMFLOAT3(-7.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 1.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 1.0f), FIELD_BOX,

	// Z=2
	XMFLOAT3(-7.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 2.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 2.0f), FIELD_BOX,

	// Z=3
	XMFLOAT3(-7.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 3.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 3.0f), FIELD_BOX,

	// Z=4
	XMFLOAT3(-7.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 4.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 4.0f), FIELD_BOX,

	// Z=5
	XMFLOAT3(-7.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 5.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 5.0f), FIELD_BOX,

	// Z=6
	XMFLOAT3(-7.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 6.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 6.0f), FIELD_BOX,

	// Z=7
	XMFLOAT3(-7.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 7.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 7.0f), FIELD_BOX,

	// Z=8
	XMFLOAT3(-7.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 8.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 8.0f), FIELD_BOX,

	// Z=9
	XMFLOAT3(-7.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 9.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 9.0f), FIELD_BOX,

	// Z=10
	XMFLOAT3(-7.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(-6.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(-5.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(-4.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(-3.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(-2.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(-1.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(0.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(1.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(2.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(3.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(4.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(5.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(6.0f, 0.0f, 10.0f), FIELD_BOX,
	XMFLOAT3(7.0f, 0.0f, 10.0f), FIELD_BOX,

	XMFLOAT3(0.0f, -1.0f, 0.0f), FIELD_MAX, //MAPデータ終了
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
			CreateBox(pDevice, pContext, &g_VertexBuffer, &g_IndexBuffer); 
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