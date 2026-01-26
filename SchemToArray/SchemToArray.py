import sys
import os
import json
import subprocess
from datetime import datetime

def install_requirements():
    """必要なライブラリを自動インストール"""
    requirements = {
        'nbtlib': 'nbtlib',
        'numpy': 'numpy'
    }
    
    print("📦 ライブラリのチェックを開始します...\n")
    missing_packages = []
    
    for module_name, package_name in requirements.items():
        try:
            __import__(module_name)
            print(f"✅ {package_name} は既にインストール済みです")
        except ImportError:
            print(f"⚠️  {package_name} が見つかりません。インストールしています...")
            missing_packages.append(package_name)
            
            try:
                subprocess.check_call([sys.executable, "-m", "pip", "install", package_name])
                print(f"✅ {package_name} をインストールしました\n")
            except subprocess.CalledProcessError:
                print(f"❌ {package_name} のインストールに失敗しました")
                print("手動でインストールしてください: pip install " + package_name)
                input("\n⏎ エンターキーを押して終了...")
                sys.exit(1)
    
    if missing_packages:
        print(f"\n✅ 必要なライブラリをすべてインストール完了しました！\n")
    else:
        print("✅ すべてのライブラリが揃っています\n")

# プログラム開始前にライブラリをチェック
install_requirements()

# ここからライブラリをインポート
import nbtlib
import numpy as np

def load_block_definitions(filename):
    """ブロック定義をJSONファイルから読み込む"""
    encodings = ['utf-8-sig', 'shift-jis', 'utf-8', 'cp1252']
    for encoding in encodings:
        try:
            with open(filename, 'r', encoding=encoding) as f:
                return json.load(f)
        except (UnicodeDecodeError, json.JSONDecodeError):
            continue
    raise ValueError(f"{filename} をどのエンコーディングでもデコードできませんでした")

def decode_varint_array(byte_array):
    """
    VarIntエンコードされたバイト配列を整数のリストにデコードする
    Minecraftの .schem 形式では、パレットIDが可変長整数（VarInt）で保存されている
    """
    decoded_ints = []
    i = 0
    length = len(byte_array)
    
    while i < length:
        value = 0
        shift = 0
        while True:
            if i >= length:
                break
            # バイトを符号なし整数として取得 (0-255)
            b = byte_array[i] & 0xFF
            i += 1
            
            # 下位7ビットを値として結合
            value |= (b & 0x7F) << shift
            
            # 最上位ビット(0x80)が0なら、この数値はここで終わり
            if (b & 0x80) == 0:
                break
            shift += 7
            
        decoded_ints.append(value)
    
    return decoded_ints

def block_to_id(block_name, block_properties=None, block_definitions=None):
    """ブロック名とプロパティからIDを取得"""
    if block_definitions is None:
        return 99
    
    # ブロック定義から優先度順にチェック
    for block_def in block_definitions['blocks']:
        def_name = block_def['name']
        # "minecraft:diamond_block" や "diamond_block" の両方に対応
        if def_name in block_name or f"minecraft:{def_name}" in block_name:
            # プロパティ条件がある場合
            if 'conditions' in block_def and block_properties is not None:
                for condition in block_def['conditions']:
                    props = condition.get('properties', {})
                    # すべてのプロパティが一致したかチェック
                    if all(block_properties.get(key) == value for key, value in props.items()):
                        return condition['id']
            
            # デフォルトIDを返す
            return block_def.get('default_id', block_definitions.get('default_id', 99))
    
    return block_definitions.get('default_id', 99)

def build_block_name_mapping(block_definitions):
    """JSONから日本語ブロック名マッピングを構築"""
    block_name_ja = {}
    
    for block_def in block_definitions['blocks']:
        default_id = block_def.get('default_id')
        
        # conditionsがある場合は各条件のIDをマップ
        if 'conditions' in block_def:
            for condition in block_def['conditions']:
                block_id = condition.get('id')
                names_ja = condition.get('names_ja', block_def.get('names_ja', 'unknown'))
                block_name_ja[block_id] = names_ja
        else:
            # conditionsがない場合はdefault_idをマップ
            names_ja = block_def.get('names_ja', 'unknown')
            if default_id == -1:
                # ID -1 の場合はリストで管理
                if -1 not in block_name_ja:
                    block_name_ja[-1] = []
                if isinstance(block_name_ja[-1], list):
                    block_name_ja[-1].append(names_ja)
            else:
                block_name_ja[default_id] = names_ja
    
    return block_name_ja

