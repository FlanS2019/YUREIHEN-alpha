# シェーダー抜本的修正計画
## スポットライト実装 + リファクタリング

**作成日**: 2025年1月28日  
**対象バージョン**: DirectX11 2D/3D混在レンダリング

---

## 📋 現状分析

### 既存実装の状態
- ✅ **Light クラス**: 平行光源・ポイントライト・スポットライト基盤あり
- ✅ **SpotLight クラス**: 追従型スポットライト（懐中電灯用）実装済み
- ⚠️ **シェーダー側**: 単一のLIGHT構造体のみ、複数ライト非対応
- ⚠️ **ライティング**: Blinn-Phong実装だが、スポットライトパラメータ欠落
- ⚠️ **構造体設計**: スポットライト用の距離減衰・コーン角度パラメータが不足

### 問題点
1. **HLSL側でスポットライトの距離減衰が未実装**
   - Pointlight/SpotLightは位置を持つが、距離による減衰計算がない
   - 平行光源と同じディフューズ計算のまま

2. **スポットライトのコーン角度（Cone Angle）が設定できない**
   - Spot Angleがないため、光源の方向性が表現できない
   - 懐中電灯の狭い照射範囲が実現不可

3. **定数バッファの利用効率が低い**
   - 複数ライト対応を想定していない
   - 構造体パディングが非効率

4. **ピクセルシェーダーのライティング計算が平行光源ベース**
   - ポイントライトとスポットライトの区別がない
   - フォールオフ（距離減衰）やスペキュラ減衰がない

---

## 🎯 実装計画

### **フェーズ 1: HLSL構造体の再設計** (優先度: 高)

#### 1.1 Light構造体の拡張
```cpp
// HLSL側 (shader_pixel_2d.hlsl)
struct LIGHT {
    bool enable;           // ライト有効フラグ
    uint type;             // ライトタイプ: 0=Directional, 1=Point, 2=Spot
    uint dummy[2];         // パディング
    
    float4 position;       // ポイント/スポットライト用位置
    float4 direction;      // 全ライト用方向（正規化済み）
    float4 diffuse;        // ディフューズ色
    float4 ambient;        // アンビエント色
    
    // ポイント/スポットライト用
    float range;           // 減衰距離（この距離で光が0になる）
    float intensity;       // 光の強さ（0.0 - 1.0）
    
    // スポットライト専用
    float coneAngle;       // コーン角度（度数法、0-180度）
    float falloff;         // フォールオフ係数（鋭さ）
};
```

**C++ 側対応 (light.h/light.cpp)**
```cpp
class Light {
protected:
    UINT type;             // 0=Directional, 1=Point, 2=Spot
    float range;           // 減衰距離
    float intensity;       // 強度
    float coneAngle;       // スポットコーン角度
    float falloff;         // コーンフォールオフ
};
```

---

### **フェーズ 2: ピクセルシェーダー実装** (優先度: 高)

#### 2.1 距離減衰関数
```hlsl
// フォールオフ減衰（距離ベース）
float CalculateAttenuation(float distance, float range, float intensity) {
    if (distance > range) return 0.0f;
    
    float attenuation = 1.0f - (distance / range);
    attenuation = max(0.0f, attenuation);
    return intensity * attenuation * attenuation; // 二次減衰
}
```

#### 2.2 スポットライトコーン計算
```hlsl
// スポットライトのコーン計算
float CalculateSpotFalloff(float3 lightDir, float3 pixelToLight, 
                          float coneAngle, float falloff) {
    float dotProd = dot(normalize(lightDir), 
                        normalize(pixelToLight));
    float coneRad = radians(coneAngle / 2.0f);
    
    if (dotProd < cos(coneRad)) return 0.0f;
    
    float diff = (dotProd - cos(coneRad)) / 
                 (1.0f - cos(coneRad));
    return pow(max(0.0f, diff), falloff);
}
```

