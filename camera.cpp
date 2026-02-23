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

void Camera_Initialize(void)
{
	CameraObject = new Camera(
		XMFLOAT3(0.0f, 0.0f, -5.0f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		45.0f,
		(float)DRAW_SCREEN_WIDTH / DRAW_SCREEN_HEIGHT,
		1.0f,
		50.0f
	);
}

void Camera_Finalize(void)
{
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);
	delete CameraObject;
}

void Camera_Update(void)
{
	CameraObject->Update();
}

void Camera_Draw(void)
{
}

void Camera_SetTargetPos(XMFLOAT3 targetPos)
{
	CameraObject->SetTargetPos(targetPos);
	CameraObject->Update();
}

float Camera_GetYaw(void)
{
	return CameraObject->GetYaw();
}

void Camera_SetSensitivity(float sensitivity)
{
	CameraObject->SetSensitivity(sensitivity);
}

float Camera_GetSensitivity()
{
	return CameraObject->GetSensitivity();
}

Camera* GetCamera(void)
{
	return CameraObject;
}

void Camera::Update()
{
	Mouse_State mouseState;
	Mouse_GetState(&mouseState);

	//なにこれ
	//if (mouseState.positionMode == MOUSE_POSITION_MODE_ABSOLUTE)
	//{
	//	if (mouseState.leftButton)
	//	{
	//		Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
	//		Mouse_SetVisible(false);
	//	}
	//	return;
	//}

	if (mouseState.positionMode == MOUSE_POSITION_MODE_RELATIVE)
	{
		m_yaw += static_cast<float>(mouseState.x) * MOUSE_SENSITIVITY * m_sensitivity * 2;
		m_pitch -= static_cast<float>(mouseState.y) * MOUSE_SENSITIVITY * m_sensitivity * 2;

		if (m_pitch > PITCH_LIMIT_LOOK_UP)
		{
			m_pitch = PITCH_LIMIT_LOOK_UP;
		}
		else if (m_pitch < PITCH_LIMIT_LOOK_DOWN)
		{
			m_pitch = PITCH_LIMIT_LOOK_DOWN;
		}

		if (m_pitch != m_lastPitch || m_yaw != m_lastYaw)
		{
			m_lastPitch = m_pitch;
			m_lastYaw = m_yaw;
		}
	}

	XMVECTOR targetVec = XMLoadFloat3(&m_targetPos);

	float pitchRad = XMConvertToRadians(m_pitch);
	float yawRad = XMConvertToRadians(m_yaw);

	float camX = -sinf(yawRad) * cosf(pitchRad) * CAMERA_DISTANCE;
	float camY = -sinf(pitchRad) * CAMERA_DISTANCE + 0.1f;
	float camZ = -cosf(yawRad) * cosf(pitchRad) * CAMERA_DISTANCE;

	XMVECTOR cameraPos = XMVectorAdd(targetVec, XMVectorSet(camX, camY, camZ, 0.0f));

	XMFLOAT3 newCameraPos;
	XMStoreFloat3(&newCameraPos, cameraPos);

	UpdateView(newCameraPos, m_targetPos);
}

void Camera::SetTargetPos(XMFLOAT3 targetPos)
{
	m_targetPos.x = targetPos.x;
	m_targetPos.y = targetPos.y + CAMERA_OFFSET_Y;
	m_targetPos.z = targetPos.z;
}