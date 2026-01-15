// フィールド専用・インスタンシング描画用頂点シェーダー [shader_field_instance.hlsl]

// 定数バッファ
cbuffer Buffer0 : register(b0)
{
    float4x4 viewProj; // View-Projection行列
};

// 入力用頂点構造体 (VBスロット0)
struct VS_INPUT
{
    float4 posL : POSITION0;
    float4 normal : NORMAL0;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;

    // インスタンスデータ (VBスロット1)
    // CPU側でTranspose済みのため、デフォルト（column_major）で受け取る
    float4x4 instWorld : INSTANCEWORLD;
};

// 出力用頂点構造体
struct VS_OUTPUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
    float4 normal : NORMAL0;
    float4 worldPos : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;
    
    // インスタンス行列を使用してワールド座標を計算
    float4 worldPos = mul(vs_in.posL, vs_in.instWorld);

    // View-Projection行列で射影座標へ変換
    vs_out.posH = mul(worldPos, viewProj);
    
    // 頂点データのUVをそのまま出力へ
    vs_out.texcoord = vs_in.texcoord;
    vs_out.worldPos = worldPos;
    
    // 法線をワールド座標系に変換
    float3 worldNormal = mul(vs_in.normal.xyz, (float3x3)vs_in.instWorld);
    vs_out.normal = float4(normalize(worldNormal), 0.0f);
    
    vs_out.color = vs_in.color;
    
    return vs_out;
}
