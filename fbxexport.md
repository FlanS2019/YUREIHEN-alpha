# Maya FBX出力設定ガイド - DirectX対応

## 概要
このドキュメントはMayaで作成したモデルをFBX形式でエクスポートし、DirectX11を使用するゲームエンジンで正しく読み込むための設定ガイドです。

---

## 1. Maya プロジェクト設定

### 1.1 作業単位の設定
**Window → Preferences → Settings**

| 項目 | 設定値 | 理由 |
|------|-------|------|
| Linear | cm | DirectX標準スケール |
| Angular | degree | 回転値の統一 |
| Time | 24fps | アニメーション標準 |

### 1.2 座標系の理解

| 項目 | Maya | DirectX | 対応 |
|------|------|---------|------|
| **Up軸** | Y軸（デフォルト） | Z軸 | `aiProcess_ConvertToLeftHanded` |
| **手型** | 右手系 | 左手系 | 変換済み |
| **テクスチャV軸** | 上向き | 下向き | 必要に応じて反転 |

**注意**: Maya側ではY-upのままで構いません。Assimp（FBX読み込みライブラリ）が自動変換します。

---

## 2. モデルの準備

### 2.1 メッシュの最適化
1. **ヒストリの削除**
   - Edit → Delete All by Type → History

2. **法線の統一**
   - Mesh → Cleanup
   - Display → Polygons → Face Normals（確認）

3. **頂点の統合**
   - Mesh → Merge Vertices

4. **オブジェクトの統合（オプション）
   - Mesh → Combine

### 2.2 UVマッピングの確認
- UV Editor で全頂点がUVマップされていることを確認
- テクスチャが正しく適用されているか確認
- オーバーラップがないか確認

---

## 3. マテリアル・テクスチャ設定（Hypershade）

### 3.1 基本設定
1. **Hypershadeを開く**
   - Windows → Shading Editors → Hypershade

2. **マテリアル作成**
   - 推奨: **Phong** または **Standard Surface**

### 3.2 テクスチャ接続設定

```
Phong / Standard Surface マテリアル構成:

┌─────────────────────────────────────────┐
│ Color (ディフューズ)                      │
│ ├─ File → diffuse.png                   │
│ └─ Connected                            │
├─ Bump Map (法線情報)                     │
│ ├─ File → normal.png                    │
│ └─ Bump Depth: 1.0                      │
├─ Specular Color (スペキュラー)            │
│ ├─ File → specular.png                  │
│ └─ Connected                            │
└─────────────────────────────────────────┘
```

### 3.3 テクスチャファイルの配置

**推奨フォルダ構成:**
```
MyProject/
│
├─ models/
│  ├─ character/
│  │  ├─ model.fbx          ← エクスポート
│  │  └─ Textures/
│  │     ├─ diffuse.png      ← ディフューズマップ
│  │     ├─ normal.png       ← 法線マップ
│  │     ├─ specular.png     ← スペキュラーマップ
│  │     └─ roughness.png    ← ラフネスマップ（オプション）
│  │
│  └─ stage/
│     ├─ stage.fbx
│     └─ Textures/
│        └─ ...
│
└─ src/
   └─ model.cpp             ← 読み込みコード
```

### 3.4 テクスチャ参照設定

**File テクスチャノード設定:**
- **Texture path**: 相対パスで指定
  - `./Textures/diffuse.png` または `Textures/diffuse.png`
- **Use Image Sequence**: OFF
- **Color Space**: 
  - Color/Diffuse: sRGB
  - Normal Map: Raw (Linear)
  - Specular: Linear

---

## 4. FBXエクスポート設定

### 4.1 エクスポート開始

**File → Export Selection** または **File → Export All**

### 4.2 FBXExport Options（詳細設定）

#### **ジオメトリー（Geometry）**
| 項目 | 設定 | 理由 |
|------|------|------|
| Smooth Mesh | ? | スムーズシェーディング |
| Polygon Meshes | ? | ポリゴンメッシュ対応 |
| Smooth Mesh Preview | ? | プレビュー結果を出力 |
| Referenced Assets | ? | 参照アセット含む |

#### **アニメーション（Animation）**
| 項目 | 設定 | 理由 |
|------|------|------|
| Bake Animation | ? | キーフレームベイク |
| Deformed Models | ? | スキン/リグ対応 |
| Skins | ?| スケルトン保存 |
| Blend Shapes | ? | モーフターゲット対応 |

