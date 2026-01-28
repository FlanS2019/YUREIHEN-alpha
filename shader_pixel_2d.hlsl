Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

struct LIGHT
{
	float4 type_enable_dummy;		// x=type, y=enable, z=dummy, w=dummy
	float4 position;			// ポイント/スポットライト用位置
	float4 direction;			// 全ライト用方向（正規化済み）
	float4 diffuse;				// ディフューズ色
	float4 ambient;				// アンビエント色
	float4 params;				// x=range, y=intensity, z=coneAngle, w=falloff
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

// ====== ライティング計算関数 ======

// 距離減衰関数
float CalculateAttenuation(float distance, float range, float intensity)
{
	if (distance > range) return 0.0f;
	
	float attenuation = 1.0f - (distance / range);
	attenuation = max(0.0f, attenuation);
	return intensity * attenuation * attenuation; // 二次減衰
}

// スポットライトコーン計算
float CalculateSpotFalloff(float3 lightDir, float3 pixelToLight, float coneAngle, float falloff)
{
	// 長さがほぼゼロの場合は計算不能なので0を返す
	float lenDir = length(lightDir);
	float lenVec = length(pixelToLight);
	if (lenDir < 0.001f || lenVec < 0.001f) return 0.0f;

	// lightDirは光源の向く方向、pixelToLightはピクセルへの方向
	// 両方を正規化して内積を取る
	float3 normLightDir = lightDir / lenDir;
	float3 normPixelToLight = pixelToLight / lenVec;
	
	// コーン角度をコサイン値に変換
	float coneRad = radians(coneAngle / 2.0f);
	float cosTheta = cos(coneRad);
	
	// 内積：0に近いほどコーンの外側
	float dotProd = dot(normLightDir, normPixelToLight);
	
	// コーンの外側ならフォールオフなし
	if (dotProd < cosTheta) return 0.0f;
	
	// コーン内でのフォールオフ計算（中心に近いほど1に近い）
	float falloffFactor = (dotProd - cosTheta) / (1.0f - cosTheta);
	falloffFactor = max(0.0f, falloffFactor);
	
	// フォールオフ係数を適用
	return pow(falloffFactor, falloff);
}

// ====== end ======

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
    
	// ライトが無効な場合は環境光のみ適用して返す
	uint type = (uint)Light.type_enable_dummy.x;
	uint enable = (uint)Light.type_enable_dummy.y;
	
	if (enable == 0)
	{
		float3 ambientOnly = Light.ambient.rgb;
		
		// スポットライトの場合はアンビエント光を使わない
		if (type != 2)
		{
			if (ambientOnly.r < 0.1f && ambientOnly.g < 0.1f && ambientOnly.b < 0.1f)
			{
				ambientOnly = float3(1.0f, 1.0f, 1.0f);
			}
		}
		else
		{
			ambientOnly = float3(0.0f, 0.0f, 0.0f);
		}
		
		baseColor.rgb *= ambientOnly;
		return baseColor;
	}
    
