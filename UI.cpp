#include "camera.h"
#include "UI.h"
#include "sprite.h"
#include "debug_ostream.h"
#include "keyboard.h"
#include "fade.h"
#include "UI_scarecombo.h"
#include "field.h"
#include "define.h"
#include "ghost.h"

// �O���[�o���ϐ�
static Timer* g_Clock = nullptr;
static Gauge* g_ScareGauge = nullptr;
Sprite* g_Reticle = nullptr;
static DWORD g_LastScoreUpdateTime = 0;

static Number* g_FloorNumber = nullptr;
static Sprite* g_FloorTextF = nullptr;

// �N���b�N�K�C�h�p
static Sprite* g_GuideClick = nullptr;

// �K�w�ړ��K�C�h�p
static Number* g_GuideFloorNum = nullptr;
static Sprite* g_GuideFloorF = nullptr;
static bool g_ShowGuideFloor = false;

// �e�K�w�̃Q�[�W�l��ۑ�����z��
static float g_FloorGaugeValues[MAP_FLOORS];
// �O�t���[���̊K�w��L�����Ă����ϐ�
static int g_LastFrameFloor = -1;


// 3D���W -> 2D�X�N���[�����W�ϊ�
static XMFLOAT2 WorldToScreen(const XMFLOAT3& worldPos)
{
	Camera* camera = GetCamera();
	// �C��: �����I�ȃR���X�g���N�^��g�p
	if (!camera) return XMFLOAT2(-100.0f, -100.0f);

	XMMATRIX view = camera->GetView();
	XMMATRIX projection = camera->GetProjection();
	XMMATRIX viewProj = view * projection;

	XMVECTOR posVec = XMLoadFloat3(&worldPos);
	posVec = XMVectorSetW(posVec, 1.0f);

	XMVECTOR clipPos = XMVector3TransformCoord(posVec, viewProj);
	XMFLOAT3 ndc;
	XMStoreFloat3(&ndc, clipPos);

	// ��ʊO����
	if (ndc.z < 0.0f || ndc.z > 1.0f)
	{
		return XMFLOAT2(-1000.0f, -1000.0f);
	}

	float screenX = (ndc.x + 1.0f) * 0.5f * SCREEN_WIDTH;
	float screenY = (1.0f - ndc.y) * 0.5f * SCREEN_HEIGHT;

	return XMFLOAT2(screenX, screenY);
}