#### **マテリアル・テクスチャ（Materials and Textures）**
| 項目 | 設定 | 理由 |
|------|------|------|
| Materials | ? | マテリアル情報保存 |
| Textures | ? | テクスチャ情報保存 |
| Embed Media | ? 推奨 | テクスチャ埋め込み |
| Use Smooth Mesh | ? | スムーズメッシュ使用 |

#### **頂点属性（Vertex Attributes）**
| 項目 | 設定 | 理由 |
|------|------|------|
| Normals | ? | 法線情報 |
| Tangents and Binormals | ? | 法線マッピング用 |
| Colors | ? | 頂点カラー対応 |
| Smooth Groups | ? | スムーズグループ |

#### **その他（Misc）**
| 項目 | 設定 | 理由 |
|------|------|------|
| Preserve Edge Smoothness | ? | エッジ滑らかさ保持 |
| Animation Sampling | 1.0 | フレームレート |
| Deformed Models | ? | 変形モデル対応 |

### 4.3 エクスポート実行

```
1. File → Export Selection (or Export All)
2. ファイル名: model.fbx
3. ファイル形式: FBX Binary (*.fbx)
4. オプション設定（上記参照）
5. Export
```

---

## 5. DirectX側の対応

### 5.1 Assimpロード設定

現在のmodel.cpp設定:
```cpp
model->AiScene = aiImportFile(FileName, 
    aiProcessPreset_TargetRealtime_MaxQuality | 
    aiProcess_ConvertToLeftHanded
);
```

**推奨追加フラグ:**
```cpp
model->AiScene = aiImportFile(FileName, 
    aiProcessPreset_TargetRealtime_MaxQuality | 
    aiProcess_ConvertToLeftHanded |
    aiProcess_GenSmoothNormals |           // スムーズ法線生成
    aiProcess_JoinIdenticalVertices |      // 重複頂点削除
    aiProcess_OptimizeGraph                // グラフ最適化
);
```

### 5.2 テクスチャロード

FBXファイルに埋め込まれたテクスチャ:
```cpp
// model.cppで自動処理
for (unsigned int i = 0; i < model->AiScene->mNumTextures; i++)
{
    aiTexture* aitexture = model->AiScene->mTextures[i];
    // LoadFromWICMemory() で読み込み
}
```

### 5.3 マテリアル情報取得

```cpp
aiMaterial* material = model->AiScene->mMaterials[mesh->mMaterialIndex];

// ディフューズテクスチャ
aiString diffusePath;
material->GetTexture(aiTextureType_DIFFUSE, 0, &diffusePath);

// 法線マップ（オプション）
aiString normalPath;
material->GetTexture(aiTextureType_HEIGHT, 0, &normalPath);

// スペキュラーマップ（オプション）
aiString specularPath;
material->GetTexture(aiTextureType_SPECULAR, 0, &specularPath);
```

---

## 6. トラブルシューティング

### 6.1 テクスチャが読み込めない

**原因**: ファイルパス不一致

**対策**:
1. FBXと同じフォルダに `Textures/` を配置
2. Maya側で相対パスを使用
3. Embed Mediaオプションを有効化

### 6.2 モデルが反転している / 左右反対

**原因**: 座標系変換の不一致

**対策**:
```cpp
// model.cppで既に対応済み
aiProcess_ConvertToLeftHanded
```

MayaでY-up出力していることを確認

### 6.3 法線がおかしい / 表面がギラギラしている

**原因**: 面の向きエラー

**対策**:
1. Maya側で **Mesh → Reverse** で面の向きを統一
2. **Display → Polygons → Face Normals** で視認
3. Hypershadeで **Double Sided** を確認

### 6.4 アニメーションが再生されない

**原因**: ベイク設定漏れ

**対策**:
1. FBXExport Options で **Bake Animation** ?
2. アニメーション期間を正しく設定
3. キーフレームがロックされていないか確認

### 6.5 スケール・サイズが正しくない

**原因**: 単位設定不一致

**対策**:
1. Maya: Preferences → Linear = cm
2. モデルを Freeze Transform してからエクスポート
3. DirectX側で必要に応じてスケール調整

### 6.6 ボーン/スキンが認識されない

**原因**: スキンバインドエラー

**対策**:
1. Skin → Bind Skin でバインド確認
2. Skin Weights がロックされていないか確認
3. FBXExport Options で **Skins** ?

---

## 7. ベストプラクティス

### 7.1 ファイル管理
- ? モデルファイルとテクスチャは同じフォルダ構成を保つ
- ? テクスチャは `Textures/` サブフォルダに整理
- ? ファイル名に日本語を使用しない（パスエラー回避）

