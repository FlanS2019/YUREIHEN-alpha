#define NOMINMAX

#include "model.h"
#include "texture.h"
#include "shader.h"
#include "camera.h"
#include "debug_ostream.h"
#include <DirectXMath.h>
#include <assert.h>
#include <iostream>
#include <float.h>
#include <algorithm>
#include <map>
#include <cstring> // memcpy

using namespace DirectX;

// Assimpの行列をDirectXMath形式に変換
XMMATRIX AiMatrixToXMMatrix(const aiMatrix4x4& mat)
{
	return XMMATRIX(
		mat.a1, mat.a2, mat.a3, mat.a4,
		mat.b1, mat.b2, mat.b3, mat.b4,
		mat.c1, mat.c2, mat.c3, mat.c4,
		mat.d1, mat.d2, mat.d3, mat.d4
	);
}

// ノードを再帰的に描画する内部関数
void RenderNode(MODEL* model, aiNode* node, XMMATRIX parentTransform, const XMFLOAT4& color, bool useColorReplace = false)
{
	// このノードのローカル変換行列と親の変換を組み合わせ
	XMMATRIX currentTransform = AiMatrixToXMMatrix(node->mTransformation) * parentTransform;

	// このノード以下のすべてのメッシュを描画
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		unsigned int meshIndex = node->mMeshes[i];
		aiMesh* mesh = model->AiScene->mMeshes[meshIndex];

		// マテリアルの色を計算
		XMFLOAT4 finalColor;
		if (useColorReplace)
		{
			// 色を置き換え（マテリアル色を無視）
			finalColor = color;
		}
		else
		{
			// ライト計算を有効化する処理
			// マテリアル色が黒い場合は白にリセット
			if (meshIndex < model->AiScene->mNumMeshes && model->MeshMaterials)
			{
				XMFLOAT4 meshColor = model->MeshMaterials[meshIndex].diffuseColor;
				
				// 【重要】メッシュの色が黒い場合は必ず白にリセット
				// これによりライトが正しく反映される
				if (meshColor.x == 0.0f && meshColor.y == 0.0f && meshColor.z == 0.0f)
				{
					meshColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				}
				
				finalColor = XMFLOAT4(
					meshColor.x * color.x,
					meshColor.y * color.y,
					meshColor.z * color.z,
					meshColor.w * color.w
				);
			}
			else
			{
				finalColor = color;
			}
		}
		
		Shader_SetMaterialColor(finalColor);

		// テクスチャをシェーダーに設定(プリキャッシュされた値を使用)
		ID3D11ShaderResourceView* textureToSet = model->MeshMaterials[meshIndex].textureView;
		Direct3D_GetDeviceContext()->PSSetShaderResources(0, 1, &textureToSet);

		// 頂点バッファ設定
		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[meshIndex], &stride, &offset);

		// インデックスバッファ設定
		Direct3D_GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer[meshIndex], DXGI_FORMAT_R32_UINT, 0);

		// インデックス数チェック：0の場合はスキップ
		unsigned int indexCount = model->MeshIndexCounts[meshIndex];
		if (indexCount > 0)
		{
			// 描画(保持されているインデックス数を使用)
			Direct3D_GetDeviceContext()->DrawIndexed(indexCount, 0, 0);
		}
	}

	// 子ノードを再帰実行
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		RenderNode(model, node->mChildren[i], currentTransform, color, useColorReplace);
	}
}

