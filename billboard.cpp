#include "billboard.h"
#include "direct3d.h"
#include "texture.h"
#include "shader.h"
#include "Camera.h"
#include <vector>

struct BILLBOARD_VERTEX
{
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT4 color;
	XMFLOAT2 tex;
};

Billboard::Billboard()
	: m_Pos(0, 0, 0), m_Size(1, 1), m_Rot(0, 0, 0), m_IsDoubleSided(false),
	m_Texture(nullptr), m_VertexBuffer(nullptr), m_VertexCount(0),
	m_CurrentIconType(BILLBOARD_ICON::NONE),
	m_IgnoreLighting(true) // デフォルトでライティング無効化
{
	m_Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
}

Billboard::~Billboard()
{
	if (m_VertexBuffer)
	{
		m_VertexBuffer->Release();
		m_VertexBuffer = nullptr;
	}
}

// 初期化
void Billboard::Initialize(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT3 rot, bool isDoubleSided)
{
	m_Pos = pos;
	m_Size = size;
	m_Rot = rot;
	m_IsDoubleSided = isDoubleSided;
	m_Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_IgnoreLighting = true; // ライティング無効化を有効

	// バッファ作成
	CreateBuffer();

	// 最初は非表示
	SetIcon(BILLBOARD_ICON::NONE);
}

// アイコン切り替え処理
void Billboard::SetIcon(BILLBOARD_ICON type)
{
	// 同じアイコンなら何もしない
	if (m_CurrentIconType == type) return;

	m_CurrentIconType = type;

	switch (type)
	{
	case BILLBOARD_ICON::NONE:
		m_Texture = nullptr; // 描画しない
		break;

	case BILLBOARD_ICON::ALERT:
		// 「！」画像のパス
		SetTexture("asset\\texture\\icon_alert.png");
		break;

	case BILLBOARD_ICON::QUESTION:
		// 「？」画像のパス
		SetTexture("asset\\texture\\icon_question.png");
		break;

	case BILLBOARD_ICON::STUN:
		// 「気絶」画像のパス
		SetTexture("asset\\texture\\icon_stun.png");
		break;

	case BILLBOARD_ICON::GHOST:
		// 「お化け」画像のパス
		SetTexture("asset\\texture\\icon_ghost.png");
		break;

	case BILLBOARD_ICON::SEARCH:
	// 「探索」画像のパス
		SetTexture("asset\\texture\\tansaku.png");
		break;

	case BILLBOARD_ICON::CHECK:
	// 「チェック」画像のパス
		SetTexture("asset\\texture\\tyousa.png");
		break;

	case BILLBOARD_ICON::DESTINATION:
		SetTexture("asset\\texture\\icon_shitayazirusi.png");
		break;

	default:
		m_Texture = nullptr;
		break;
	}
}

void Billboard::SetTexture(const char* texturePath)
{
	if (texturePath)
	{
		std::string strPath(texturePath);
		std::wstring wstrPath(strPath.begin(), strPath.end());
		m_Texture = LoadTexture(wstrPath.c_str());
	}
	else
	{
		m_Texture = nullptr;
	}
}

void Billboard::CreateBuffer(void)
{
	if (m_VertexBuffer) { m_VertexBuffer->Release(); m_VertexBuffer = nullptr; }

	float w = 0.5f;
	float h = 0.5f;
	std::vector<BILLBOARD_VERTEX> vList;

	// ライティング無効化時は頂点カラーを白に設定
	XMFLOAT4 vertexColor = m_IgnoreLighting ? XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) : XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// 表面
	vList.push_back({ { -w,  h, 0.0f }, { 0.0f, 0.0f, -1.0f }, vertexColor, { 0.0f, 0.0f } });
	vList.push_back({ {  w,  h, 0.0f }, { 0.0f, 0.0f, -1.0f }, vertexColor, { 1.0f, 0.0f } });
	vList.push_back({ { -w, -h, 0.0f }, { 0.0f, 0.0f, -1.0f }, vertexColor, { 0.0f, 1.0f } });
	vList.push_back({ { -w, -h, 0.0f }, { 0.0f, 0.0f, -1.0f }, vertexColor, { 0.0f, 1.0f } });
	vList.push_back({ {  w,  h, 0.0f }, { 0.0f, 0.0f, -1.0f }, vertexColor, { 1.0f, 0.0f } });
	vList.push_back({ {  w, -h, 0.0f }, { 0.0f, 0.0f, -1.0f }, vertexColor, { 1.0f, 1.0f } });

	// 裏面
	if (m_IsDoubleSided)
	{
		vList.push_back({ {  w,  h, 0.0f }, { 0.0f, 0.0f, 1.0f }, vertexColor, { 1.0f, 0.0f } });
		vList.push_back({ { -w,  h, 0.0f }, { 0.0f, 0.0f, 1.0f }, vertexColor, { 0.0f, 0.0f } });
		vList.push_back({ {  w, -h, 0.0f }, { 0.0f, 0.0f, 1.0f }, vertexColor, { 1.0f, 1.0f } });
		vList.push_back({ {  w, -h, 0.0f }, { 0.0f, 0.0f, 1.0f }, vertexColor, { 1.0f, 1.0f } });
		vList.push_back({ { -w,  h, 0.0f }, { 0.0f, 0.0f, 1.0f }, vertexColor, { 0.0f, 0.0f } });
		vList.push_back({ { -w, -h, 0.0f }, { 0.0f, 0.0f, 1.0f }, vertexColor, { 0.0f, 1.0f } });
	}

	m_VertexCount = (int)vList.size();

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(BILLBOARD_VERTEX) * m_VertexCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vList.data();

	Direct3D_GetDevice()->CreateBuffer(&bd, &InitData, &m_VertexBuffer);
}

void Billboard::Update(void)
{
	// アニメーション用
}

void Billboard::Draw(void)
{
	if (!m_VertexBuffer || !m_Texture) return;

	// 状態の設定
	SetBlendState(BLENDSTATE_ALFA);
	SetDepthWrite(false);

	Shader_Begin();

	// 1. カメラ情報の取得
	XMMATRIX view = GetCamera()->GetView();
	XMMATRIX proj = GetCamera()->GetProjection();

	// カメラ座標の取得（ビルボード回転用）
	XMVECTOR det;
	XMMATRIX invView = XMMatrixInverse(&det, view);
	invView.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMMATRIX matBillboard = invView;

	// 2. ワールド行列の作成
	XMMATRIX matScale = XMMatrixScaling(m_Size.x, m_Size.y, 1.0f);
	XMMATRIX matRot = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(m_Rot.x),
		XMConvertToRadians(m_Rot.y),
		XMConvertToRadians(m_Rot.z)
	);
	XMMATRIX matTrans = XMMatrixTranslation(m_Pos.x, m_Pos.y, m_Pos.z);

	// ワールド行列の完成
	XMMATRIX world = matScale * matRot * matBillboard * matTrans;

	// 3. ワールド・ビュー・プロジェクションを全部かけ合わせる (WVP行列)
	XMMATRIX wvp = world * view * proj;

	Shader_SetMatrix(wvp);
	Shader_SetWorldMatrix(world);

	// Billboard のマテリアルカラーを白に設定（ライティング影響を最小化）
	Shader_SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// 4. 描画
	ID3D11DeviceContext* context = Direct3D_GetDeviceContext();
	UINT stride = sizeof(BILLBOARD_VERTEX);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->PSSetShaderResources(0, 1, &m_Texture);

	context->Draw(m_VertexCount, 0);

	// 状態を戻す
	SetDepthWrite(true);
}