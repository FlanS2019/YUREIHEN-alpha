#pragma execution_character_set("utf-8")

#include "shader_2d.h"
#include <fstream>
#include <windows.h>
#include <vector>
#include <cstring>

static ID3D11VertexShader* g_pVS2D = nullptr;
static ID3D11PixelShader* g_pPS2D_Default = nullptr;
static ID3D11PixelShader* g_pPS2D_Hole = nullptr;
static ID3D11InputLayout* g_pIL2D = nullptr;

static ID3D11Buffer* g_pCBProj = nullptr; // b0
static ID3D11Buffer* g_pCBHole = nullptr; // b5 (2D穴あき専用)

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

template<typename T>
static void UpdateCB(ID3D11Buffer* pBuffer, const T& data)
{
    if (!pBuffer || !g_pContext) return;
    D3D11_MAPPED_SUBRESOURCE msr;
    if (SUCCEEDED(g_pContext->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) {
        memcpy(msr.pData, &data, sizeof(T));
        g_pContext->Unmap(pBuffer, 0);
    }
}

static bool LoadCSO(const char* path, std::vector<unsigned char>& out)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    if (!ifs.read(reinterpret_cast<char*>(out.data()), size)) return false;
    return true;
}

bool Shader2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    if (!pDevice || !pContext) return false;
    g_pDevice = pDevice;
    g_pContext = pContext;

    std::vector<unsigned char> vs;
    if (!LoadCSO("shader_vertex_2d.cso", vs)) {
        MessageBox(nullptr, L"2D頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_2d.cso", L"エラー", MB_OK);
        return false;
    }

    if (FAILED(g_pDevice->CreateVertexShader(vs.data(), vs.size(), nullptr, &g_pVS2D))) {
        MessageBox(nullptr, L"2D頂点シェーダーの作成に失敗しました", L"エラー", MB_OK);
        return false;
    }

    // sprite.cppのVertexレイアウトに合わせる（POSITION/NORMAL/COLOR/TEXCOORD）
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (FAILED(g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vs.data(), vs.size(), &g_pIL2D))) {
        MessageBox(nullptr, L"2D入力レイアウトの作成に失敗しました", L"エラー", MB_OK);
        return false;
    }

    std::vector<unsigned char> ps0;
    if (!LoadCSO("shader_pixel_2d.cso", ps0)) {
        MessageBox(nullptr, L"2Dピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_2d.cso", L"エラー", MB_OK);
        return false;
    }
    if (FAILED(g_pDevice->CreatePixelShader(ps0.data(), ps0.size(), nullptr, &g_pPS2D_Default))) {
        MessageBox(nullptr, L"2Dピクセルシェーダーの作成に失敗しました", L"エラー", MB_OK);
        return false;
    }

    std::vector<unsigned char> psh;
    if (!LoadCSO("shader_pixel_2d_hole.cso", psh)) {
        MessageBox(nullptr, L"穴あき2Dピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_2d_hole.cso", L"エラー", MB_OK);
        return false;
    }
    if (FAILED(g_pDevice->CreatePixelShader(psh.data(), psh.size(), nullptr, &g_pPS2D_Hole))) {
        MessageBox(nullptr, L"穴あき2Dピクセルシェーダーの作成に失敗しました", L"エラー", MB_OK);
        return false;
    }

    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    bd.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
    if (FAILED(g_pDevice->CreateBuffer(&bd, nullptr, &g_pCBProj))) return false;

    // 16byteアライン
    bd.ByteWidth = sizeof(DirectX::XMFLOAT4);
    if (FAILED(g_pDevice->CreateBuffer(&bd, nullptr, &g_pCBHole))) return false;

    return true;
}

void Shader2D_Finalize()
{
    if (g_pPS2D_Hole) { g_pPS2D_Hole->Release(); g_pPS2D_Hole = nullptr; }
    if (g_pPS2D_Default) { g_pPS2D_Default->Release(); g_pPS2D_Default = nullptr; }
    if (g_pCBHole) { g_pCBHole->Release(); g_pCBHole = nullptr; }
    if (g_pCBProj) { g_pCBProj->Release(); g_pCBProj = nullptr; }
    if (g_pIL2D) { g_pIL2D->Release(); g_pIL2D = nullptr; }
    if (g_pVS2D) { g_pVS2D->Release(); g_pVS2D = nullptr; }

    g_pDevice = nullptr;
    g_pContext = nullptr;
}

void Shader2D_SetProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
    DirectX::XMFLOAT4X4 m;
    DirectX::XMStoreFloat4x4(&m, DirectX::XMMatrixTranspose(matrix));
    UpdateCB(g_pCBProj, m);
}

void Shader2D_SetHoleParams(const Shader2D_HoleParams& params)
{
    // float4(center.x, center.y, radius, softness)
    DirectX::XMFLOAT4 packed(params.centerPx.x, params.centerPx.y, params.radiusPx, params.softnessPx);
    UpdateCB(g_pCBHole, packed);
}

void Shader2D_BeginDefault()
{
    if (!g_pContext) return;
    g_pContext->VSSetShader(g_pVS2D, nullptr, 0);
    g_pContext->PSSetShader(g_pPS2D_Default, nullptr, 0);
    g_pContext->IASetInputLayout(g_pIL2D);

    g_pContext->VSSetConstantBuffers(0, 1, &g_pCBProj);
    // Hole用CBは未使用でもOK
}

void Shader2D_BeginHole()
{
    if (!g_pContext) return;
    g_pContext->VSSetShader(g_pVS2D, nullptr, 0);
    g_pContext->PSSetShader(g_pPS2D_Hole, nullptr, 0);
    g_pContext->IASetInputLayout(g_pIL2D);

    g_pContext->VSSetConstantBuffers(0, 1, &g_pCBProj);
    g_pContext->PSSetConstantBuffers(5, 1, &g_pCBHole);
}

void Shader2D_SetUseHolePS(bool enable)
{
    if (!g_pContext) return;

    // PSだけを切替。入力レイアウト/VS/CBは維持。
    g_pContext->PSSetShader(enable ? g_pPS2D_Hole : g_pPS2D_Default, nullptr, 0);

    if (enable)
    {
        g_pContext->PSSetConstantBuffers(5, 1, &g_pCBHole);
    }
    else
    {
        // 念のため解除（他のPSがb5を使っても影響しないように）
        ID3D11Buffer* nullCB[1] = { nullptr };
        g_pContext->PSSetConstantBuffers(5, 1, nullCB);
    }
}