// アニメーション対応のノード描画関数（ノード変換適用版）
void RenderNodeAnimation(MODEL* model, aiNode* node, XMMATRIX parentTransform, const BoneMatrices& boneMatrices, const XMFLOAT4& color, bool useColorReplace = false, XMMATRIX worldTransform = XMMatrixIdentity())
{
	// このノードのローカル変換行列と親の変換を組み合わせ
	XMMATRIX currentTransform = AiMatrixToXMMatrix(node->mTransformation) * parentTransform;

	// ノード名がアニメーション対象の場合、ボーン行列を適用
	// FindBoneIndexで割り当てられたインデックスとノード名から対応を取る
	int nodeAnimIndex = -1;
	
	// ノード名とボーン行列インデックスのマッピング（簡略化）
	// ExtractAnimationFromAssimpで動的に割り当てられたインデックスを使用
	static std::map<std::string, int> nodeToMatrixIndex;
	std::string nodeName(node->mName.data);
	
	if (nodeToMatrixIndex.find(nodeName) == nodeToMatrixIndex.end())
	{
		// 最初のアニメーション情報からマッピングを作成
		if (model && model->AiScene && model->AiScene->mNumAnimations > 0)
		{
			aiAnimation* anim = model->AiScene->mAnimations[0];
			for (unsigned int c = 0; c < anim->mNumChannels; c++)
			{
				std::string channelName(anim->mChannels[c]->mNodeName.data);
				if (channelName == nodeName)
				{
					nodeToMatrixIndex[nodeName] = c;
					break;
				}
			}
		}
	}
	
	if (nodeToMatrixIndex.find(nodeName) != nodeToMatrixIndex.end())
	{
		nodeAnimIndex = nodeToMatrixIndex[nodeName];
		if (nodeAnimIndex >= 0 && nodeAnimIndex < BoneMatrices::MAX_BONES)
		{
			// ボーン行列を適用（ノード階層変換の代わりにアニメーション行列を使用）
			currentTransform = boneMatrices.matrices[nodeAnimIndex] * parentTransform;
		}
	}

	// このノード以下のすべてのメッシュを描画
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		unsigned int meshIndex = node->mMeshes[i];
		aiMesh* mesh = model->AiScene->mMeshes[meshIndex];

		// マテリアルの色を計算
		XMFLOAT4 finalColor;
		if (useColorReplace)
		{
			finalColor = color;
		}
		else
		{
			if (meshIndex < model->AiScene->mNumMeshes && model->MeshMaterials)
			{
				XMFLOAT4 meshColor = model->MeshMaterials[meshIndex].diffuseColor;
				
				if (meshColor.x == 0.0f && meshColor.y == 0.0f && meshColor.z == 0.0f)
				{
					meshColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				}
				
				finalColor = XMFLOAT4(
					meshColor.x * color.x,
					meshColor.y * color.y,
					meshColor.z * color.z,
					meshColor.w * color.w
				);
			}
			else
			{
				finalColor = color;
			}
		}
		
		Shader_SetMaterialColor(finalColor);

		// テクスチャをシェーダーに設定
		ID3D11ShaderResourceView* textureToSet = model->MeshMaterials[meshIndex].textureView;
		Direct3D_GetDeviceContext()->PSSetShaderResources(0, 1, &textureToSet);

		// 頂点バッファ設定
		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[meshIndex], &stride, &offset);

		// インデックスバッファ設定
		Direct3D_GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer[meshIndex], DXGI_FORMAT_R32_UINT, 0);

		// ワールド行列の設定（アニメーション変換 + モデルワールド変換を組み合わせ）
		XMMATRIX meshWorldMatrix = currentTransform * worldTransform;
		Shader_SetWorldMatrix(meshWorldMatrix);

		// WVP行列の計算と設定
		Camera* pCamera = GetCamera();
		if (pCamera)
		{
			XMMATRIX View = pCamera->GetView();
			XMMATRIX Projection = pCamera->GetProjection();
			XMMATRIX WVP = meshWorldMatrix * View * Projection;
			Shader_SetMatrix(WVP);
		}

		// インデックス数チェック：0の場合はスキップ
		unsigned int indexCount = model->MeshIndexCounts[meshIndex];
		if (indexCount > 0)
		{
			// 描画
			Direct3D_GetDeviceContext()->DrawIndexed(indexCount, 0, 0);
		}
	}

	// 子ノードを再帰実行
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		RenderNodeAnimation(model, node->mChildren[i], currentTransform, boneMatrices, color, useColorReplace, worldTransform);
	}
}