#### 2.3 ライティング計算の分岐
```hlsl
float4 CalculateLighting(LIGHT light, float3 normal, 
                         float3 pixelWorldPos, float3 viewDir) {
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    
    if (light.type == 0) {  // Directional
        // 既存の平行光源計算
        float3 lightDir = normalize(-light.direction.xyz);
        float lambertDiffuse = max(dot(normal, lightDir), 0.0f);
        diffuse = lambertDiffuse * light.diffuse.rgb;
        // スペキュラ計算...
        
    } else if (light.type == 1) {  // Point Light
        float3 lightVec = light.position.xyz - pixelWorldPos;
        float distance = length(lightVec);
        float3 lightDir = normalize(lightVec);
        
        // 距離減衰
        float attenuation = CalculateAttenuation(distance, 
                                                light.range, 
                                                light.intensity);
        
        if (attenuation > 0.0f) {
            float lambertDiffuse = max(dot(normal, lightDir), 0.0f);
            diffuse = lambertDiffuse * light.diffuse.rgb * attenuation;
            // スペキュラ計算（減衰適用）...
        }
        
    } else if (light.type == 2) {  // Spot Light
        float3 lightVec = light.position.xyz - pixelWorldPos;
        float distance = length(lightVec);
        float3 lightDir = normalize(lightVec);
        
        // 距離減衰
        float distAttenuation = CalculateAttenuation(distance, 
                                                    light.range, 
                                                    light.intensity);
        
        // スポットコーン減衰
        float spotFalloff = CalculateSpotFalloff(light.direction.xyz, 
                                               lightVec, 
                                               light.coneAngle, 
                                               light.falloff);
        
        float attenuation = distAttenuation * spotFalloff;
        
        if (attenuation > 0.0f) {
            float lambertDiffuse = max(dot(normal, lightDir), 0.0f);
            diffuse = lambertDiffuse * light.diffuse.rgb * attenuation;
            // スペキュラ計算（減衰適用）...
        }
    }
    
    return float4(diffuse, 1.0f);  // スペキュラ含めて返す
}
```

---

### **フェーズ 3: C++側コード更新** (優先度: 中)

#### 3.1 Light クラス拡張
```cpp
// light.h
class Light {
private:
    uint type;             // 0=Directional, 1=Point, 2=Spot
    float range;           // [Point/Spot] 減衰距離
    float intensity;       // [Point/Spot] 光の強さ
    float coneAngle;       // [Spot] コーン角度（度数法）
    float falloff;         // [Spot] フォールオフ鋭さ

public:
    // タイプ別のコンストラクタ
    Light(/* Directional */);
    Light(/* Point Light */);
    Light(/* Spot Light */);
    
    // ゲッター/セッター
    void SetLightType(uint t) { type = t; }
    void SetRange(float r) { range = max(0.1f, r); }
    void SetIntensity(float i) { intensity = clamp(i, 0.0f, 1.0f); }
    void SetConeAngle(float angle) { coneAngle = clamp(angle, 1.0f, 180.0f); }
    void SetFalloff(float f) { falloff = clamp(f, 0.1f, 10.0f); }
};
```

#### 3.2 SpotLight クラスへのパラメータ追加
```cpp
// light.h
class SpotLight {
public:
    void SetRange(float r) { 
        if (light) light->SetRange(r); 
    }
    
    void SetConeAngle(float angle) { 
        if (light) light->SetConeAngle(angle); 
    }
    
    void SetFalloff(float f) { 
        if (light) light->SetFalloff(f); 
    }
};
```

---

### **フェーズ 4: shader.cpp の定数バッファ対応** (優先度: 中)

#### 4.1 LIGHT構造体のサイズ確認
```cpp
// shader.cpp
// HLSL の LIGHT 構造体サイズは以下で計算される:
// bool (4) + uint (4) + uint (4) + uint (4) +     // 16 bytes
// float4 (16) + float4 (16) + float4 (16) +       // 48 bytes
// float4 (16) + float (4) + float (4) + float (4) + float (4) // 20 bytes
// 合計: 100 bytes → 128 bytes (16バイト境界)

buffer_desc.ByteWidth = sizeof(LIGHT);  // 128 bytes (パディング含む)
```

#### 4.2 Shader_SetLight 関数の確認
既存の実装でOK（Light*をそのまま設定）

---

### **フェーズ 5: テスト + デバッグ** (優先度: 中)

#### 5.1 テストシーン
1. **スポットライト単体テスト**
   - 懐中電灯オブジェクトを配置
   - コーン角度を30度に設定
   - 距離減衰を視覚的に確認

2. **複合ライティングテスト**
   - 平行光源（太陽） + スポットライト（懐中電灯）
   - 正しく重ね合わさるか確認

3. **エッジケース**
   - range = 0 での動作
   - coneAngle = 180度（全方向）
   - falloff パラメータの値による変化

#### 5.2 パフォーマンス確認
- シェーダーの分岐が多いため、GPU負荷を測定
- 必要に応じて分岐を削減（例：早期リターン活用）

