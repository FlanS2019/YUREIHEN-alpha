#include "camera.h"
#include "d3d11.h"
#include "DirectXMath.h"
using namespace DirectX;
#include "direct3d.h"
#include "shader.h"
#include "keyboard.h"
#include "mouse.h"
#include "texture.h"
#include "debug_ostream.h"

#define ROTATE_Y_MAX (80.0f)
#define MOUSE_SENSITIVITY (0.15f)
#define CAMERA_DISTANCE (6.0f)  // 注視対象からのカメラ距離を短縮

static Camera* CameraObject;
static float g_pitch = 0.0f;  // 上下視点角度
static float g_yaw = 0.0f;    // 左右視点角度
static float g_lastPitch = 0.0f;  // 前フレームのピッチ
static float g_lastYaw = 0.0f;    // 前フレームのヨー
static XMFLOAT3 g_targetPos = { 0.0f, 0.0f, 0.0f };  // 注視対象位置

void Camera_Initialize(void)
{
    CameraObject = new Camera();
    
    // マウスを相対モードに設定
    Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
    
    // カーソルを非表示
    ShowCursor(FALSE);
    
    g_pitch = 0.0f;
    g_yaw = 0.0f;
    g_targetPos = { 0.0f, 0.0f, 0.0f };
}

void Camera_Finalize(void)
{
    // マウスを絶対モードに戻す
    Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
    
    // カーソルを表示
    ShowCursor(TRUE);
    
    delete CameraObject;
}

void Camera_Update(void)
{
    // マウス入力
    Mouse_State mouseState;
    Mouse_GetState(&mouseState);
    
    // ESCキーで終了
    if (Keyboard_IsKeyDownTrigger(KK_ESCAPE))
    {
        Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
        ShowCursor(TRUE);
        return;
    }
    
    // マウスロック状態でない場合は視点操作をスキップ
    if (mouseState.positionMode == MOUSE_POSITION_MODE_ABSOLUTE)
    {
        // ウィンドウがアクティブになったらマウスロックを再度有効にする
        if (mouseState.leftButton)
        {
            Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
            ShowCursor(FALSE);
        }
        return;  // マウスロック解除中は視点操作をしない
    }
    
    // マウス移動量を回転角度に反映
    g_yaw += static_cast<float>(mouseState.x) * MOUSE_SENSITIVITY;
    g_pitch -= static_cast<float>(mouseState.y) * MOUSE_SENSITIVITY;  // 縦方向を反転
    
    // ピッチ角度を制限
    if (g_pitch > ROTATE_Y_MAX)
    {
        g_pitch = ROTATE_Y_MAX;
    }
    else if (g_pitch < -ROTATE_Y_MAX)
    {
        g_pitch = -ROTATE_Y_MAX;
    }
    
    // ピッチとヨーの変更を検出して出力
    if (g_pitch != g_lastPitch || g_yaw != g_lastYaw)
    {
        //hal::dout << "Camera - Pitch: " << g_pitch << ", Yaw: " << g_yaw << std::endl;
        g_lastPitch = g_pitch;
        g_lastYaw = g_yaw;
    }
    
    // 注視対象位置を中心に計算
    XMVECTOR targetVec = XMLoadFloat3(&g_targetPos);
    
    // カメラ位置を計算（注視対象周りに球体回転）
    float pitchRad = XMConvertToRadians(g_pitch);
    float yawRad = XMConvertToRadians(g_yaw);
    
    // 注視対象 から相対的なカメラ位置（符号を反転）
    float camX = -sinf(yawRad) * cosf(pitchRad) * CAMERA_DISTANCE;
    float camY = -sinf(pitchRad) * CAMERA_DISTANCE + 1.5f;  // 上方向にオフセット
    float camZ = -cosf(yawRad) * cosf(pitchRad) * CAMERA_DISTANCE;
    
    XMVECTOR cameraPos = XMVectorAdd(targetVec, XMVectorSet(camX, camY, camZ, 0.0f));
    
    // カメラ位置を計算
    XMFLOAT3 newCameraPos;
    XMStoreFloat3(&newCameraPos, cameraPos);
    
    CameraObject->UpdateView(newCameraPos, g_targetPos);
}

void Camera_Draw(void)
{
}

void Camera_SetTargetPos(XMFLOAT3 targetPos)
{
    g_targetPos = targetPos;
}

float Camera_GetYaw(void)
{
    return g_yaw;
}

Camera* GetCamera(void)
{
    return CameraObject;
}