### 7.2 モデル作成時
- ? ポリゴン数は適切に（ゲーム機での負荷を考慮）
- ? 不要な頂点を削除
- ? 重複メッシュを統合
- ? ヒストリを削除してからエクスポート

### 7.3 テクスチャ設定
- ? テクスチャサイズを統一（512x512, 1024x1024など）
- ? テクスチャ形式: PNG, TGA（アルファ対応）
- ? Color Spaceを正しく設定（sRGB vs Linear）

### 7.4 アニメーション
- ? アニメーション開始フレームを0に設定
- ? キーフレーム間隔を一定に保つ
- ? 不要なキーフレームを削除

---

## 8. チェックリスト（エクスポート前）

```
□ ヒストリを削除した
□ すべての頂点がUVマップされている
□ マテリアル/テクスチャが正しく設定されている
□ 法線が統一されている
□ スケールが正しい（Working Units = cm）
□ すべてのメッシュが Freeze Transform されている
□ ボーン/アニメーションが正しく設定されている（アニメーション使用時）
□ モデルに不要な非表示オブジェクトがない
□ テクスチャファイルパスが相対パスに設定されている
□ Embed Media オプションが有効（推奨）
```

---

## 9. DirectX統合のサンプルコード

### 9.1 モデル読み込み（model.cpp）- 改善版

```cpp
// FBXファイル読み込み時の最適フラグセット
model->AiScene = aiImportFile(FileName,
    aiProcessPreset_TargetRealtime_MaxQuality |
    aiProcess_ConvertToLeftHanded |
    aiProcess_Triangulate |              // 四角形以上を自動的に三角形化 ← 重要
    aiProcess_GenSmoothNormals |         // スムーズ法線生成
    aiProcess_JoinIdenticalVertices |    // 重複頂点削除
    aiProcess_OptimizeGraph              // グラフ最適化
);
```

**各フラグの説明:**

| フラグ | 説明 |
|-------|------|
| `aiProcess_Triangulate` | **四角形以上を三角形に変換**（DirectX対応） |
| `aiProcess_ConvertToLeftHanded` | 右手系から左手系に変換 |
| `aiProcess_GenSmoothNormals` | 法線を生成（ない場合） |
| `aiProcess_JoinIdenticalVertices` | 重複頂点を統合 |
| `aiProcess_OptimizeGraph` | ノード階層を最適化 |

### 9.2 エラーハンドリングの改善

```cpp
// テクスチャ座標の存在確認
if (mesh->HasTextureCoords(0))
{
    vertex[v].texCoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, 
                                  mesh->mTextureCoords[0][v].y);
}
else
{
    vertex[v].texCoord = XMFLOAT2(0.0f, 0.0f);  // デフォルト値
}

// 法線の存在確認
if (mesh->HasNormals())
{
    vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, 
                                mesh->mNormals[v].y, 
                                mesh->mNormals[v].z);
}
else
{
    vertex[v].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);  // 上向きデフォルト
}
```

### 9.3 マテリアル・テクスチャ取得

```cpp
// RenderNode内でテクスチャ取得
aiMaterial* material = model->AiScene->mMaterials[mesh->mMaterialIndex];

// ディフューズテクスチャ
aiString texturePath;
if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
{
    if (model->Texture.count(texturePath.data))
    {
        Direct3D_GetDeviceContext()->PSSetShaderResources(
            0, 1, &model->Texture[texturePath.data]
        );
    }
}
```

---

## 10. model.cpp改善点の詳細

### 10.1 アサーション削除による柔軟性向上

**変更前:**
```cpp
assert(face->mNumIndices == 3);  // ← Mayaモデルで失敗
```

**変更後:**
```cpp
if (face->mNumIndices == 3)
{
    // 通常の三角形処理
}
else if (face->mNumIndices > 3)
{
    // 警告を出して最初の三角形のみ使用
    std::cerr << "Warning: Face has " << face->mNumIndices << " indices";
}
```

### 10.2 改善されたFBXロードプロセス

```cpp
処理フロー:

1. FBXファイル読み込み
   ↓
2. Triangulate処理（四角形→三角形化）✓ 新規
   ↓
3. 法線生成・最適化
   ↓
4. 頂点/インデックスバッファ作成
   ↓
5. テクスチャロード
   ↓
6. メモリ解放
```

### 10.3 Null チェックの追加

