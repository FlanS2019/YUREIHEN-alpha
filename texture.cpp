#include "texture.h"

ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass)
{
	TexMetadata metadata;
	ScratchImage image; 
	ID3D11ShaderResourceView* g_Texture = NULL;

	// 標準的な方法でロード
	// WIC_FLAGS_NONE: メタデータをそのまま使用
	LoadFromWICFile(texpass, WIC_FLAGS_FORCE_SRGB, &metadata, image);
	// sRGB変換しない
	
	//// メタデータでsRGB対応フォーマットをチェック
	//bool isSRGB = DirectX::IsSRGB(metadata.format);
	//
	//// sRGB対応フォーマットでない場合は変換
	//if (!isSRGB)
	//{
	//	// 線形フォーマット → sRGB対応フォーマットに変換
	//	metadata.format = DirectX::MakeSRGB(metadata.format);
	//}
	//
	//// フォーマットをオーバーライド
	//image.OverrideFormat(metadata.format);
	
	// 標準的に作成
	CreateShaderResourceView(
		Direct3D_GetDevice(),
		image.GetImages(),
		image.GetImageCount(),
		metadata,
		&g_Texture
	);
	
	assert(g_Texture);		//ロード失敗時にダイアログを表示

	return g_Texture;
}