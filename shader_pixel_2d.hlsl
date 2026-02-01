/*=============================================================================

   2D描画用ピクセルシェーダー [shader_pixel_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/
Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

struct LIGHT
{
	bool enable;
	bool3 dummy;
	float4 Position; // ライト位置（ポイントライト用）
	float4 Direction; // ライト方向（直光源用）
	float4 Diffuse;
	float4 Ambient;
	float4 Params; // x: Range, y: Intensity
};

cbuffer Buffer2 : register(b2)
{
	LIGHT Light;
};

cbuffer Buffer3 : register(b3)
{
	float4 MaterialColor;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float4 color : COLOR0;
	float2 texcoord : TEXCOORD0;
	float4 normal : NORMAL0; // 法線（ワールド空間）
	float4 worldPos : TEXCOORD1; // ワールド座標
};

float4 main(PS_INPUT ps_in) : SV_TARGET
{
    // テクスチャからサンプル
	float4 texColor = g_Texture.Sample(g_SamplerState, ps_in.texcoord);
    
    // マテリアルカラー安全チェック
	float4 materialColorSafe = MaterialColor;
	if (MaterialColor.r == 0.0f && MaterialColor.g == 0.0f && MaterialColor.b == 0.0f)
	{
		materialColorSafe = float4(1.0f, 1.0f, 1.0f, 1.0f);
	}
    
    // ベースカラー = テクスチャカラー × マテリアルカラー × 頂点カラー
	float4 baseColor = texColor * materialColorSafe * ps_in.color;
    
    // ライティング計算
	if (Light.enable)
	{
        // 法線を正規化
		float3 normal = normalize(ps_in.normal.xyz);
        
		float3 diffuseColor = float3(0, 0, 0);
		float3 ambientColor = Light.Ambient.rgb;
        
        // Position の w=1 ならポイントライト、w=0 なら直光源
		if (Light.Position.w > 0.5f)
		{
            // ポイントライト：ピクセルからライトへの方向を計算
			float3 lightVec = Light.Position.xyz - ps_in.worldPos.xyz;
			float dist = length(lightVec);
			float3 lightDir = normalize(lightVec);

			// 距離減衰の計算
			float range = max(0.1f, Light.Params.x);
			float intensity = Light.Params.y;
			float attenuation = saturate(1.0f - (dist / range));
			attenuation = pow(attenuation, 2.0f); // 減衰を急にする
			
			float lambert = max(dot(normal, lightDir), 0.0f);
			diffuseColor = lambert * Light.Diffuse.rgb * attenuation * intensity;
		}
		else
		{
            // 直光源：方向をそのまま使用
			float3 lightDir = normalize(-Light.Direction.xyz);
			float lambert = max(dot(normal, lightDir), 0.0f);
			diffuseColor = lambert * Light.Diffuse.rgb;
		}
        
        // 最終的なライティングカラーの算出
		baseColor.rgb *= (diffuseColor + ambientColor);
	}
    
	return baseColor;
}