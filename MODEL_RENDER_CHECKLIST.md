# モデル描画トラブルシューティングチェックリスト

## ?? 描画されない場合の診断フロー

### ステップ 1: コンソール出力確認
実行時に以下の情報が出力されているか確認：

```
Mesh 0: XXX indices (YYY faces)
Mesh 1: XXX indices (YYY faces)
...
```

- **出力あり**: インデックス数が正しく計算されている ?
- **出力なし**: ModelLoad関数が正常に完了していない ?

### ステップ 2: モデルファイルの確認

| 項目 | 確認内容 | 修正方法 |
|------|---------|---------|
| **ファイルパス** | `asset\model\chest.fbx` が存在するか | パスを確認・修正 |
| **ファイルサイズ** | 0バイトではないか | ファイルを再ダウンロード |
| **FBX形式** | Binary形式か（テキスト形式は非対応） | Maya側でBinary出力を確認 |

### ステップ 3: Maya FBXエクスポート設定確認

**fbxexport.mdを参照して以下を確認:**

- ? `aiProcess_Triangulate` - 四角形以上が三角形化されているか
- ? `aiProcess_GenSmoothNormals` - 法線が生成されているか
- ? `aiProcess_ConvertToLeftHanded` - 座標系が左手系に変換されているか

### ステップ 4: シェーダー定数バッファ設定

**確認項目:**

```cpp
// shader.cpp: Shader_Begin()で以下が実行されているか
g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer);  // WVP行列
g_pContext->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer); // ライト
```

- **実行されている**: 定数バッファが正しく設定されている ?
- **実行されていない**: Shader_Begin()がコールされていない ?

### ステップ 5: ワールド行列の検証

**RenderNode内での行列セット:**

```cpp
Shader_SetWorldMatrix(currentTransform);  // メッシュごとの変換
```

- **セットされている**: 各メッシュの変換が独立している ?
- **セットされていない**: 全メッシュが同じ位置に描画される ?

### ステップ 6: テクスチャの確認

**Mayaモデルにテクスチャが埋め込まれているか:**

```
FBXExport Options:
□ Materials: ?
□ Textures: ?
□ Embed Media: ? （推奨）
```

- **テクスチャなし**: 白いモデルが描画される（エラーなし）
- **テクスチャあり**: テクスチャが適用された状態で描画

---

## ?? デバッグ出力例

### 正常な場合

```
Mesh 0: 1200 indices (400 faces)
Mesh 1: 2400 indices (800 faces)
Mesh 2: 600 indices (200 faces)
```

### 問題がある場合

```
Mesh 0: 0 indices (400 faces)  ← インデックス数が0！
```

**原因**: 四角形以上のポリゴンが削除されている可能性
**対応**: `aiProcess_Triangulate`フラグが正しく機能しているか確認

---

## ?? 行列計算フロー（重要）

```
1. モデル行列: World = Scale × Rotation × Translation
2. WVP行列:    WVP = World × View × Projection
3. ワールド行列: 各メッシュごとにShader_SetWorldMatrix()で設定

正しい順序:
VS: posH = mul(posL, WVP)  // 頂点座標変換
    normal変換用に worldMtx も使用
```

---

## ?? よくある問題と解決策

| 問題 | 原因 | 解決方法 |
|------|------|---------|
| **真っ黒** | ライト無効化 / カメラ位置 | game.cppでLight.SetEnable(true) |
| **描画されない** | インデックス数0 / メッシュなし | コンソール出力確認 |
| **テクスチャがない** | テクスチャ埋め込みなし | Embed Mediaオプション有効化 |
| **法線反転（面が暗い）** | 面の向きエラー | Maya側でMesh→Reverseで統一 |
| **ポリゴンの隙間** | 座標系不一致 | aiProcess_ConvertToLeftHanded確認 |

---

## ?? デバッグプリント追加（モデル読み込み後）

model.cppのModelLoad内にデバッグコードを追加：

```cpp
std::cout << "=== Model Load Debug ===" << std::endl;
std::cout << "Total Meshes: " << model->AiScene->mNumMeshes << std::endl;
std::cout << "Total Textures: " << model->AiScene->mNumTextures << std::endl;
std::cout << "Root Node: " << model->AiScene->mRootNode->mName.data << std::endl;

for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
{
    aiMesh* mesh = model->AiScene->mMeshes[m];
    std::cout << "Mesh " << m << ": " 
              << mesh->mNumVertices << " vertices, "
              << mesh->mNumFaces << " faces, "
              << (mesh->HasNormals() ? "HAS_NORMALS" : "NO_NORMALS") << std::endl;
}
```

---

## ? チェックリスト（実行前）

```
□ asset\model\chest.fbx が存在するか確認
□ コンソール出力でインデックス数を確認
□ Shader_Begin()が呼ばれているか確認
□ Shader_SetLight()でLight.enableがtrueか確認
□ SetDepthTest(true)で深度テストが有効か確認
□ camera.GetView()とGetProjection()が正しい値を返しているか
□ モデル表示座標がカメラのビュー内か確認
□ FBXファイルがTriangulate処理されているか確認（コンソール出力）
```

---

## ?? 実行時コマンド

| キー | 動作 |
|------|------|
| **W** | カメラ前進 |
| **A/D** | カメラ左右移動 |
| **S** | カメラ後退 |
| **Space** | カメラ上昇 |
| **Shift** | カメラ下降 |
| **R** | カメラリセット |
| **ESC** | マウスロック解除 |
| **マウス移動** | 視点回転 |

---

## 参考資料

- `fbxexport.md`: Maya FBX出力設定ガイド
- `model.cpp`: FBXロードと描画処理
- `shader_vertex_2d.hlsl` / `shader_pixel_2d.hlsl`: シェーダー実装

