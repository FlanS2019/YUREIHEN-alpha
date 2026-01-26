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

#include <Windows.h>
#include <DirectXMath.h>

using namespace DirectX;

// ================================================================================
// Light クラス：基本ライト（平行光源、ポイントライト、スポットライト対応）
// ================================================================================

class Light
{
protected:
	BOOL enable;	// ライトの有効無効
	BOOL dummy[3];	// パディング
	XMFLOAT4 position;		// ライト位置（ポイントライト・スポットライト用）
	XMFLOAT4 direction;		// ライトの向き（正規化される）
	XMFLOAT4 diffuse;		// 光の色
	XMFLOAT4 ambient;		// 環境光
	
public:
	// 平行光源コンストラクタ
	Light(BOOL e, XMFLOAT4 direction, XMFLOAT4 diffuse, XMFLOAT4 ambient);

	// ポイントライト/スポットライトコンストラクタ
	Light(BOOL e, XMFLOAT4 position, XMFLOAT4 direction, XMFLOAT4 diffuse, XMFLOAT4 ambient);

	// 有効/無効設定
	void SetEnable(BOOL enable) { this->enable = enable; }
	BOOL GetEnable() const { return enable; }
	
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
	SpotLight(float heightOffset = 2.0f);
	
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
};

#endif // LIGHT_H
