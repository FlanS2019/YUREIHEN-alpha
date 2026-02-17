#define MAX_POINT_LIGHTS 8
#define MAX_BONES 256

cbuffer Buffer0 : register(b0)
{
	float4x4 mtx;
};

cbuffer Buffer1 : register(b1)
{
	float4x4 worldMtx;
};

struct POINT_LIGHT
{
	bool enable;
	bool3 dummy;
	float4 Position;
	float4 Direction;
	float4 Diffuse;
	float4 Params;
};

cbuffer Buffer2 : register(b2)
{
	POINT_LIGHT PointLights[MAX_POINT_LIGHTS];
	int PointLightCount;
	float3 LightPadding;
	float4 AmbientColor;
};

cbuffer BoneBuffer : register(b5)
{
	float4x4 BoneMatrices[MAX_BONES];
};

struct VS_INPUT
{
	float3 posL      : POSITION0;
	float3 normal    : NORMAL0;
	float4 color     : COLOR0;
	float2 texcoord  : TEXCOORD0;
	uint4  boneIndex : BLENDINDICES0;
	float4 boneWeight: BLENDWEIGHT0;
};

struct VS_OUTPUT
{
	float4 posH     : SV_POSITION;
	float4 color    : COLOR0;
	float2 texcoord : TEXCOORD0;
	float4 normal   : NORMAL0;
	float4 worldPos : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT vs_in)
{
	VS_OUTPUT vs_out;

	float4 posL4 = float4(vs_in.posL, 1.0f);
	float4 normL4 = float4(vs_in.normal, 0.0f);

	float4 skinnedPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 skinnedNormal = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float totalWeight = vs_in.boneWeight.x + vs_in.boneWeight.y + vs_in.boneWeight.z + vs_in.boneWeight.w;

	if (totalWeight > 0.0f)
	{
		[unroll]
		for (int i = 0; i < 4; i++)
		{
			float w = 0.0f;
			uint idx = 0;
			if (i == 0) { w = vs_in.boneWeight.x; idx = vs_in.boneIndex.x; }
			else if (i == 1) { w = vs_in.boneWeight.y; idx = vs_in.boneIndex.y; }
			else if (i == 2) { w = vs_in.boneWeight.z; idx = vs_in.boneIndex.z; }
			else { w = vs_in.boneWeight.w; idx = vs_in.boneIndex.w; }

			if (w > 0.0f && idx < MAX_BONES)
			{
				skinnedPos += w * mul(posL4, BoneMatrices[idx]);
				skinnedNormal += w * mul(normL4, BoneMatrices[idx]);
			}
		}
		skinnedPos.w = 1.0f;
	}
	else
	{
		skinnedPos = posL4;
		skinnedNormal = normL4;
	}

	vs_out.posH = mul(skinnedPos, mtx);
	vs_out.texcoord = vs_in.texcoord;
	vs_out.worldPos = mul(skinnedPos, worldMtx);

	vs_out.normal = float4(skinnedNormal.xyz, 0.0f);
	vs_out.normal = mul(vs_out.normal, worldMtx);
	vs_out.normal = normalize(vs_out.normal);

	vs_out.color = vs_in.color;

	return vs_out;
}
