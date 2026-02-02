#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
using namespace DirectX;



enum class BILLBOARD_ICON
{
	NONE,		// 表示なし
	ALERT,		// ！ (発見)
	QUESTION,	// ？ (警戒・不審)
	STUN,		// 星 (気絶)
	GHOST,		// お化けマーク (プレイヤー位置など)
	
};

class Billboard
{
public:
	Billboard();
	~Billboard();

	// 初期化
	void Initialize(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT3 rot, bool isDoubleSided = false);

	void Update(void);
	void Draw(void);

	// --- 便利機能 ---

	// 定義したアイコンに切り替える関数
	void SetIcon(BILLBOARD_ICON type);

	// 任意の画像パスを指定したい場合用
	void SetTexture(const char* texturePath);


	// --- 基本セッター・ゲッター ---
	void SetPos(XMFLOAT3 pos) { m_Pos = pos; }
	XMFLOAT3 GetPos(void) { return m_Pos; }

	void SetSize(XMFLOAT2 size) { m_Size = size; }
	XMFLOAT2 GetSize(void) { return m_Size; }

	void SetRotation(XMFLOAT3 rot) { m_Rot = rot; }
	XMFLOAT3 GetRotation(void) { return m_Rot; }

	void SetColor(XMFLOAT4 color) { m_Color = color; }

private:
	ID3D11Buffer* m_VertexBuffer;
	ID3D11ShaderResourceView* m_Texture;

	XMFLOAT3 m_Pos;
	XMFLOAT2 m_Size;
	XMFLOAT3 m_Rot;
	XMFLOAT4 m_Color;

	bool m_IsDoubleSided;
	int m_VertexCount;

	// 現在のアイコンタイプを覚えておく
	BILLBOARD_ICON m_CurrentIconType;

	void CreateBuffer(void);
};