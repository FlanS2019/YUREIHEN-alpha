#include "camera.h"
#include "d3d11.h"
#include "DirectXMath.h"
using namespace DirectX;
#include "direct3d.h"
#include "define.h"
#include "shader.h"
#include "keyboard.h"
#include "mouse.h"
#include "texture.h"
#include "debug_ostream.h"


static Camera* CameraObject;
static float g_pitch = 0.0f;  // 上下視点角度
static float g_yaw = 0.0f;    // 左右視点角度
static float g_lastPitch = 0.0f;  // 前フレームのピッチ
static float g_lastYaw = 0.0f;    // 前フレームのヨー
static XMFLOAT3 g_targetPos = { 0.0f, 0.0f, 0.0f };  // 注視対象位置

static float g_CameraSensitivity = 0.1f;	// 感度調整用変数

void Camera_Initialize(void)
{

	CameraObject = new Camera(
		XMFLOAT3(0.0f, 0.0f, -5.0f),            // pos (初期位置)
		XMFLOAT3(0.0f, 0.0f, 0.0f),             // at (注視点)
		XMFLOAT3(0.0f, 1.0f, 0.0f),             // up (上方向)
		45.0f,                                  // fov : 視野角(標準は45度)
		(float)SCREEN_WIDTH / SCREEN_HEIGHT,    // aspect : アスペクト比
		1.0f,                                   // near : 手前の限界
		50.0f                                   // far : 奥の限界
	);

	// マウスを相対モードに設定
	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);

	// カーソル表示状態を設定（ShowCursor()のカウント問題を避けるため、Mouse_SetVisible()を使う）
	// Mouse_SetVisible() は相対モード中は何もしないため、Mouse_SetMode()の後に呼ぶ
	Mouse_SetVisible(false);

	g_pitch = 0.0f;
	g_yaw = 0.0f;
	g_targetPos = { 0.0f, 0.0f, 0.0f };
}

void Camera_Finalize(void)
{
    // マウスを絶対モードに戻す
    Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
    
    // カーソルを表示（Mouse_SetVisible()を使う）
    Mouse_SetVisible(true);
    
    delete CameraObject;
}

void Camera_Update(void)
{
    // マウス入力
    Mouse_State mouseState;
    Mouse_GetState(&mouseState);
    
    // マウスロック状態でない場合は視点操作をスキップ
    if (mouseState.positionMode == MOUSE_POSITION_MODE_ABSOLUTE)
    {
        // ウィンドウがアクティブになったらマウスロックを再度有効にする
        if (mouseState.leftButton)
        {
            Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
            Mouse_SetVisible(false);
        }
        return;  // マウスロック解除中は視点操作をしない
    }
    
    // マウス移動量を回転角度に反映
    g_yaw += static_cast<float>(mouseState.x) * MOUSE_SENSITIVITY;
    g_pitch -= static_cast<float>(mouseState.y) * MOUSE_SENSITIVITY;  // 縦方向を反転
    
	if (g_pitch > PITCH_LIMIT_LOOK_UP)
	{
		g_pitch = PITCH_LIMIT_LOOK_UP;
	}
	else if (g_pitch < PITCH_LIMIT_LOOK_DOWN)
	{
		g_pitch = PITCH_LIMIT_LOOK_DOWN;
	}

	// ピッチとヨーの変更を検出して出力
	if (g_pitch != g_lastPitch || g_yaw != g_lastYaw)
	{
		g_lastPitch = g_pitch;
		g_lastYaw = g_yaw;
	}

	// 注視対象位置を中心に計算
	XMVECTOR targetVec = XMLoadFloat3(&g_targetPos);

	// カメラ位置を計算
	float pitchRad = XMConvertToRadians(g_pitch);
	float yawRad = XMConvertToRadians(g_yaw);

	// 注視対象 から相対的なカメラ位置
	float camX = -sinf(yawRad) * cosf(pitchRad) * CAMERA_DISTANCE;

	float camY = -sinf(pitchRad) * CAMERA_DISTANCE + 0.1f;
	float camZ = -cosf(yawRad) * cosf(pitchRad) * CAMERA_DISTANCE;

	XMVECTOR cameraPos = XMVectorAdd(targetVec, XMVectorSet(camX, camY, camZ, 0.0f));

	// カメラ位置をセット
	XMFLOAT3 newCameraPos;
	XMStoreFloat3(&newCameraPos, cameraPos);

	CameraObject->UpdateView(newCameraPos, g_targetPos);
}

void Camera_Draw(void)
{
}

void Camera_SetTargetPos(XMFLOAT3 targetPos)
{
	g_targetPos.x = targetPos.x;
	g_targetPos.y = targetPos.y + CAMERA_OFFSET_Y;
	g_targetPos.z = targetPos.z;
}

float Camera_GetYaw(void)
{
	return g_yaw;
}

void Camera_SetSensitivity(float sensitivity)
{
	g_CameraSensitivity = sensitivity;
}

float Camera_GetSensitivity()
{
	return g_CameraSensitivity;
}

Camera* GetCamera(void)
{
	return CameraObject;
}