MODEL* ModelLoad(const char* FileName)
{
	MODEL* model = new MODEL;

	const std::string modelPath(FileName);

	// ===== モデルファイルの読み込み開始 =====
	hal::dout << std::endl;
	hal::dout << "========================================" << std::endl;
	hal::dout << ">> Model Loading: " << FileName << std::endl;
	hal::dout << "========================================" << std::endl;

	// Assimpのフラグを改善: Triangulateフラグで自動的に三角形化
	model->AiScene = aiImportFile(FileName, 
		aiProcessPreset_TargetRealtime_MaxQuality | 
		aiProcess_ConvertToLeftHanded |
		aiProcess_Triangulate |              // 四角形以上を三角形化
		aiProcess_GenSmoothNormals |         // スムーズ法線生成
		aiProcess_JoinIdenticalVertices |    // 重複頂点削除
		aiProcess_OptimizeGraph              // グラフ最適化
	);

	if (!model->AiScene)
	{
		// エラー内容を取得
		const char* errorString = aiGetErrorString();

		hal::dout << "ERROR: Model loading failed!" << std::endl;
		hal::dout << "  File: " << FileName << std::endl;
		hal::dout << "  Assimp Error: " << errorString << std::endl;
		hal::dout << "========================================" << std::endl;

		std::string msg = "モデルの読み込みに失敗しました。\n";
		msg += "ファイルパス: " + std::string(FileName) + "\n";
		msg += "エラー内容: " + std::string(errorString);

		// メッセージボックスを表示
		MessageBoxA(NULL, msg.c_str(), "Model Load Error", MB_OK | MB_ICONERROR);

		// 強制終了せずに安全に終わる（またはここで止める）
		delete model;
		return nullptr;
	}

	hal::dout << "  Scene loaded. Meshes: " << model->AiScene->mNumMeshes 
			  << ", Materials: " << model->AiScene->mNumMaterials
			  << ", Textures: " << model->AiScene->mNumTextures << std::endl;
	hal::dout << std::endl;

	model->VertexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];
	model->IndexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];
	model->MeshIndexCounts = new unsigned int[model->AiScene->mNumMeshes];
	model->MeshMaterials = new MODEL::MeshMaterial[model->AiScene->mNumMeshes];

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// ===== マテリアル情報の取得 =====
		{
			aiMaterial* material = model->AiScene->mMaterials[mesh->mMaterialIndex];

			// ディフューズ色（基本色）を取得
			aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
			aiReturn colorResult = material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
			
			// マテリアル色が取得できなかった場合、または全て0の場合は白をデフォルトに設定
			if (colorResult != AI_SUCCESS || 
				(diffuseColor.r == 0.0f && diffuseColor.g == 0.0f && diffuseColor.b == 0.0f))
			{
				diffuseColor = aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);  // デフォルトは白
				hal::dout << "  Mesh[" << m << "]: Material color not found or black -> set white" << std::endl;
			}

			model->MeshMaterials[m].diffuseColor = XMFLOAT4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);

			// テクスチャ情報の取得
			aiString texturePath;
			if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))
			{
				model->MeshMaterials[m].hasTexture = true;
				model->MeshMaterials[m].texturePath = texturePath.data;
			}
			else
			{
				model->MeshMaterials[m].hasTexture = false;
				model->MeshMaterials[m].texturePath.clear();
			}

			// Blinn/光沢度パラメータも取得可能（参考）
			float shininess = 32.0f;  // デフォルト光沢度
			material->Get(AI_MATKEY_SHININESS, shininess);
			// ※ 将来的にシェーダーコンスタントバッファに追加可能
		}

		// ===== 頂点バッファ生成 =====
		{
			Vertex3D* vertex = new Vertex3D[mesh->mNumVertices];

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				// 座標変換注意: aiProcess_ConvertToLeftHandedを使う場合は素直に代入
				vertex[v].position = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
				
				// テクスチャ座標が存在するかチェック
				if (mesh->HasTextureCoords(0))
				{
					vertex[v].texCoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
				}
				else
				{
					// テクスチャ座標がない場合はデフォルト値
					vertex[v].texCoord = XMFLOAT2(0.5f, 0.5f);
				}
				
				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				
				// 法線が存在するかチェック
				if (mesh->HasNormals())
				{
					vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
				}
				else
				{
					// 法線がない場合はデフォルト（上向き）
					vertex[v].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
				}
			}

			// 頂点数の検証
			if (mesh->mNumVertices == 0)
			{
				hal::dout << "ERROR: Mesh " << m << " has 0 vertices" << std::endl;
				delete[] vertex;
				return nullptr;
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DYNAMIC;  // 動的更新対応に変更
			bd.ByteWidth = sizeof(Vertex3D) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;  // CPU書き込み対応

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = vertex;

			HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);
			if (FAILED(hr))
			{
				hal::dout << "ERROR: Failed to create vertex buffer for mesh " << m << std::endl;
				delete[] vertex;
				return nullptr;
			}

			delete[] vertex;

			// デバッグ情報出力
			hal::dout << "  Mesh[" << m << "]: " << mesh->mNumVertices << " vertices, "
					  << (mesh->HasNormals() ? "WITH" : "WITHOUT") << " normals, "
					  << (mesh->HasTextureCoords(0) ? "WITH" : "WITHOUT") << " UVs"
					  << (mesh->HasBones() ? ", WITH bones" : "") << std::endl;
		}

		// ===== インデックスバッファ生成 =====
		{
			// 三角形化されているため、すべてのフェイスは3つのインデックスを持つ
			unsigned int indexCount = 0;
			
			// インデックス数を計算
			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];
				
				if (face->mNumIndices >= 3)
				{
					// 三角形化後は通常3、稀に4以上の場合は最初の三角形のみを使用
					indexCount += 3;
				}
			}

			// インデックス数を保存
			model->MeshIndexCounts[m] = indexCount;

			hal::dout << "  Mesh[" << m << "]: " << indexCount << " indices (" 
					  << mesh->mNumFaces << " faces)" << std::endl;

			// インデックス数が0の場合、ダミーバッファを作成
			if (indexCount == 0)
			{
				hal::dout << "  WARNING: Mesh[" << m << "] has 0 indices, creating dummy index buffer" << std::endl;
				
				// ダミーインデックス（1つの無効なインデックス）を作成
				unsigned int dummyIndex = 0;
				D3D11_BUFFER_DESC bd;
				ZeroMemory(&bd, sizeof(bd));
				bd.Usage = D3D11_USAGE_DEFAULT;
				bd.ByteWidth = sizeof(unsigned int);  // 最小サイズ
				bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
				bd.CPUAccessFlags = 0;

				D3D11_SUBRESOURCE_DATA sd;
				ZeroMemory(&sd, sizeof(sd));
				sd.pSysMem = &dummyIndex;

				HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);
				if (FAILED(hr))
				{
					hal::dout << "ERROR: Failed to create dummy index buffer for mesh " << m << std::endl;
					return nullptr;
				}
				continue;  // 次のメッシュへ
			}

			unsigned int* index = new unsigned int[indexCount];
			unsigned int indexOffset = 0;

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];

				// 三角形チェック（より柔軟に対応）
				if (face->mNumIndices >= 3 && indexOffset + 3 <= indexCount)
				{
					index[indexOffset + 0] = face->mIndices[0];
					index[indexOffset + 1] = face->mIndices[1];
					index[indexOffset + 2] = face->mIndices[2];
					indexOffset += 3;
				}
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * indexCount;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = index;

			HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);
			if (FAILED(hr))
			{
				hal::dout << "ERROR: Failed to create index buffer for mesh " << m << std::endl;
				delete[] index;
				return nullptr;
			}

			delete[] index;
		}
	}

	// ===== テクスチャ読み込み =====
	hal::dout << "Loading embedded textures..." << std::endl;
	for (unsigned int i = 0; i < model->AiScene->mNumTextures; i++)
	{
		aiTexture* aitexture = model->AiScene->mTextures[i];
		if (!aitexture) continue;

		ID3D11ShaderResourceView* texture = nullptr;
		TexMetadata metadata;
		ScratchImage image;

		// aiTexture: mHeight == 0 -> compressed image data (e.g. PNG/JPG) stored in pcData with size mWidth
		// mHeight  != 0 -> uncompressed raw image data (RGBA) with dimensions mWidth x mHeight
		if (aitexture->mHeight == 0)
		{
			// 圧縮データ (WIC対応) をメモリから読み込む
			if (aitexture->pcData && aitexture->mWidth > 0)
			{
				HRESULT hr = LoadFromWICMemory(reinterpret_cast<const uint8_t*>(aitexture->pcData), (size_t)aitexture->mWidth, WIC_FLAGS_NONE, &metadata, image);
				if (SUCCEEDED(hr))
				{
					hr = CreateShaderResourceView(Direct3D_GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &texture);
					if (FAILED(hr))
					{
						hal::dout << "ERROR: Failed to create SRV for embedded texture: " 
								  << (aitexture->mFilename.data ? aitexture->mFilename.data : "(unknown)") << std::endl;
					}
				}
				else
				{
					hal::dout << "ERROR: Failed to load embedded compressed texture: " 
							  << (aitexture->mFilename.data ? aitexture->mFilename.data : "(unknown)") << std::endl;
				}
			}
			else
			{
				hal::dout << "ERROR: Embedded texture has no data: " 
						  << (aitexture->mFilename.data ? aitexture->mFilename.data : "(unknown)") << std::endl;
			}
		}
		else
		{
			// 生データ (RGBA) を ScratchImage にコピーして SRV を作成する
			if (aitexture->pcData && aitexture->mWidth > 0 && aitexture->mHeight > 0)
			{
				HRESULT hr = image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, (size_t)aitexture->mWidth, (size_t)aitexture->mHeight, 1, 1);
				if (SUCCEEDED(hr))
				{
					const DirectX::Image* img = image.GetImage(0, 0, 0);
					if (img && img->pixels)
					{
						size_t bytes = (size_t)aitexture->mWidth * (size_t)aitexture->mHeight * 4; // RGBA
						memcpy(img->pixels, reinterpret_cast<const uint8_t*>(aitexture->pcData), bytes);
						metadata = image.GetMetadata();
						hr = CreateShaderResourceView(Direct3D_GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &texture);
						if (FAILED(hr))
						{
							hal::dout << "ERROR: Failed to create SRV for raw embedded texture: " 
									  << (aitexture->mFilename.data ? aitexture->mFilename.data : "(unknown)") << std::endl;
						}
					}
					else
					{
						hal::dout << "ERROR: ScratchImage pixel buffer not available for: " 
								  << (aitexture->mFilename.data ? aitexture->mFilename.data : "(unknown)") << std::endl;
					}
				}
				else
				{
					hal::dout << "ERROR: Failed to initialize ScratchImage for: " 
							  << (aitexture->mFilename.data ? aitexture->mFilename.data : "(unknown)") << std::endl;
				}
			}
			else
			{
				hal::dout << "ERROR: Invalid embedded raw texture data: " 
						  << (aitexture->mFilename.data ? aitexture->mFilename.data : "(unknown)") << std::endl;
			}
		}

		if (texture)
		{
			model->Texture[aitexture->mFilename.data] = texture;
		}
		else
		{
			// 失敗した場合はログのみ。モデルのテクスチャマップへ登録しない（後でデフォルトテクスチャを使う）
			hal::dout << "WARN: Embedded texture skipped: " 
					  << (aitexture->mFilename.data ? aitexture->mFilename.data : "(unknown)") << std::endl;
		}
	}

	// ダミー白テクスチャ（テクスチャなしメッシュ用）をモデルごとに読み込み
	model->WhiteTexture = LoadTexture(L"asset\\texture\\fade.png");
	if (!model->WhiteTexture)
	{
		hal::dout << "WARN: Failed to load white texture (fade.png)" << std::endl;
	}

	// メッシュごとのテクスチャをプリキャッシュ
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		if (model->MeshMaterials[m].hasTexture && model->Texture.count(model->MeshMaterials[m].texturePath))
		{
			model->MeshMaterials[m].textureView = model->Texture[model->MeshMaterials[m].texturePath];
		}
		else
		{
			model->MeshMaterials[m].textureView = model->WhiteTexture;
		}
	}

	hal::dout << std::endl;
	hal::dout << "========================================" << std::endl;
	hal::dout << "<< Model Loading Complete: " << FileName << std::endl;
	hal::dout << "========================================" << std::endl;

	// モデル情報サマリー
	XMFLOAT3 modelSize = ModelGetSize(model);
	XMFLOAT4 avgColor = ModelGetAverageMaterialColor(model);
	hal::dout << "  Model Size: (" << modelSize.x << ", " << modelSize.y << ", " << modelSize.z << ")" << std::endl;
	hal::dout << "  Average Material Color: (" << avgColor.x << ", " << avgColor.y << ", " << avgColor.z << ", " << avgColor.w << ")" << std::endl;
	hal::dout << "========================================" << std::endl;
	hal::dout << std::endl;

	return model;
}

