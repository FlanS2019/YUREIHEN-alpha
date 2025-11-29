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
#define MOVEMENT_SPEED (0.1f)  // カメラ移動速度

static Camera* CameraObject;
static float g_pitch = 0.0f;  // 上下視点角度
static float g_yaw = 0.0f;    // 左右視点角度
static float g_lastPitch = 0.0f;  // 前フレームのピッチ
static float g_lastYaw = 0.0f;    // 前フレームのヨー

void Camera_Initialize(void)
{
    CameraObject = new Camera();
    
    // マウスを相対モードに設定
    Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
    
    // カーソルを非表示
    ShowCursor(FALSE);
    
    g_pitch = 0.0f;
    g_yaw = 0.0f;
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
    
    // Rキーでカメラ回転をリセット
    //if (Keyboard_IsKeyDownTrigger(KK_R))
    //{
    //    g_pitch = 0.0f;
    //    g_yaw = 0.0f;
    //}
    
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
    
    // 現在のカメラ位置と注視点を取得
    XMFLOAT3 pos = CameraObject->GetPos();
    XMFLOAT3 atPos = CameraObject->GetAtPos();
    
    XMVECTOR posVec = XMLoadFloat3(&pos);
    XMVECTOR atVec = XMLoadFloat3(&atPos);
    
    // カメラから注視点への方向
    XMVECTOR toTarget = XMVectorSubtract(atVec, posVec);
    
    // 距離を保存
    float distance = XMVectorGetX(XMVector3Length(toTarget));
    
    // 新しい方向を計算（ピッチとヨーを適用）
    float pitchRad = XMConvertToRadians(g_pitch);
    float yawRad = XMConvertToRadians(g_yaw);
    
    XMVECTOR forward;
    forward = XMVectorSet(
        sinf(yawRad) * cosf(pitchRad),
        sinf(pitchRad),
        cosf(yawRad) * cosf(pitchRad),
        0.0f
    );
    forward = XMVector3Normalize(forward);
    
    // ====================================
    // マインクラフト風移動処理
    // ====================================
    
    // 水平方向の前方向を計算（Y軸成分を0にして水平方向のみ）
    XMVECTOR horizontalForward = XMVectorSet(
        sinf(yawRad),
        0.0f,
        cosf(yawRad),
        0.0f
    );
    horizontalForward = XMVector3Normalize(horizontalForward);
    
    // 右方向を計算（Y軸で回転した前方向に垂直）
    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, horizontalForward));
    
    // 移動ベクトルを初期化
    XMVECTOR moveVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    
    // WASD キーでの移動入力
    // W: 前方移動
    if (Keyboard_IsKeyDown(KK_W))
    {
        moveVec = XMVectorAdd(moveVec, XMVectorScale(horizontalForward, MOVEMENT_SPEED));
    }
    // S: 後方移動
    if (Keyboard_IsKeyDown(KK_S))
    {
        moveVec = XMVectorAdd(moveVec, XMVectorScale(horizontalForward, -MOVEMENT_SPEED));
    }
    // D: 右移動
    if (Keyboard_IsKeyDown(KK_D))
    {
        moveVec = XMVectorAdd(moveVec, XMVectorScale(right, MOVEMENT_SPEED));
    }
    // A: 左移動
    if (Keyboard_IsKeyDown(KK_A))
    {
        moveVec = XMVectorAdd(moveVec, XMVectorScale(right, -MOVEMENT_SPEED));
    }
    // Space: 上昇
    if (Keyboard_IsKeyDown(KK_SPACE))
    {
        moveVec = XMVectorAdd(moveVec, XMVectorScale(worldUp, MOVEMENT_SPEED));
    }
    // Shift: 下降
    if (Keyboard_IsKeyDown(KK_LEFTSHIFT))
    {
        moveVec = XMVectorAdd(moveVec, XMVectorScale(worldUp, -MOVEMENT_SPEED));
    }
    
    // カメラ位置と注視点を移動ベクトルで更新
    posVec = XMVectorAdd(posVec, moveVec);
    atVec = XMVectorAdd(atVec, moveVec);
    
    // 新しい注視点を計算
    XMVECTOR newAtPos = XMVectorAdd(posVec, XMVectorScale(forward, distance));
    
    XMFLOAT3 newPos;
    XMFLOAT3 newAtPosFloat;
    XMStoreFloat3(&newPos, posVec);
    XMStoreFloat3(&newAtPosFloat, newAtPos);
    
    CameraObject->UpdateView(newPos, newAtPosFloat);
}

void Camera_Draw(void)
{
}

Camera* GetCamera(void)
{
    return CameraObject;
}
