#pragma once

//==============================================================================
// プロジェクト全体の定数定義ファイル
//==============================================================================

#define MAP_FLOORS (3)

//=== Camera 関連定数 ===
#define PITCH_LIMIT_LOOK_UP    (20.0f)   // 上を見る限界（カメラが下がる限界）: 床埋まり防止
#define PITCH_LIMIT_LOOK_DOWN  (-5.0f)  // 下を見る限界（カメラが上がる限界）: 天井埋まり防止

#define MOUSE_SENSITIVITY (0.15f)
#define CAMERA_DISTANCE (6.0f)  // カメラ距離
#define CAMERA_OFFSET_Y (1.5f)  // 注視点のオフセット

//=== Furniture 関連定数 ===
#define FURNITURE_NUM (100)
#define FURNITURE_DETECTION_RANGE (5.0f) // Ghost検出範囲

//=== Busters 関連定数 ===
#define BUSTERS_PATROL_SPEED (0.001f)    // パトロール速度

#define MAP_MIN_X (-20.0f)
#define MAP_MAX_X (20.0f)
#define MAP_MIN_Z (-20.0f)
#define MAP_MAX_Z (20.0f)

#define BUSTERS_PATROL_RANGH (5.0f)      // 恐怖感知範囲
#define BUSTERS_SUSPICION_RANGE (10.0f)  // 怪しんで近づいてくる範囲
#define PATROL_HEIGHT (0.0f)

//=== Ghost 関連定数 ===
#define SCARE_RANGE (7.5f)		// 恐怖範囲
#define GHOST_MOVEMENT_SPEED (0.1f)
#define GHOST_ACCELERATION (0.010f)
#define GHOST_DECELERATION (0.98f)
#define GHOST_MAX_SPEED (0.10f)
#define FLOOR_COOLDOWN_TIME (2.0f) // 階層移動のクールダウン時間（秒）

#define GHOST_POS_Y (0.5f)

//=== Minimap 関連定数 ===

#define MINIMAP_POS_X   (100.0f) 
#define MINIMAP_POS_Y   (500.0f) 
#define BLOCK_SIZE      (10.0f)  
#define VIEW_RANGE      (8) 