void ModelRelease(MODEL* model)
{
	if (!model) return;

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		if (model->VertexBuffer[m])
			model->VertexBuffer[m]->Release();
		if (model->IndexBuffer[m])
			model->IndexBuffer[m]->Release();
	}

	delete[] model->VertexBuffer;
	delete[] model->IndexBuffer;
	delete[] model->MeshIndexCounts;
	delete[] model->MeshMaterials;

	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : model->Texture)
	{
		if (pair.second)
			pair.second->Release();
	}

	if (model->WhiteTexture)
		model->WhiteTexture->Release();

	if (model->AiScene)
		aiReleaseImport(model->AiScene);

	delete model;
}

void ModelDraw(MODEL* model, XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 scale, const XMFLOAT4& color, bool useColorReplace)
{
	if (!model) return;

	// カメラ取得
	Camera* pCamera = GetCamera();
	if (!pCamera) return;

	// ビュー・プロジェクション行列の取得
	XMMATRIX View = pCamera->GetView();
	XMMATRIX Projection = pCamera->GetProjection();

	// モデルの変換行列
	XMMATRIX TranslationMatrix = XMMatrixTranslation(pos.x,pos.y,pos.z);
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(rot.x),
		XMConvertToRadians(rot.y),
		XMConvertToRadians(rot.z));
	XMMATRIX ScalingMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

	// ワールド行列の計算(スケール → 回転 → 移動の順)
	XMMATRIX World = ScalingMatrix * RotationMatrix * TranslationMatrix;

	// WVP行列の計算
	XMMATRIX WVP = World * View * Projection;

	// シェーダーに行列を設定
	Shader_SetMatrix(WVP);           // WVP行列を設定
	Shader_SetWorldMatrix(World);    // ワールド行列を設定

	// シェーダーを使用してパイプラインを設定
	Shader_Begin();

	// プリミティブ・トポロジーを設定
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ルートノードから再帰的に描画開始(変換はスケール行列)
	// colorが渡されなかった場合（デフォルト）は白色を確保
	XMFLOAT4 finalColor = color;
	
	// カラー変更を使用していないなら白に固定
	if (!useColorReplace)
	{
		finalColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	
	XMMATRIX identity = XMMatrixIdentity();
	RenderNode(model, model->AiScene->mRootNode, identity, finalColor, useColorReplace);
}

