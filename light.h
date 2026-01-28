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
#ifndef LIGHT_H
#define LIGHT_H

#define NOMINMAX
#include <Windows.h>
#include <DirectXMath.h>
#include <algorithm>
#include <cstdint>

using namespace DirectX;

// ================================================================================
// Light クラス：基本ライト（平行光源、ポイントライト、スポットライト対応）
// ================================================================================

class Light
{
protected:
	uint32_t type;				// 0=Directional, 1=Point, 2=Spot (4 bytes)
	uint32_t enable;			// ライトの有効無効 (4 bytes)
	uint32_t dummy[2];			// パディング (8 bytes) = 16 bytes total (1st register)
	
	XMFLOAT4 position;		// ライト位置（ポイントライト・スポットライト用） (16 bytes = 2nd register)
	XMFLOAT4 direction;		// ライトの向き（正規化される） (16 bytes = 3rd register)
	XMFLOAT4 diffuse;		// 光の色 (16 bytes = 4th register)
	XMFLOAT4 ambient;		// 環境光 (16 bytes = 5th register)
	
	float range;			// [Point/Spot] 減衰距離 (4 bytes)
	float intensity;		// [Point/Spot] 光の強さ (0.0-1.0) (4 bytes)
	float coneAngle;		// [Spot] コーン角度（度数法） (4 bytes)
	float falloff;			// [Spot] フォールオフ係数 (4 bytes) = 16 bytes total (6th register)
	
public:
	// 平行光源コンストラクタ
	Light(BOOL e, XMFLOAT4 direction, XMFLOAT4 diffuse, XMFLOAT4 ambient);

	// ポイントライト/スポットライトコンストラクタ
	Light(BOOL e, XMFLOAT4 position, XMFLOAT4 direction, XMFLOAT4 diffuse, XMFLOAT4 ambient);

	// ライトタイプ設定
	void SetLightType(uint32_t t) { this->type = t; }
	uint32_t GetLightType() const { return type; }
	
	// 有効/無効設定
	void SetEnable(BOOL enable) { this->enable = (uint32_t)enable; }
	uint32_t GetEnable() const { return enable; }
	
	// ポイントライト用メソッド
	void SetPosition(XMFLOAT4 pos) { this->position = pos; }
	void SetPosition(float x, float y, float z) { 
		this->position = XMFLOAT4(x, y, z, 1.0f); 
	}
	XMFLOAT4 GetPosition() const { return position; }
	
	// 方向設定用メソッド
	void SetDirection(XMFLOAT4 dir);
	XMFLOAT4 GetDirection() const { return direction; }
	
	// ディフューズ色設定
	void SetDiffuse(XMFLOAT4 diffuse) { this->diffuse = diffuse; }
	XMFLOAT4 GetDiffuse() const { return diffuse; }
	
	// アンビエント設定
	void SetAmbient(XMFLOAT4 ambient) { this->ambient = ambient; }
	XMFLOAT4 GetAmbient() const { return ambient; }
	
	// Point/Spot Light用：距離減衰
	void SetRange(float r) { 
		this->range = (r > 0.1f) ? r : 0.1f;
	}
	float GetRange() const { return range; }
	
	// Point/Spot Light用：強度
	void SetIntensity(float i) { 
		float clamped = (i < 0.0f) ? 0.0f : (i > 1.0f) ? 1.0f : i;
		this->intensity = clamped;
	}
	float GetIntensity() const { return intensity; }
	
	// Spot Light用：コーン角度（度数法）
	void SetConeAngle(float angle) { 
		float clamped = (angle < 1.0f) ? 1.0f : (angle > 180.0f) ? 180.0f : angle;
		this->coneAngle = clamped;
	}
	float GetConeAngle() const { return coneAngle; }
	
	// Spot Light用：フォールオフ鋭さ
	void SetFalloff(float f) { 
		float clamped = (f < 0.1f) ? 0.1f : (f > 10.0f) ? 10.0f : f;
		this->falloff = clamped;
	}
	float GetFalloff() const { return falloff; }

private:
	// ベクトルの正規化ヘルパー
	static void NormalizeVector(XMFLOAT4& vec);
};

// ================================================================================
// AmbientLight クラス：環境光管理
// ================================================================================

class AmbientLight
{
private:
	BOOL enable;			// 環境光の有効無効
	XMFLOAT4 ambientColor;	// 環境光の色

public:
	// コンストラクタ
	AmbientLight(BOOL enable, XMFLOAT4 color);
	
	// デストラクタ
	~AmbientLight() = default;
	
	// 有効/無効設定
	void SetEnable(BOOL enable) { this->enable = enable; }
	BOOL GetEnable() const { return enable; }
	
	// 環境光色設定
	void SetColor(XMFLOAT4 color) { this->ambientColor = color; }
	void SetColor(float r, float g, float b, float a = 1.0f) {
		this->ambientColor = XMFLOAT4(r, g, b, a);
	}
	XMFLOAT4 GetColor() const { return ambientColor; }
};

// ================================================================================
// SpotLight クラス：追従型スポットライト
// ================================================================================

class SpotLight
{
private:
	Light* light;				// ライトオブジェクト
	float heightOffset;			// 追従対象からの高さオフセット
	XMFLOAT3 lastTrackedPos;	// 最後に追従した位置

public:
	// コンストラクタ
	SpotLight(XMFLOAT3 initialPos, float intensity, XMFLOAT4 color);
	
	// デストラクタ
	~SpotLight();
	
	// ライト情報を追従対象に基づいて更新
	void UpdatePosition(XMFLOAT3 targetPos);
	
	// ライトオブジェクトの取得
	Light* GetLight() const { return light; }
	
	// 有効/無効設定
	void SetEnable(BOOL enable);
	BOOL GetEnable() const;
	
	// 色設定
	void SetColor(float r, float g, float b, float a = 1.0f);
	void SetColor(XMFLOAT4 color);
	XMFLOAT4 GetColor() const;
	
	// 方向設定
	void SetDirection(XMFLOAT4 dir);
	XMFLOAT4 GetDirection() const;
	
	// 高さオフセット設定
	void SetHeightOffset(float offset) { this->heightOffset = offset; }
	float GetHeightOffset() const { return heightOffset; }
	
	// 現在のライト位置を取得
	XMFLOAT4 GetPosition() const;
	
	// 距離減衰設定
	void SetRange(float r);
	float GetRange() const;
	
	// 光の強度設定
	void SetIntensity(float i);
	float GetIntensity() const;
	
	// スポットコーン角度設定（度数法）
	void SetConeAngle(float angle);
	float GetConeAngle() const;
	
	// スポットライトフォールオフ設定
	void SetFalloff(float f);
	float GetFalloff() const;
};

#endif // LIGHT_H
