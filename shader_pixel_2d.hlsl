Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

struct LIGHT
{
	bool enable;
	bool3 dummy;
	float4 Direction;
	float4 Diffuse;
	float4 Ambient;
};

cbuffer Buffer2 : register(b2)
{
	LIGHT Light;
};

cbuffer Buffer3 : register(b3)
{
	float4 MaterialColor;
};

cbuffer Buffer4 : register(b4)
{
	float4 CameraPos;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float4 color : COLOR0;
	float2 texcoord : TEXCOORD0;
	float4 normal : NORMAL0;
	float4 worldPos : TEXCOORD1;
};

float4 main(PS_INPUT ps_in) : SV_TARGET
{
	float4 texColor = g_Texture.Sample(g_SamplerState, ps_in.texcoord);
    
	float4 materialColorSafe = MaterialColor;
	if (MaterialColor.r == 0.0f && MaterialColor.g == 0.0f && MaterialColor.b == 0.0f)
	{
		materialColorSafe = float4(1.0f, 1.0f, 1.0f, 1.0f);
	}
    
	float4 baseColor = texColor * materialColorSafe * ps_in.color;
    
	// Blinn-Phongライティング計算
	{
		float3 normal = normalize(ps_in.normal.xyz);
		
		// ライト方向（既に正規化済み）
		float3 lightDir = normalize(-Light.Direction.xyz);
		
		// ビュー方向：ワールド座標で計算
		float3 viewDir = normalize(CameraPos.xyz - ps_in.worldPos.xyz);
        
		// ディフューズ成分（Lambert）
		float lambertDiffuse = max(dot(normal, lightDir), 0.0f);
		float3 diffuse = lambertDiffuse * Light.Diffuse.rgb;
        
		// スペキュラ成分（Blinn-Phong）
		float3 halfDir = normalize(lightDir + viewDir);
		float blinnSpecular = max(dot(normal, halfDir), 0.0f);
		
		// 光沢度パラメータ：高いほど鏡面的（反射光がより集中）
		float shininess = 16.0f;
		float specularIntensity = pow(blinnSpecular, shininess);
		
		// スペキュラ強度：反射光の強さ
		float specularStrength = 2.0f;
		float3 specular = specularIntensity * specularStrength * Light.Diffuse.rgb;
        
		// 環境光（ディフューズをサポート）
		float3 ambient = Light.Ambient.rgb * 1.5f;
        
		// 最終的な色に合成
		// テクスチャ色 × (ディフューズ + 環境光) + スペキュラ（環境光の影響を受けない）
		float3 finalColor = baseColor.rgb * (diffuse + ambient) + specular;
		
		baseColor.rgb = finalColor;
	}
    
	return baseColor;
}