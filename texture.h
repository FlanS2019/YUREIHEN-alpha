#pragma once
#include <d3d11.h>
#include "direct3d.h"
using namespace DirectX;
#include "debug_ostream.h"

ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass);
