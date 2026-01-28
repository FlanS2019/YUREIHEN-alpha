/*==============================================================================

   2D描画用頂点シェーダー [shader_vertex_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer Buffer0 : register(b0)
{
    float4x4 mtx;
};

cbuffer Buffer1 : register(b1)
{
    float4x4 worldMtx;
};

struct LIGHT
{
    float4 type_enable_dummy;
    float4 position;
    float4 direction;
    float4 diffuse;
    float4 ambient;
    float4 params;
};

cbuffer Buffer2 : register(b2)
{
    LIGHT Light;
};

//入力用頂点構造体
struct VS_INPUT
{
    float4 posL : POSITION0;
    float4 normal : NORMAL0;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

//出力用頂点構造体
struct VS_OUTPUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
    float4 normal : NORMAL0;        // 法線をピクセルシェーダーに渡す
    float4 worldPos : TEXCOORD1;    // ワールド座標
};

VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;
    
    // 頂点座標を射影座標へ変換
    vs_out.posH = mul(vs_in.posL, mtx);
    
    // テクスチャ座標
    vs_out.texcoord = vs_in.texcoord;
    
    // ワールド座標を計算（ライティング計算用）
    vs_out.worldPos = mul(vs_in.posL, worldMtx);
    
    // 法線をワールド座標系に変換
    // 回転行列のみを適用（スケーリングの影響を除く）
    float3 worldNormal = mul(vs_in.normal.xyz, (float3x3)worldMtx);
    
    // 法線を正規化
    vs_out.normal = float4(normalize(worldNormal), 0.0f);
    
    // 頂点カラー（そのまま出力）
    vs_out.color = vs_in.color;
    
    return vs_out;
}

//=============================================================================
// 頂点シェーダー
//=============================================================================