//----------------------------
// UI������
//----------------------------
void UI_Initialize(void)
{
	g_Clock = new Timer(
		{ CLOCK_POS_X, CLOCK_POS_Y },
		{ CLOCK_SIZE, CLOCK_SIZE },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\clock.png",
		2, 1,
		CLOCK_MIN, CLOCK_MAX
	);

	// �N���b�N�K�C�h
	g_GuideClick = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 100.0f },
		{ 100.0f, 100.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\click_guide.png"
	);

	// �K�w�ړ��K�C�h(����)
	g_GuideFloorNum = new Number(
		{ 0.0f, 0.0f },
		{ 40.0f, 40.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		25.0f
	);

	// �K�w�ړ��K�C�h(F)
	g_GuideFloorF = new Sprite(
		{ 0.0f, 0.0f },
		{ 40.0f, 40.0f },
		0.0f,
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\floor_f.png"
	);

	g_ScareGauge = new Gauge(
		{ SCREEN_WIDTH - 270.0f, 70.0f },
		{ GAUGE_SIZE, GAUGE_SIZE },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\gauge.png",
		3, 1,
		0.0f, 100.0f,
		2, 0
	);

	g_Reticle = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		{ 30.0f, 30.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.5f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\grass.png"
	);

	UI_ScareCombo_Initialize();

	// ���݂̊K�w�\��
	float floorPosX = CLOCK_POS_X;
	float floorPosY = CLOCK_POS_Y + 200.0f;

	g_FloorNumber = new Number(
		{ floorPosX - 20.0f, floorPosY },
		{ 60.0f, 60.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		40.0f
	);
	g_FloorNumber->SetNumber(1);

	g_FloorTextF = new Sprite(
		{ floorPosX + 30.0f, floorPosY },
		{ 60.0f, 60.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\floor_f.png"
	);

	// �Q�[�W�Ǘ�������
	for (int i = 0; i < MAP_FLOORS; i++)
	{
		g_FloorGaugeValues[i] = 50.0f;
	}

	g_LastFrameFloor = Field_GetCurrentFloor();
	g_ScareGauge->SetValue(g_FloorGaugeValues[g_LastFrameFloor]);
}

//----------------------------
// UI�X�V
//----------------------------
void UI_Update(void)
{
	if (Keyboard_IsKeyDown(KK_L))
	{
		SetScene(SCENE_ANM_LOSE);// Debug�p�ɔs�k�A�j���[�V�����֒��ڔ��
		return;

	}
	
	// ���|�Q�[�W���ő�Ȃ珟���V�[���ֈڍs�i�f�o�b�O�p�j
	if (g_ScareGauge->GetValue() >= g_ScareGauge->GetMaxValue())
	{
		StartFade(SCENE_ANM_WIN);
	}

	// --- �s�k��� ---
	if (g_Clock->Update() || g_ScareGauge->GetValue() <= 0.0f)
	{
		hal::dout << "�s�k����𖞂����܂���" << std::endl;
		StartFade(SCENE_ANM_LOSE);
	}

	UI_ScareCombo_Update();
	g_FloorNumber->SetNumber(Field_GetCurrentFloor() + 1);

	// --- �K�i�K�C�h�̐��� ---
	bool onStairs = false;
	int targetFloor = 0;

	Ghost* ghost = GetGhost();

	if (ghost && !ghost->GetIsTransformed())
	{
		XMFLOAT3 pos = ghost->GetPos();
		FIELD_TYPE blockType = Field_GetBlockType(pos.x, pos.z);
		int floorIndex = Field_GetCurrentFloor();

		if (blockType == FIELD_STAIRS_UP)
		{
			if (floorIndex < MAP_FLOORS - 1)
			{
				onStairs = true;
				targetFloor = floorIndex + 2;
			}
		}
		else if (blockType == FIELD_STAIRS_DOWN)
		{
			if (floorIndex > 0)
			{
				onStairs = true;
				targetFloor = floorIndex;
			}
		}

		// �ړ��K�C�h�̍��W�v�Z
		if (onStairs)
		{
			g_ShowGuideFloor = true;

			XMFLOAT3 headPos = pos;
			headPos.y += 2.0f;

			XMFLOAT2 screenPos = WorldToScreen(headPos);

			// �C��: SetPos �ɂ� XMFLOAT2 ��n��
			if (g_GuideFloorNum)
			{
				g_GuideFloorNum->SetPos(XMFLOAT2(screenPos.x - 25.0f, screenPos.y));
				g_GuideFloorNum->SetNumber(targetFloor);
			}

			if (g_GuideFloorF)
			{
				g_GuideFloorF->SetPos(XMFLOAT2(screenPos.x + 25.0f, screenPos.y));
			}
		}
		else
		{
			g_ShowGuideFloor = false;
		}
	}

	// �N���b�N�K�C�h�̓_��
	if (onStairs)
	{
		static float flash = 0.0f;
		flash += 0.1f;
		float alpha = 0.5f + sinf(flash) * 0.5f;

		// �C��: SetColor �ɂ� XMFLOAT4 ��n��
		if (g_GuideClick)
			g_GuideClick->SetColor(XMFLOAT4(1.0f, 1.0f, 1.0f, alpha));
	}
	else
	{
		if (g_GuideClick)
			g_GuideClick->SetColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f));
	}
}

//----------------------------
// UI�`��
//----------------------------
void UI_Draw(void)
{
	g_Clock->Draw();
	g_ScareGauge->Draw();
	UI_ScareCombo_Draw();

	if (g_FloorNumber) g_FloorNumber->Draw();
	if (g_FloorTextF) g_FloorTextF->Draw();

	// �N���b�N�K�C�h
	if (g_GuideClick) g_GuideClick->Draw();

	// �K�w�ړ��K�C�h
	if (g_ShowGuideFloor)
	{
		if (g_GuideFloorNum) g_GuideFloorNum->Draw();
		if (g_GuideFloorF) g_GuideFloorF->Draw();
	}
}

//----------------------------
// UI�I��
//----------------------------
void UI_Finalize(void)
{
	delete g_Clock;
	delete g_ScareGauge;
	delete g_Reticle;
	UI_ScareCombo_Finalize();

	if (g_FloorNumber) { delete g_FloorNumber; g_FloorNumber = nullptr; }
	if (g_FloorTextF) { delete g_FloorTextF; g_FloorTextF = nullptr; }

	if (g_GuideClick) { delete g_GuideClick; g_GuideClick = nullptr; }
	if (g_GuideFloorNum) { delete g_GuideFloorNum; g_GuideFloorNum = nullptr; }
	if (g_GuideFloorF) { delete g_GuideFloorF; g_GuideFloorF = nullptr; }
}

void AddScareGauge(float value)
{
	if (g_ScareGauge)
	{
		g_ScareGauge->AddValue(value);
	}
}