	// Blinn-Phongライティング計算
	{
		float3 normal = normalize(ps_in.normal.xyz);
		float3 diffuse = float3(0.0f, 0.0f, 0.0f);
		float3 specular = float3(0.0f, 0.0f, 0.0f);
		
		float range = Light.params.x;
		float intensity = Light.params.y;
		float coneAngle = Light.params.z;
		float falloff = Light.params.w;
		
		// ビュー方向：ワールド座標で計算
		float3 viewDir = normalize(CameraPos.xyz - ps_in.worldPos.xyz);
		
		if (type == 0) // Directional Light
		{
			// 平行光源計算
			float3 lightDir = normalize(-Light.direction.xyz);
			float lambertDiffuse = max(dot(normal, lightDir), 0.0f);
			diffuse = lambertDiffuse * Light.diffuse.rgb;
			
			// スペキュラ計算（Blinn-Phong）
			if (MaterialColor.w > 0.5f)
			{
				float3 halfDir = normalize(lightDir + viewDir);
				float blinnSpecular = max(dot(normal, halfDir), 0.0f);
				float shininess = 16.0f;
				float specularIntensity = pow(blinnSpecular, shininess);
				float specularStrength = 2.0f;
				specular = specularIntensity * specularStrength * Light.diffuse.rgb;
			}
			else
			{
				float3 halfDir = normalize(lightDir + viewDir);
				float blinnSpecular = max(dot(normal, halfDir), 0.0f);
				float shininess = 2.0f;
				float specularIntensity = pow(blinnSpecular, shininess);
				float specularStrength = 0.3f;
				specular = specularIntensity * specularStrength * Light.diffuse.rgb;
			}
		}
		else if (type == 1) // Point Light
		{
			float3 lightVec = Light.position.xyz - ps_in.worldPos.xyz;
			float distance = length(lightVec);
			float3 lightDir = normalize(lightVec);
			
			// 距離減衰
			float attenuation = CalculateAttenuation(distance, range, intensity);
			
			if (attenuation > 0.0f)
			{
				float lambertDiffuse = max(dot(normal, lightDir), 0.0f);
				diffuse = lambertDiffuse * Light.diffuse.rgb * attenuation;
				
				// スペキュラ計算
				if (MaterialColor.w > 0.5f)
				{
					float3 halfDir = normalize(lightDir + viewDir);
					float blinnSpecular = max(dot(normal, halfDir), 0.0f);
					float shininess = 16.0f;
					float specularIntensity = pow(blinnSpecular, shininess);
					float specularStrength = 2.0f;
					specular = specularIntensity * specularStrength * Light.diffuse.rgb * attenuation;
				}
				else
				{
					float3 halfDir = normalize(lightDir + viewDir);
					float blinnSpecular = max(dot(normal, halfDir), 0.0f);
					float shininess = 2.0f;
					float specularIntensity = pow(blinnSpecular, shininess);
					float specularStrength = 0.3f;
					specular = specularIntensity * specularStrength * Light.diffuse.rgb * attenuation;
				}
			}
		}
		else if (type == 2) // Spot Light
		{
			float3 lightVec = Light.position.xyz - ps_in.worldPos.xyz;
			float distance = length(lightVec);
			float3 lightDir = normalize(lightVec);
			float3 lightToPixel = -lightVec; // 光源からピクセルへのベクトル
			
			// 距離減衰
			float distAttenuation = CalculateAttenuation(distance, range, intensity);
			
			// スポットコーン減衰
			float spotFalloff = CalculateSpotFalloff(Light.direction.xyz, lightToPixel, coneAngle, falloff);
			
			float attenuation = distAttenuation * spotFalloff;
			
			if (attenuation > 0.0f)
			{
				float lambertDiffuse = max(dot(normal, lightDir), 0.0f);
				diffuse = lambertDiffuse * Light.diffuse.rgb * attenuation;
				
				// スペキュラ計算
				if (MaterialColor.w > 0.5f)
				{
					float3 halfDir = normalize(lightDir + viewDir);
					float blinnSpecular = max(dot(normal, halfDir), 0.0f);
					float shininess = 16.0f;
					float specularIntensity = pow(blinnSpecular, shininess);
					float specularStrength = 2.0f;
					specular = specularIntensity * specularStrength * Light.diffuse.rgb * attenuation;
				}
				else
				{
					float3 halfDir = normalize(lightDir + viewDir);
					float blinnSpecular = max(dot(normal, halfDir), 0.0f);
					float shininess = 2.0f;
					float specularIntensity = pow(blinnSpecular, shininess);
					float specularStrength = 0.3f;
					specular = specularIntensity * specularStrength * Light.diffuse.rgb * attenuation;
				}
			}
		}
		
		// 環境光（ディフューズをサポート）
		float3 ambient = Light.ambient.rgb;
		
		// スポットライトはアンビエント光を使わない（局所的照明）
		if (type == 2)
		{
			ambient = float3(0.0f, 0.0f, 0.0f);
		}
		else
		{
			if (ambient.r < 0.01f && ambient.g < 0.01f && ambient.b < 0.01f)
			{
				ambient = float3(1.0f, 1.0f, 1.0f);
			}
		}
        
		// 最終的な色に合成
		float3 finalColor = baseColor.rgb * (diffuse + ambient) + specular;
		
		baseColor.rgb = finalColor;
	}
    
	return baseColor;
}