void ModelAnimationDraw(MODEL* model, XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 scale, const BoneMatrices& boneMatrices, const XMFLOAT4& color, bool useColorReplace)
{
	if (!model) return;

	// モデルの変換行列（位置、回転、スケール）
	XMMATRIX TranslationMatrix = XMMatrixTranslation(pos.x, pos.y, pos.z);
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(rot.x),
		XMConvertToRadians(rot.y),
		XMConvertToRadians(rot.z));
	XMMATRIX ScalingMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

	// ワールド行列の計算(スケール → 回転 → 移動の順)
	XMMATRIX worldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;

	// シェーダーを使用してパイプラインを設定
	Shader_Begin();

	// プリミティブ・トポロジーを設定
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ルートノードから再帰的に描画開始
	XMFLOAT4 finalColor = color;
	
	if (!useColorReplace)
	{
		finalColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	
	XMMATRIX identity = XMMatrixIdentity();
	RenderNodeAnimation(model, model->AiScene->mRootNode, identity, boneMatrices, finalColor, useColorReplace, worldMatrix);
}

XMFLOAT3 ModelGetSize(MODEL* model)
{
	if (!model || !model->AiScene)
	{
		return XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	XMFLOAT3 minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		for (unsigned int v = 0; v < mesh->mNumVertices; v++)
		{
			aiVector3D pos = mesh->mVertices[v];

			minBounds.x = std::min(minBounds.x, pos.x);
			minBounds.y = std::min(minBounds.y, pos.y);
			minBounds.z = std::min(minBounds.z, pos.z);

			maxBounds.x = std::max(maxBounds.x, pos.x);
			maxBounds.y = std::max(maxBounds.y, pos.y);
			maxBounds.z = std::max(maxBounds.z, pos.z);
		}
	}

	XMFLOAT3 size(
		maxBounds.x - minBounds.x,
		maxBounds.y - minBounds.y,
		maxBounds.z - minBounds.z
	);

	return size;
}

XMFLOAT4 ModelGetAverageMaterialColor(MODEL* model)
{
	if (!model || !model->AiScene || model->AiScene->mNumMaterials == 0)
	{
		return XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
	unsigned int count = 0;

	for (unsigned int m = 0; m < model->AiScene->mNumMaterials; m++)
	{
		if (!model->MeshMaterials || m >= model->AiScene->mNumMeshes) continue;

		XMFLOAT4 matColor = model->MeshMaterials[m].diffuseColor;

		r += matColor.x;
		g += matColor.y;
		b += matColor.z;
		a += matColor.w;
		count++;
	}

	if (count > 0)
	{
		return XMFLOAT4(r / count, g / count, b / count, a / count);
	}

	return XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
}