```cpp
// ModelDraw時の安全性向上
void ModelDraw(MODEL* model)
{
    if (!model) return;  // ← Null チェック追加
    
    // ...処理...
}

// ModelRelease時のリソース保護
void ModelRelease(MODEL* model)
{
    if (!model) return;  // ← Null チェック追加
    
    // ...解放処理...
}
```

---

## 11. Lambertシェーダー実装ガイド

### 11.1 Lambert陰影とは

**Lambert陰影** は最も基本的なリアルタイムライティング手法です。

```
Lambert計算式:
Brightness = max(0, dot(Normal, LightDirection))

実装ストラテジー:
1. Gouraud Shading（頂点シェーダー）- 低品質・高速
2. Per-Pixel Lighting（ピクセルシェーダー）✓推奨 - 高品質・中速
```

### 11.2 現在のシェーダー構成

**頂点シェーダー（Per-Pixel対応版）:**
```hlsl
struct VS_OUTPUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
    float4 normal : NORMAL0;        // ピクセルシェーダーに渡す
    float4 worldPos : TEXCOORD1;    // ワールド座標
};
```

**ピクセルシェーダー（Lambert計算）:**
```hlsl
float3 normal = normalize(ps_in.normal.xyz);
float3 lightDir = normalize(-Light.Direction.xyz);
float lambert = max(dot(normal, lightDir), 0.0f);
float3 diffuse = lambert * Light.Diffuse.rgb;
float3 ambient = Light.Ambient.rgb;
finalColor.rgb *= (diffuse + ambient);
```

### 11.3 Mayaモデルでの確認ポイント

Lambert陰影を正しく機能させるための確認項目：

| 項目 | 確認内容 | 対応方法 |
|------|---------|---------|
| **法線情報** | 法線が正しく計算されているか | Maya側で法線を統一 |
| **テクスチャUV** | UVマッピングが正しいか | UV Editor で確認 |
| **ライト方向** | ライトが適切に設定されているか | game.cpp で方向を設定 |
| **マテリアル色** | マテリアルのディフューズ色 | Hypershade で確認 |
| **ポリゴン形状** | すべてのポリゴンが三角形か | Triangulateで自動化 ✓ |

### 11.4 トラブルシューティング（Lambertシェーダー）

| 問題 | 原因 | 解決方法 |
|------|------|---------|
| **全体が真っ黒** | ライト無効化 / 法線逆向き | Light.enableをtrueに / 法線反転 |
| **全体が明るすぎる** | アンビエント値が大きい | Light.Ambient値を低減 |
| **面が均一で立体感がない** | ポリゴン数が少ない（Gouraud） | Per-Pixel Lightingを使用 ✓ |
| **明暗が反転している** | 光の方向が逆 | Direction値の符号を反転 |
| **テクスチャが見えない** | テクスチャロード失敗 | ファイルパス確認 |
| **アサーション失敗（face->mNumIndices）** | 非三角形ポリゴン | aiProcess_Triangulate追加 ✓ 解決 |

### 11.5 ライト設定（game.cpp例）

```cpp
MainLight = new Light
(TRUE,
    XMFLOAT4(0.0f, -10.0f, 0.0f, 1.0f),    // ライト方向（上から照射）
    XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f),       // ディフューズ色（白い光）
    XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f)        // アンビエント（環境光・影の明るさ）
);
```

### 11.6 推奨される拡張機能

将来的に実装を検討できる機能：

- ✓ **Phong Shading**: スペキュラーハイライト追加
- ✓ **Normal Mapping**: 法線マップでディテール向上
- ✓ **Shadow Mapping**: 影の計算
- ✓ **Parallax Mapping**: 凹凸感の向上

---

## 12. 参考リソース

- **Assimp Documentation**: https://assimp-docs.readthedocs.io/
- **DirectXMath**: Microsoft DirectX SDK
- **Maya FBX Plugin**: Autodesk公式ドキュメント
- **OpenGL/DirectXコーディネート**: Khronos Wiki
- **Lambert Shading**: https://en.wikipedia.org/wiki/Lambertian_reflectance

---

## 13. 更新履歴

| 日付 | 変更内容 |
|------|---------|
| 2024年 | 初版作成 |
| 2024年 | Lambertシェーダー実装ガイド追加 |
| 2024年 | model.cpp改善版（aiProcess_Triangulate対応） |

---

**注記**: このドキュメントはDirectX11 + Assimpを前提とした設定ガイドです。
異なるバージョンやレンダラーを使用する場合は、適宜調整してください。
