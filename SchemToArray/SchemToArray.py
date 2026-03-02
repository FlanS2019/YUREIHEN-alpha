# このプログラムは基本的にマイクラサーバー上で動作するので、ローカルでの実行は行わない前提で作っています。

import sys
import os
import json
import subprocess
import logging
from datetime import datetime, timezone, timedelta  # タイムゾーン用に追加
import urllib.request


# ロガー設定
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


def install_requirements():
    """必要なライブラリを自動インストール"""
    requirements = {
        'nbtlib': 'nbtlib',
        'numpy': 'numpy'
    }
    
    logger.info("ライブラリのチェックを開始します...")
    missing_packages = []
    
    for module_name, package_name in requirements.items():
        try:
            __import__(module_name)
        except ImportError:
            logger.warning(f"{package_name} が見つかりません。インストールしています...")
            missing_packages.append(package_name)
            try:
                subprocess.check_call([sys.executable, "-m", "pip", "install", package_name, "-q"])
                logger.info(f"{package_name} をインストールしました")
            except subprocess.CalledProcessError:
                logger.error(f"{package_name} のインストールに失敗しました")
                sys.exit(1)


install_requirements()


import nbtlib
import numpy as np


def load_block_definitions():
    """ブロック定義を GitHub から取得する (BOM対応)"""
    url = "https://raw.githubusercontent.com/realryo1/YUREIHEN-alpha/refs/heads/master/SchemToArray/block_definitions.json"
    
    logger.info(f"ブロック定義を取得中: {url}")
    
    try:
        with urllib.request.urlopen(url, timeout=10) as response:
            raw_data = response.read()
            # UTF-8 BOM対応 (utf-8-sig)
            data = raw_data.decode('utf-8-sig')
            return json.loads(data)
    except Exception as e:
        logger.error(f"ブロック定義の取得中にエラーが発生しました: {e}")
        sys.exit(1)


def decode_varint_array(byte_array):
    """VarIntエンコードされたバイト配列を整数のリストにデコードする"""
    decoded_ints = []
    i = 0
    length = len(byte_array)
    
    while i < length:
        value = 0
        shift = 0
        while True:
            if i >= length:
                break
            b = byte_array[i] & 0xFF
            i += 1
            value |= (b & 0x7F) << shift
            if (b & 0x80) == 0:
                break
            shift += 7
        decoded_ints.append(value)
    return decoded_ints


def block_to_id(block_name, block_properties, block_definitions):
    """ブロック名とプロパティからIDを取得"""
    if block_definitions is None:
        return 99
    
    for block_def in block_definitions['blocks']:
        def_name = block_def['name']
        if def_name == block_name or f"minecraft:{def_name}" == block_name:
            if 'conditions' in block_def and block_properties:
                for condition in block_def['conditions']:
                    props = condition.get('properties', {})
                    if all(block_properties.get(k) == v for k, v in props.items()):
                        return condition['id']
            return block_def.get('default_id', 99)
            
    return block_definitions.get('default_id', 99)


def build_block_name_mapping(block_definitions):
    """JSONから日本語ブロック名マッピングを構築"""
    block_name_ja = {}
    for block_def in block_definitions['blocks']:
        default_id = block_def.get('default_id')
        if 'conditions' in block_def:
            for condition in block_def['conditions']:
                block_id = condition.get('id')
                names_ja = condition.get('names_ja', block_def.get('names_ja', 'unknown'))
                block_name_ja[block_id] = names_ja
        else:
            names_ja = block_def.get('names_ja', 'unknown')
            if default_id != -1:
                block_name_ja[default_id] = names_ja
    return block_name_ja


def backup_to_google_drive(local_file_path):
    """rcloneでGoogle Driveへバックアップ"""
    REMOTE_PATH = "realryo1:Violisun共有フォルダ/フィールド"
    
    if not os.path.exists(local_file_path):
        return False
        
    try:
        logger.info(f"Google Drive へバックアップ中... {local_file_path}")
        cmd = ["rclone", "copy", local_file_path, REMOTE_PATH]
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            logger.info("✅ Google Drive バックアップ完了")
            return True
        else:
            logger.warning(f"rclone 警告: {result.stderr}")
            return False
    except Exception as e:
        logger.error(f"バックアップ失敗: {e}")
        return False


