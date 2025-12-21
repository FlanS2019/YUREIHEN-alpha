# GitHub Copilot Rules for DX21 Project

## 必須ルール
1. **日本語で返答**
2. **簡潔な説明を交えながら編集をする**
3. **ビルドは指定された場合のみ行う**（デフォルトではビルド実行なし）
4. **参照されたファイルのキャッシュは避ける** - オリジナルのファイルを参照する
5. **説明は不要** - 説明が必要な場合のみ行う
6. **.h、.cppファイル作成時はUTF-8 BOM付きで作成する** - エンコーディング警告を避けるため
7. **HLSLシェーダーファイル（.hlsl）はUTF-8 BOMなしで作成する** - コンパイルエラーを避けるため

## プロジェクト仕様
- **言語**: C++14
- **フレームワーク**: Direct3D 11

## HLSLシェーダーファイルのエンコーディング
- **重要**: HLSLシェーダーファイル（.hlsl）は**UTF-8 BOMなし**で作成する必要がある
- ファイルの先頭バイトが EF BB BF（BOM）の場合、コンパイルエラーが発生する
- **解決方法**：
-- PowerShell で BOM を削除：
     ```powershell
     powershell -NoProfile -Command "[System.IO.File]::ReadAllBytes('shader_pixel_2d.hlsl') | Select-Object -Skip 3 | Set-Content -Path 'shader_pixel_2d.hlsl' -Encoding Byte"
     ```

## 禁止事項
- コードブロックのみの出力（必ずインラインコメント編集を行う）
- ビルド実行（指定された場合のみ実行）
- UTF-8 BOMなしのファイル作成（特に日本語コメントを含む場合）
- HLSLファイルに BOM を含める