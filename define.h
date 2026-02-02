#pragma once

//==============================================================================
// プロジェクト全体の定数定義ファイル
//==============================================================================

#define CLASS_NAME L"幽霊変 - Violisun"
#define WINDOW_CAPTION L"幽霊変 - Violisun"
#define SCREEN_WIDTH (1280.0f)
#define SCREEN_HEIGHT (720.0f)
#define WIN32_LEAN_AND_MEAN	//32bitアプリには不要な情報を無視
#define FPS (60)

#if defined(_DEBUG)
//=== デバッグ関連定数 ===
#define STOP_TIMER_BUSTER (true) //trueならタイマーとバスターズのupdateを停止させる
#define DIRECT_START (true) //trueならgameシーンから直接開始する
#define DEBUG_DRAW (false) //trueならdebugdraw機能を有効にする
#else
//==================================
//          ここは触らない
//==================================
#define STOP_TIMER_BUSTER (false) 
#define DIRECT_START (false) 
#define DEBUG_DRAW (false)
//==================================
//          ここは触らない
//==================================
#endif

//=== フィールド関連定数 ===
#define MAP_FLOORS (3)
#define START_FLOOR (3)// 階の数字をそのまま入れる（使用時に-1）

//=== Camera 関連定数 ===
#define PITCH_LIMIT_LOOK_UP    (25.0f)   // 上を見る限界（カメラが下がる限界）: 床埋まり防止
#define PITCH_LIMIT_LOOK_DOWN  (-60.0f)  // 下を見る限界（カメラが上がる限界）: 天井埋まり防止
#define MOUSE_SENSITIVITY (0.15f)
#define CAMERA_DISTANCE (6.0f)  // カメラ距離
#define CAMERA_OFFSET_Y (1.5f)  // 注視点のオフセット

//=== Furniture 関連定数 ===
#define FURNITURE_NUM (500)
#define FURNITURE_DETECTION_RANGE (5.0f) // Ghost検出範囲

//=== Busters 関連定数 ===
#define BUSTERS_PATROL_SPEED (0.001f)    // パトロール速度
#define BUSTERS_MOVE_SPEED_SEARCH    (0.04f) // 探索速度
#define BUSTERS_MOVE_SPEED_SUSPICION (0.06f) // 警戒速度
#define BUSTERS_MOVE_SPEED_CHASE     (0.08f) // 追跡速度

#define MAP_MIN_X (-20.0f)
#define MAP_MAX_X (20.0f)
#define MAP_MIN_Z (-20.0f)
#define MAP_MAX_Z (20.0f)

#define BUSTERS_PATROL_RANGH (5.0f)      // 恐怖感知範囲
#define BUSTERS_SUSPICION_RANGE (10.0f)  // 怪しんで近づいてくる範囲
#define PATROL_HEIGHT (0.0f)
#define BUSTERS_DEFOURT_GAUGE (50.0f)
#define WAIT_TIMER_DEFAULT (60)		     // 待機時間のデフォルト
#define WAIT_TIMER_COOLDOWN (1800)		 // 待機のクールタイム
#define KEEP_STATE_TIME (120)			 // 発見状態を維持する時間

//=== Ghost 関連定数 ===
#define SCARE_RANGE (7.5f)		// 恐怖範囲
#define GHOST_MOVEMENT_SPEED (0.01f)
#define GHOST_ACCELERATION (0.010f)
#define GHOST_DECELERATION (0.98f)
#define GHOST_MAX_SPEED (0.10f)
#define FLOOR_COOLDOWN_TIME (2.0f) // 階層移動のクールダウン時間（秒）
#define GHOST_POS_Y (0.5f)

//=== Minimap 関連定数 ===
#define MINIMAP_POS_OFFSET   (150.0f) 
#define BLOCK_SIZE      (10.0f)  
#define VIEW_RANGE      (13) 

//=== スコア関連定数 ===
#define SCARE_GAUGE_MAX (100.0f)
#define SCORE_LURE (3) // 引き寄せスコア
#define SCORE_STOP (3) // 停止スコア
#define SCORE_SCARE (5) // 驚かせスコア

//=== UI_ScareCombo 関連定数 ===
#define SCARECOMBO_POS_X (SCREEN_WIDTH - 110.0f)
#define SCARECOMBO_POS_Y (170.0f)
#define SCARECOMBO_MAX (5)
#define SCARECOMBO_OVER_TIME (10000)// ミリ秒
#define SCARECOMBO_BAR_SIZE_X (140.0f)
#define SCARECOMBO_BAR_POS_X (SCARECOMBO_POS_X - 70.0f)
