/*==============================================================================

   ライト関連 [light.h]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------
   
   統合内容：
   - Light クラス：平行光源、ポイントライト、スポットライト対応
   - AmbientLight クラス：フィールド環境光管理
   - SpotLight クラス：追従型スポットライト管理

==============================================================================*/
#pragma once
using namespace DirectX;
#define NOMINMAX
#include <Windows.h>
#include <DirectXMath.h>

// シェーダー定数バッファ用構造体
struct LightData
{
	BOOL enable;
	BOOL dummy[3];
	XMFLOAT4 position;
	XMFLOAT4 direction;
	XMFLOAT4 diffuse;
	XMFLOAT4 ambient;
	XMFLOAT4 params;	// x: range, y: intensity
};

class AmbientLight
{
public:
	XMFLOAT4 color;

	AmbientLight() : color(0.3f, 0.3f, 0.3f, 1.0f) {}
	AmbientLight(XMFLOAT4 color) : color(color) {}

	void SetColor(XMFLOAT4 color) { this->color = color; }
	XMFLOAT4 GetColor() const { return color; }
};

class PointLight
{
public:
	BOOL enable;
	XMFLOAT4 position;
	XMFLOAT4 direction;
	XMFLOAT4 diffuse;
	float range;
	float intensity;

	PointLight() : enable(FALSE), position{ 0,0,0,1 }, direction{ 0,0,1,0 }, diffuse{ 1,1,1,1 }, range(10.0f), intensity(1.0f) {}

	PointLight(BOOL e, XMFLOAT4 pos, XMFLOAT4 dir, XMFLOAT4 diffuse, float range = 10.0f, float intensity = 1.0f)
		: enable(e), position(pos), diffuse(diffuse), range(range), intensity(intensity)
	{
		SetDirection(dir);
	}

	void SetEnable(BOOL enable) { this->enable = enable; }
	BOOL GetEnable() const { return enable; }

	void SetPosition(XMFLOAT4 pos) { this->position = pos; }
	void SetPosition(float x, float y, float z) { this->position = XMFLOAT4(x, y, z, 1.0f); }
	XMFLOAT4 GetPosition() const { return this->position; }

	void SetDirection(XMFLOAT4 dir) {
		float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len > 0.0f) {
			this->direction = XMFLOAT4(dir.x / len, dir.y / len, dir.z / len, dir.w);
		}
		else {
			this->direction = dir;
		}
	}
	XMFLOAT4 GetDirection() const { return this->direction; }

	void SetDiffuse(XMFLOAT4 color) { this->diffuse = color; }
	XMFLOAT4 GetDiffuse() const { return this->diffuse; }

	void SetRange(float range) { this->range = range; }
	void SetIntensity(float intensity) { this->intensity = intensity; }
	float GetRange() const { return this->range; }
	float GetIntensity() const { return this->intensity; }
};
