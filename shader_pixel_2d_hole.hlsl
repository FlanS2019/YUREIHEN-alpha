/*=============================================================================

   2D描画用 ピクセルシェーダー(円形くり抜き) [shader_pixel_2d_hole.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

cbuffer Buffer5 : register(b5)
{
    float4 HoleParams; // x: center.x(px) y: center.y(px) z: radius(px) w: softness(px)
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
    float4 baseColor = g_Texture.Sample(g_SamplerState, ps_in.texcoord) * ps_in.color;

    float2 center = HoleParams.xy;
    float radius = HoleParams.z;
    float softness = HoleParams.w;

    float dist = distance(ps_in.posH.xy, center);

    // 円の内側は描かない（下の画面を見せる）
    if (softness <= 0.0f)
    {
        if (dist < radius)
        {
            discard;
        }
        return baseColor;
    }

    // ふちをなめらかに（softness分だけフェード）
    // dist <= radius で透明、dist >= radius+softness で通常
    float a = saturate((dist - radius) / softness);
    baseColor.a *= a;
    return baseColor;
}
