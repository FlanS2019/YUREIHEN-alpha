# PointLight 実装まとめ

## データ定義（`light.h`）
- `PointLightData`：シェーダー定数バッファ向けの構造体。`position`/`direction`/`diffuse` と `params(x=range, y=intensity)` を保持。
- `LightData`：`PointLightData` の配列（`MAX_POINT_LIGHTS=16`）と `lightCount`、`ambient` を保持。
- `PointLight` クラス：有効フラグ、位置、方向、拡散色、範囲、強度を持ち、`SetDirection` で正規化処理を行う。

## シェーダー側（`shader_vertex_2d.hlsl` / `shader_pixel_2d.hlsl`）
- `Buffer2` に `POINT_LIGHT` 配列、`PointLightCount`、`AmbientColor` を保持。
- ピクセルシェーダーで `PointLightCount` の範囲でループし、
  - `Position.w > 0.5` の場合はポイントライトとして距離減衰（範囲内のみ）を計算。
  - それ以外は方向ライト扱いとして Lambert を加算。
- 最終色は `diffuseAccum + AmbientColor` で乗算。

## ライト登録とカリング（`shader.cpp`）
- `Shader_AddPointLight` で `PointLight` を候補に追加し、カメラからの距離二乗を保持。
- `Shader_FlushLights` で候補数が上限超過時に近距離順で `partial_sort`。
- 上位 `MAX_POINT_LIGHTS` 件を `g_CurrentLightData.pointLights` に詰めて `g_pLightConstantBuffer` を更新。
- `Shader_SetCameraPos` でカリング用カメラ位置を更新。

## 利用箇所（実装例）
- `Furniture`（`furniture.h` / `furniture.cpp`）
  - 壁掛けライト（ID:70）で `PointLight` を生成。
  - `Furniture_SetLight` から `Shader_AddPointLight` に登録。
- `UI_Tutorial_SetLight`（`UI_Tutorial.cpp`）
  - チュートリアルページ用の `PointLight` を更新し、`Shader_AddPointLight` に登録。

## まとめ
- `PointLight` は CPU 側で候補登録 → 近距離順に上限内へ詰め替え → 定数バッファ更新 → HLSL で距離減衰付きの拡散計算、という流れで処理される。