try:
    # ファイルパスの取得
    if len(sys.argv) > 1:
        SCHEM_FILE = sys.argv[1]
    else:
        print("📁 ファイルパスを入力してください")
        SCHEM_FILE = input("🔹 .schemファイルのパス: ").strip()

    BLOCK_DEFINITIONS_FILE = "block_definitions.json"

    # 入力ファイルの存在確認
    if not os.path.exists(SCHEM_FILE):
        print(f"❌ エラー: {SCHEM_FILE} が見つかりません")
        input("\n⏎ エンターキーを押して終了...")
        sys.exit(1)

    if not os.path.exists(BLOCK_DEFINITIONS_FILE):
        print(f"❌ エラー: {BLOCK_DEFINITIONS_FILE} が見つかりません")
        print("   カレントディレクトリに block_definitions.json を配置してください")
        input("\n⏎ エンターキーを押して終了...")
        sys.exit(1)

    # 出力ファイルパスを生成（schemファイルと同じディレクトリ）
    schem_dir = os.path.dirname(os.path.abspath(SCHEM_FILE))
    schem_basename = os.path.splitext(os.path.basename(SCHEM_FILE))[0]
    output_file = os.path.join(schem_dir, f"{schem_basename}.h")

    # ブロック定義ファイルを読み込む
    print(f"ブロック定義を読み込み: {BLOCK_DEFINITIONS_FILE}")
    block_definitions = load_block_definitions(BLOCK_DEFINITIONS_FILE)

    # 日本語ブロック名マッピングを構築
    block_name_ja = build_block_name_mapping(block_definitions)

    # Schematic読み込み
    print(f"読み込み: {SCHEM_FILE}")
    schem_file = nbtlib.load(SCHEM_FILE)
    schem = schem_file['Schematic']
    width, height, length = int(schem['Width']), int(schem['Height']), int(schem['Length'])
    print(f"サイズ: {width}x{height}x{length}\n")

    # パレット→IDマップ
    blocks_container = schem.get('Blocks', {})
    palette = blocks_container.get('Palette', {}) if isinstance(blocks_container, dict) else {}
    block_id_map = {}

    if palette:
        for name, idx in palette.items():
            props = {}
            if '[' in name and ']' in name:
                props_str = name[name.find('[')+1:name.find(']')]
                for prop in props_str.split(','):
                    if '=' in prop:
                        key, value = prop.split('=')
                        props[key] = value
            
            block_id = block_to_id(name, props, block_definitions)
            idx_int = int(idx)
            block_id_map[idx_int] = block_id
        
        print(f"ブロックマップ生成完了")
    else:
        print("⚠️ Palette が見つかりません")

    # Blocks データを取得
    blocks_container = schem.get('Blocks', {})
    blocks_data = blocks_container.get('Data') if isinstance(blocks_container, dict) else None
    
    if blocks_data is None:
        print("❌ エラー: ブロックデータが見つかりません")
        input("\n⏎ エンターキーを押して終了...")
        sys.exit(1)

    # 【修正】生のバイトデータをVarIntとしてデコードする
    print("🔄 ブロックデータをデコード中...")
    decoded_blocks = decode_varint_array(blocks_data)
    blocks = np.array(decoded_blocks, dtype=np.int32)
    
    # デコード後のデータ数が合っているか確認
    expected_count = width * height * length
    if len(blocks) != expected_count:
        print(f"⚠️ 警告: データサイズが一致しません。")
        print(f"  期待値: {expected_count}")
        print(f"  実測値: {len(blocks)}")
        # 続行はさせるが、警告を出す

    # 縦x横x高さ配列生成
    level_map = np.zeros((height, length, width), dtype=int)

    for y in range(height):
        for z in range(length):
            for x in range(width):
                # 仕様: index = x + z * Width + y * Width * Length
                idx = x + z * width + y * width * length
                if idx < len(blocks):
                    block_idx = int(blocks[idx])
                    block_id = block_id_map.get(block_idx, 99)
                    level_map[y][z][x] = block_id

    print("生成された配列:")
    print("="*60)
    for y in range(height):
        print(f"{{ // Y={y}")
        for z in range(length - 1, -1, -1):
            if z >= 10:
                print("    ... (省略) ...")
                break
            print("    {", end="")
            row = []
            for x in range(width):
                if x >= 10:
                    row.append("...")
                    break
                row.append(f"{int(level_map[y][z][x]):2d}")
            print(",".join(row), end="")
            print(f"}}, // Z={z}")
        print("},")
    print("="*60 + "\n")

    # タイムスタンプを取得
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # 出力ファイルの上書き確認
    if os.path.exists(output_file):
        print(f"⚠️  ファイルが既に存在します: {output_file}")
        response = input("上書きしますか？ (y/n): ").strip().lower()
        if response != 'y':
            print("❌ キャンセルしました")
            input("\n⏎ エンターキーを押して終了...")
            sys.exit(0)

    # Cヘッダ出力
    with open(output_file, "w", encoding="utf-8") as f:
        f.write("#pragma once\n\n")
        f.write(f"// Generated: {timestamp}\n")
        f.write(f"// Source: {SCHEM_FILE}\n")
        f.write(f"// Block IDs:\n")
        for block_id in sorted(block_name_ja.keys()):
            f.write(f"//   {block_id}: {block_name_ja.get(block_id, 'unknown')}\n")
        
        f.write(f"\n#define MAP_LENGTH {length}\n")
        f.write(f"#define MAP_WIDTH {width}\n")
        f.write(f"#define MAP_HEIGHT {height}\n\n")
        f.write(f"static int {schem_basename}[MAP_HEIGHT][MAP_LENGTH][MAP_WIDTH] = {{\n")
        
        for y in range(height):
            f.write(f"    {{ // Y={y}\n")
            for z in range(length - 1, -1, -1):
                f.write("        {")
                row = [f"{int(level_map[y][z][x]):2d}" for x in range(width - 1, -1, -1)]
                f.write(",".join(row))
                f.write(f"}}, // Z={z}\n")
            f.write("    },\n")
        
        f.write("};\n\n")
        f.write(f"// 使用方法: {schem_basename}[y][z][x]\n")

    print("✅ ファイル生成完了！")
    print(f"📂 出力先: {schem_dir}")
    print(f"📄 ファイル: {output_file}")
    print("📏 配列サイズ:", level_map.shape)

    # ブロック分布を見やすく表示
    block_distribution = {}
    for block_id in level_map.flatten():
        block_id_int = int(block_id)
        block_distribution[block_id_int] = block_distribution.get(block_id_int, 0) + 1
    
    print("\n" + "="*50)
    print("📊 ブロック分布詳細")
    print("="*50)
    
    # ID -1 のブロック（仮）を先に出力
    if -1 in block_distribution:
        count = block_distribution[-1]
        if count > 0:
            block_names = block_name_ja.get(-1, 'unknown')
            if isinstance(block_names, list):
                print(f"ID {-1:3d}: " + "\n       ".join(block_names))
                print(f"         × {count:5d}個")
            else:
                print(f"ID {-1:3d}: {block_names:15s} × {count:5d}個")
    
    # ID 0 以上のブロックを出力
    print("-"*50)
    for block_id in sorted([bid for bid in block_distribution.keys() if bid >= 0]):
        count = block_distribution[block_id]
        if count > 0:
            block_name = block_name_ja.get(block_id, 'unknown')
            if not isinstance(block_name, list):
                print(f"ID {block_id:3d}: {block_name:15s} × {count:5d}個")
    print("="*50)

    # エンターキーで終了
    input("\n⏎ エンターキーを押して終了...")

except Exception as e:
    print(f"\n❌ 予期しないエラーが発生しました:")
    print(f"   {type(e).__name__}: {e}")
    input("\n⏎ エンターキーを押して終了...")
    sys.exit(1)