def main():
    try:
        if len(sys.argv) < 2:
            logger.error("使用方法: python SchemToArray.py <.schemファイルのパス>")
            sys.exit(1)
        
        SCHEM_FILE = sys.argv[1]
        
        if not os.path.exists(SCHEM_FILE):
            logger.error(f"ファイルが見つかりません: {SCHEM_FILE}")
            sys.exit(1)

        schem_basename = os.path.splitext(os.path.basename(SCHEM_FILE))[0]
        output_file = os.path.join(os.path.dirname(SCHEM_FILE), f"{schem_basename}.h")

        block_definitions = load_block_definitions()
        block_name_ja = build_block_name_mapping(block_definitions)

        logger.info(f"読み込み: {SCHEM_FILE}")
        nbt_file = nbtlib.load(SCHEM_FILE)
        schem = nbt_file.root if hasattr(nbt_file, 'root') else nbt_file

        if 'Schematic' in schem:
            schem = schem['Schematic']

        try:
            width = int(schem['Width'])
            height = int(schem['Height'])
            length = int(schem['Length'])
            logger.info(f"サイズ: {width}x{height}x{length}")
        except KeyError:
            logger.error("必須タグ(Width/Height/Length)が見つかりません")
            sys.exit(1)

        palette = {}
        if 'Palette' in schem:
            palette = schem['Palette']
        elif 'Blocks' in schem and 'Palette' in schem['Blocks']:
            palette = schem['Blocks']['Palette']
        
        if not palette:
            logger.error("Palette が見つかりません")
            sys.exit(1)

        block_id_map = {}
        used_ids_in_file = set()
        
        for name, idx in palette.items():
            if '[' in name:
                base_name = name.split('[')[0]
                props_str = name.split('[')[1].rstrip(']')
                props = dict(p.split('=') for p in props_str.split(',') if '=' in p)
            else:
                base_name = name
                props = {}
            
            final_id = block_to_id(base_name, props, block_definitions)
            block_id_map[int(idx)] = final_id
            used_ids_in_file.add(final_id)

        raw_data = None
        if 'BlockData' in schem:
            raw_data = schem['BlockData']
        elif 'Blocks' in schem and 'Data' in schem['Blocks']:
            raw_data = schem['Blocks']['Data']
        
        if raw_data is None:
            logger.error("BlockData が見つかりません")
            sys.exit(1)

        decoded_blocks = decode_varint_array(raw_data)
        blocks = np.array(decoded_blocks, dtype=np.int32)
        
        level_map = np.zeros((height, length, width), dtype=int)
        for y in range(height):
            for z in range(length):
                for x in range(width):
                    idx = x + (z * width) + (y * width * length)
                    if idx < len(blocks):
                        pal_idx = blocks[idx]
                        level_map[y][z][x] = block_id_map.get(pal_idx, 99)

        # --- 日本時間(JST)でタイムスタンプを生成 ---
        jst = timezone(timedelta(hours=9))
        timestamp = datetime.now(jst).strftime("%Y-%m-%d %H:%M:%S")

        with open(output_file, "w", encoding="utf-8") as f:
            f.write("#pragma once\n\n")
            f.write(f"// Generated: {timestamp} (JST)\n")
            f.write(f"// Source: {os.path.abspath(SCHEM_FILE)}\n")
            f.write("// Block IDs:\n")
            
            for b_id in sorted(used_ids_in_file):
                name_ja = block_name_ja.get(b_id, "不明なブロック")
                f.write(f"//   {b_id}: {name_ja}\n")
            
            f.write(f"\n#define MAP_LENGTH {length}\n")
            f.write(f"#define MAP_WIDTH {width}\n")
            f.write(f"#define MAP_HEIGHT {height}\n\n")
            
            var_name = schem_basename.replace(".", "_").replace("-", "_")
            f.write(f"static int {var_name}[MAP_HEIGHT][MAP_LENGTH][MAP_WIDTH] = {{\n")
            
            for y in range(height):
                f.write(f"    {{ // Y={y}\n")
                for z in range(length - 1, -1, -1):
                    f.write("        {")
                    row = [f"{int(level_map[y][z][x]):2d}" for x in range(width - 1, -1, -1)]
                    f.write(",".join(row))
                    f.write(f"}}, // Z={z}\n")
                f.write("    },\n")
            f.write("};\n")

        logger.info(f"生成完了: {output_file}")
        backup_to_google_drive(output_file)

    except Exception as e:
        logger.error(f"予期しないエラー: {e}", exc_info=True)
        sys.exit(1)


if __name__ == "__main__":
    main()
