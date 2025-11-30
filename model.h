#pragma once
//#define NOMINMAX
#include <unordered_map>

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"
#pragma comment (lib, "assimp-vc143-mt.lib")

#include	"d3d11.h"
#include	"DirectXMath.h"
using namespace DirectX;
#include	"direct3d.h"


struct MODEL
{
	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer;
	ID3D11Buffer** IndexBuffer;

	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;

	// メッシュ単位のインデックス数
	unsigned int* MeshIndexCounts;
	
	// メッシュ単位のマテリアル情報
	struct MeshMaterial
	{
		XMFLOAT4 diffuseColor;
		bool hasTexture;
		std::string texturePath;
		ID3D11ShaderResourceView* textureView;
	}* MeshMaterials;

	// 白テクスチャ（テクスチャ無しメッシュ用）
	ID3D11ShaderResourceView* WhiteTexture;
};


MODEL* ModelLoad(const char* FileName);
void ModelRelease(MODEL* model);
void ModelDraw(MODEL* model, XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 scale, const XMFLOAT4& color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), bool useColorReplace = false);
XMFLOAT3 ModelGetSize(MODEL* model);
XMFLOAT4 ModelGetAverageMaterialColor(MODEL* model);