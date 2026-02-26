/*=============================================================================

   2D描画用ピクセルシェーダー [shader_pixel_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/
#define MAX_POINT_LIGHTS 16

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

struct POINT_LIGHT
{
	int enable;
	int3 dummy;
	float4 Position;
	float4 Direction;
	float4 Diffuse;
	float4 Params; // x: Range, y: Intensity
};

cbuffer Buffer2 : register(b2)
{
	POINT_LIGHT PointLights[MAX_POINT_LIGHTS];
	int PointLightCount;
	float3 LightPadding;
	float4 AmbientColor;
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
	float4 normal : NORMAL0;
	float4 worldPos : TEXCOORD1;
};

float4 main(PS_INPUT ps_in) : SV_TARGET
{
	float4 texColor = g_Texture.Sample(g_SamplerState, ps_in.texcoord);
	float4 baseColor = texColor * MaterialColor * ps_in.color;

	// ポイントライトが0個の場合はアンビエントのみ適用（2D描画の高速化）
	int activeLights = min(PointLightCount, MAX_POINT_LIGHTS);
	[branch]
	if (activeLights == 0)
	{
		baseColor.rgb *= AmbientColor.rgb;
		return baseColor;
	}

	float3 normal = normalize(ps_in.normal.xyz);
	float3 diffuseAccum = float3(0.0f, 0.0f, 0.0f);
	float3 worldPos = ps_in.worldPos.xyz;

	[loop]
	for (int i = 0; i < activeLights; ++i)
	{
		[branch]
		if (!PointLights[i].enable)
		{
			continue;
		}

		float3 contribution = float3(0.0f, 0.0f, 0.0f);

		[branch]
		if (PointLights[i].Position.w > 0.5f)
		{
			// ポイントライト：距離チェックを先に行い範囲外なら即スキップ
			float3 lightVec = PointLights[i].Position.xyz - worldPos;
			float range = PointLights[i].Params.x;
			float distSq = dot(lightVec, lightVec);
			float rangeSq = range * range;

			[branch]
			if (distSq < rangeSq)
			{
				float dist = sqrt(distSq);
				float3 lightDir = lightVec / max(dist, 0.001f);
				float intensity = PointLights[i].Params.y;
				float attenuation = 1.0f - (dist / max(range, 0.1f));
				attenuation = attenuation * attenuation;

				float lambert = max(dot(normal, lightDir), 0.0f);
				contribution = lambert * PointLights[i].Diffuse.rgb * attenuation * intensity;
			}
		}
		else
		{
			float3 lightDir = normalize(-PointLights[i].Direction.xyz);
			float lambert = max(dot(normal, lightDir), 0.0f);
			contribution = lambert * PointLights[i].Diffuse.rgb;
		}

		diffuseAccum += contribution;
	}

	baseColor.rgb *= (diffuseAccum + AmbientColor.rgb);

	return baseColor;
}