---

## 🛠️ リファクタリング計画

### **R1: シェーダーの再構成**

#### 目標
- ピクセルシェーダーを関数で分割
- ライティング計算の責任分離
- 可読性と保守性の向上

#### 実装内容
```hlsl
// 新しいファイル構成案
// shader_lighting_common.hlsl (共通ライティング関数)
// - CalculateAttenuation()
// - CalculateSpotFalloff()
// - CalculateLambert()
// - CalculateBlinnSpecular()
// - CalculateLighting()

// shader_pixel_2d.hlsl (改良版)
// - 共通ファイルをinclude
// - main() を簡潔に
```

### **R2: HLSL 構造体の効率化**

#### 現在の問題
- パディングが多い
- 型の混在により整列効率が悪い

#### 改善案
```hlsl
// 改良版 LIGHT 構造体
struct LIGHT {
    uint type;              // 0=Directional, 1=Point, 2=Spot
    uint padding[3];        // 明示的パディング
    float4 position;        // ワールド座標 / 方向は direction で使用
    float4 direction;       // ライト方向（正規化済み）
    float4 diffuse;         // RGB + intensity (w)
    float4 ambient;         // RGB + range (w)
    // または
    float4 params;          // x=range, y=intensity, z=coneAngle, w=falloff
};
```

### **R3: 定数バッファの管理改善**

#### 現在の問題
- グローバル静的ポインタが多い
- 初期化順序の依存性

#### 改善案
```cpp
// constbuffer 管理クラスの導入
class ConstantBufferManager {
private:
    std::map<std::string, ID3D11Buffer*> buffers;
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    
public:
    bool Create(const std::string& name, size_t size);
    void Update(const std::string& name, const void* data);
    void SetVS(int slot, const std::string& name);
    void SetPS(int slot, const std::string& name);
};
```

### **R4: ライト管理システム**

#### 将来の複数ライト対応を視野に
```cpp
// ライト管理クラス案
class LightSystem {
private:
    std::vector<Light*> lights;
    AmbientLight* ambientLight;
    SpotLight* spotLight;
    
public:
    void AddLight(Light* light);
    void RemoveLight(Light* light);
    void UpdateLighting(ID3D11DeviceContext* context);
    Light* GetMainLight() const;  // 主要ライト（現在の Light* と同じ）
};
```

---

## 📊 実装スケジュール

| フェーズ | タスク | 工数 | 依存 |
|---------|--------|------|------|
| 1 | HLSL構造体設計 | 2h | - |
| 2 | ピクセルシェーダー実装 | 3-4h | 1 |
| 3 | C++側コード更新 | 2h | 2 |
| 4 | shader.cpp対応 | 1h | 3 |
| 5 | テスト + デバッグ | 2-3h | 4 |
| R1-R4 | リファクタリング | 3-4h | 5 |

**推定総工数**: 13-16時間（分割実施推奨）

---

## ✅ 完了チェックリスト

### 実装フェーズ
- [ ] HLSL LIGHT構造体の設計完了
- [ ] C++ Light クラスの拡張実装
- [ ] ピクセルシェーダーのライティング計算実装
- [ ] shader.cpp の定数バッファ対応
- [ ] コンパイル + 初期動作確認
- [ ] スポットライト単体テスト完了
- [ ] 複合ライティングテスト完了
- [ ] パフォーマンス計測・最適化

### リファクタリングフェーズ
- [ ] シェーダー関数の分割・整理
- [ ] 構造体パディングの最適化
- [ ] 定数バッファ管理クラスの導入（オプション）
- [ ] ドキュメント更新
- [ ] コードレビュー

---

## 🚀 推奨実装順序

### **第1段階（最小限の機能実装）**
1. HLSL の LIGHT 構造体拡張
2. ピクセルシェーダーの距離減衰 + スポットコーン計算
3. Light / SpotLight クラスに新パラメータ追加
4. テスト

### **第2段階（品質向上）**
1. シェーダーコードの整理（共通関数化）
2. パフォーマンス最適化
3. ドキュメント整備

---

## 📝 注意事項

1. **HLSL サイズ計算**: パディングを考慮した 16 バイト整列が必須
2. **初期化順序**: Device / Context の設定タイミングを確認
3. **後方互換性**: 既存の平行光源コードが動作することを確認
4. **テクスチャ絡みのシェーディング**: 光沢度パラメータとの相互作用を確認