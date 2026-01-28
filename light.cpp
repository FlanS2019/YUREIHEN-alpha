/*==============================================================================

   ライト関連 [light.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------
   
   統合内容：
   - Light クラス：平行光源、ポイントライト、スポットライト対応
   - AmbientLight クラス：フィールド環境光管理
   - SpotLight クラス：追従型スポットライト管理

==============================================================================*/
#include "light.h"
#include <cmath>

// ================================================================================
// Light クラス実装
// ================================================================================

void Light::NormalizeVector(XMFLOAT4& vec)
{
	float len = sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	if (len > 0.0f) {
		vec.x /= len;
		vec.y /= len;
		vec.z /= len;
	}
}

// 平行光源コンストラクタ
// 用途：フィールド全体を照らす主要光源（太陽光など）
// 引数：
//   e - ライトの有効/無効
//   direction - ライト方向（正規化される）
//   diffuse - ディフューズ色
//   ambient - アンビエント色
Light::Light(BOOL e, XMFLOAT4 direction, XMFLOAT4 diffuse, XMFLOAT4 ambient)
	: type(0), enable((uint32_t)e), position(XMFLOAT4(0, 0, 0, 1)), diffuse(diffuse), ambient(ambient), direction(direction),
	  range(100.0f), intensity(1.0f), coneAngle(30.0f), falloff(1.0f)
{
	dummy[0] = 0;
	dummy[1] = 0;
	NormalizeVector(this->direction);
}

// ポイントライト/スポットライトコンストラクタ
// 用途：特定位置から放射する光（懐中電灯、炎、スポットライトなど）
// 引数：
//   e - ライトの有効/無効
//   position - ライト位置（ワールド座標）
//   direction - ライト方向（スポットライト用、正規化される）
//   diffuse - ディフューズ色
//   ambient - アンビエント色
Light::Light(BOOL e, XMFLOAT4 position, XMFLOAT4 direction, XMFLOAT4 diffuse, XMFLOAT4 ambient)
	: type(1), enable((uint32_t)e), position(position), diffuse(diffuse), ambient(ambient), direction(direction),
	  range(50.0f), intensity(1.0f), coneAngle(30.0f), falloff(1.0f)
{
	dummy[0] = 0;
	dummy[1] = 0;
	NormalizeVector(this->direction);
}

// 方向設定用メソッド
void Light::SetDirection(XMFLOAT4 dir)
{
	this->direction = dir;
	NormalizeVector(this->direction);
}

// ================================================================================
// AmbientLight クラス実装
// ================================================================================

// コンストラクタ
AmbientLight::AmbientLight(BOOL enable, XMFLOAT4 color)
	: enable(enable), ambientColor(color)
{
}

// ================================================================================
// SpotLight クラス実装
// ================================================================================

// コンストラクタ
SpotLight::SpotLight(XMFLOAT3 initialPos, float intensity, XMFLOAT4 color)
	: heightOffset(initialPos.y), lastTrackedPos(initialPos)
{
	// ポイントライト型のライトを生成（位置と方向を持つ）
	light = new Light(
		TRUE,
		XMFLOAT4(initialPos.x, initialPos.y, initialPos.z, 1.0f),	// 初期位置
		XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f),						// 方向：下向き
		color,													// ディフューズ色
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)						// アンビエント色：黒
		);
	
	// SpotLight専用に初期化
	if (light)
	{
		light->SetLightType(2);					// Spot Light タイプ
		light->SetRange(25.0f);					// 減衰距離
		light->SetIntensity(intensity);			// 光の強さ
		light->SetConeAngle(45.0f);				// コーン角度
		light->SetFalloff(1.5f);				// フォールオフ鋭さ
	}
}

// デストラクタ
SpotLight::~SpotLight()
{
	if (light)
	{
		delete light;
		light = nullptr;
	}
}

// ライト情報を追従対象に基づいて更新
void SpotLight::UpdatePosition(XMFLOAT3 targetPos)
{
	if (!light) return;
	
	float lightY = targetPos.y + heightOffset;
	
	// 追従位置を高さオフセット分だけ上方に設定
	light->SetPosition(
		targetPos.x,
		lightY,
		targetPos.z
	);
	
	XMFLOAT4 dir(
		targetPos.x - light->GetPosition().x,
		targetPos.y - lightY,
		targetPos.z - light->GetPosition().z,
		0.0f
	);
	light->SetDirection(dir);
	
	lastTrackedPos = targetPos;
}

// 有効/無効設定
void SpotLight::SetEnable(BOOL enable)
{
	if (light)
	{
		light->SetEnable(enable);
	}
}

BOOL SpotLight::GetEnable() const
{
	return light ? light->GetEnable() : FALSE;
}

// 色設定
void SpotLight::SetColor(float r, float g, float b, float a)
{
	if (light)
	{
		light->SetDiffuse(XMFLOAT4(r, g, b, a));
	}
}

void SpotLight::SetColor(XMFLOAT4 color)
{
	if (light)
	{
		light->SetDiffuse(color);
	}
}

XMFLOAT4 SpotLight::GetColor() const
{
	return light ? light->GetDiffuse() : XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
}

// 方向設定
void SpotLight::SetDirection(XMFLOAT4 dir)
{
	if (light)
	{
		light->SetDirection(dir);
	}
}

XMFLOAT4 SpotLight::GetDirection() const
{
	return light ? light->GetDirection() : XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
}

// 現在のライト位置を取得
XMFLOAT4 SpotLight::GetPosition() const
{
	return light ? light->GetPosition() : XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
}

// 距離減衰設定
void SpotLight::SetRange(float r)
{
	if (light)
	{
		light->SetRange(r);
	}
}

float SpotLight::GetRange() const
{
	return light ? light->GetRange() : 50.0f;
}

// 光の強度設定
void SpotLight::SetIntensity(float i)
{
	if (light)
	{
		light->SetIntensity(i);
	}
}

float SpotLight::GetIntensity() const
{
	return light ? light->GetIntensity() : 1.0f;
}

// スポットコーン角度設定（度数法）
void SpotLight::SetConeAngle(float angle)
{
	if (light)
	{
		light->SetConeAngle(angle);
	}
}

float SpotLight::GetConeAngle() const
{
	return light ? light->GetConeAngle() : 30.0f;
}

// スポットライトフォールオフ設定
void SpotLight::SetFalloff(float f)
{
	if (light)
	{
		light->SetFalloff(f);
	}
}

float SpotLight::GetFalloff() const
{
	return light ? light->GetFalloff() : 1.0f;
}
