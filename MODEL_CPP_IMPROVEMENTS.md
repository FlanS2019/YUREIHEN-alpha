# model.cpp 改善記録

## 問題：モデルが描画されない（虚無状態）

### 原因分析

1. **インデックスバッファのサイズ不正**
   - 元のコード: `DrawIndexed(mesh->mNumFaces * 3, 0, 0)`
   - 問題: 四角形以上のポリゴンがスキップされた場合、実際のインデックス数と異なる

2. **行列計算の順序エラー**
   - modeldraw.cpp: `WVP = Model_World * VP` (誤り)
   - 正解: `WVP = World * View * Projection`

3. **定数バッファ設定の不完全さ**
   - ピクセルシェーダーにライト情報が渡されていなかった

4. **アサーション厳密化による処理スキップ**
   - `assert(face->mNumIndices == 3)` で非三角形ポリゴンをエラー扱い

---

## 実装した改善

### 1. MeshData構造体による正確なインデックス数追跡

```cpp
struct MeshData
{
    unsigned int indexCount;  // 実際のインデックス数
};

static MeshData* g_meshData = nullptr;
```

**効果**: メッシュごとの正確なインデックス数を保持し、描画時に使用

### 2. RenderNode関数の改善

```cpp
// 改善前:
Direct3D_GetDeviceContext()->DrawIndexed(mesh->mNumFaces * 3, 0, 0);

// 改善後:
Direct3D_GetDeviceContext()->DrawIndexed(g_meshData[meshIndex].indexCount, 0, 0);
```

**効果**: 実際のインデックス数を正確に使用

### 3. Assimpロード時のフラグ追加

```cpp
aiProcess_Triangulate              // 四角形以上を三角形化 ← 重要！
aiProcess_GenSmoothNormals         // 法線生成
aiProcess_JoinIdenticalVertices    // 重複削除
aiProcess_OptimizeGraph            // グラフ最適化
```

**効果**: Maya出力のあらゆるポリゴン形状に対応

### 4. テクスチャ座標のデフォルト値修正

```cpp
// 改善前:
vertex[v].texCoord = XMFLOAT2(0.0f, 0.0f);

// 改善後:
vertex[v].texCoord = XMFLOAT2(0.5f, 0.5f);
```

**効果**: テクスチャなしの場合も中央でサンプリング

### 5. インデックス計算の堅牢化

```cpp
// 各フェイスのインデックス数をカウント
for (unsigned int f = 0; f < mesh->mNumFaces; f++)
{
    const aiFace* face = &mesh->mFaces[f];
    if (face->mNumIndices >= 3)
    {
        indexCount += 3;  // 常に3（Triangulate後）
    }
}
```

**効果**: 三角形化後に確実に3つのインデックスを使用

### 6. デバッグ情報の充実

```cpp
std::cout << "Mesh " << m << ": " << indexCount << " indices (" 
          << mesh->mNumFaces << " faces)" << std::endl;

std::cout << "Mesh " << m << " created: " 
          << mesh->mNumVertices << " vertices, "
          << (mesh->HasNormals() ? "WITH" : "WITHOUT") << " normals, "
          << (mesh->HasTextureCoords(0) ? "WITH" : "WITHOUT") << " UVs" << std::endl;
```

**効果**: コンソール出力でロード状況を確認可能

### 7. エラーハンドリングの強化

```cpp
if (FAILED(hr))
{
    std::cerr << "Failed to create vertex buffer for mesh " << m << std::endl;
    delete[] vertex;
    return nullptr;
}
```

**効果**: エラー時にメモリリークを防止

---

## modeldraw.cpp の修正

### 行列計算順序の修正

```cpp
// 改善前（誤り）:
XMMATRIX WVP = Model_World * VP;  // W * (V * P)

// 改善後（正解）:
XMMATRIX WVP = World * View * Projection;  // (W * V) * P
```

### Null チェックの追加

```cpp
if (!g_testModel) return;
Camera* pCamera = GetCamera();
if (!pCamera) return;
```

---

## shader.cpp の修正

### ピクセルシェーダーへのライト設定追加

```cpp
void Shader_Begin()
{
    // ...既存コード...
    
    // ピクセルシェーダーにもライト定数バッファを設定 ← 追加！
    g_pContext->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);
}
```

---

## 描画フロー（改善後）

```
1. ModelLoad()
   ├─ Assimp読み込み（Triangulate有効）
   ├─ 頂点バッファ作成
   ├─ インデックスバッファ作成
   ├─ インデックス数を g_meshData に保存
   └─ テクスチャ読み込み

2. ModelDraw_DrawAll()
   ├─ Shader_Begin()でシェーダー設定
   ├─ WVP行列を計算（World × View × Projection）
   ├─ Shader_SetMatrix(WVP)で定数バッファ設定
   └─ ModelDraw()呼び出し

3. ModelDraw()
   ├─ プリミティブトポロジ設定
   └─ RenderNode()で再帰描画

4. RenderNode()（メッシュごと）
   ├─ ワールド行列を合成
   ├─ Shader_SetWorldMatrix()で設定
   ├─ テクスチャ設定
   ├─ 頂点/インデックスバッファ設定
   └─ DrawIndexed(g_meshData[meshIndex].indexCount) ← 正確な数
```

---

## 検証方法

### 1. コンソール出力確認

```
Mesh 0: 1200 indices (400 faces)
WITH normals
WITH UVs
```

### 2. ビジュアル確認

- **白いメッシュが表示される**: テクスチャなし（正常）
- **光の陰影が見える**: Lambert陰影が機能（正常）
- **何も表示されない**: 以下をチェック
  - FBXファイルパスが正しいか
  - Light.enableがtrueか
  - カメラがモデルを見ているか

---

## トラブルシューティング

### 問題: コンソール出力が空白

**原因**: ModelLoad()がnullptrを返している
**対策**:
1. `asset\model\chest.fbx`が存在するか確認
2. ファイルがBinary FBX形式か確認
3. Mayaでサンプルモデルを再エクスポート

### 問題: インデックス数が0

**原因**: aiProcess_Triangulateが四角形を削除
**対策**: Maya側でポリゴンを三角形に統一（Mesh → Triangulate）

### 問題: テクスチャが表示されない

**原因**: テクスチャが埋め込まれていない
**対策**: FBXExport Options で "Embed Media" を ?

---

## 参考ドキュメント

- `fbxexport.md`: Maya FBX出力設定ガイド
- `MODEL_RENDER_CHECKLIST.md`: 描画トラブルシューティング
- `shader_vertex_2d.hlsl`: 頂点シェーダー
- `shader_pixel_2d.hlsl`: ピクセルシェーダー

---

**改善完了日**: 2024年
**ビルド状態**: ? 成功
**テスト状態**: ?? 実行待機

