// OpAnim.cpp
#include "Fade.h"
#include "OpAnim.h"
#include "shader.h"
#include "Sprite.h"
#include "Keyboard.h"
#include "main.h"
#include "scene.h"
#include <cmath>
#include <d3d11.h>

// Sprite インスタンスを管理するポインタ
static Sprite* g_yakataSprite = nullptr;
static Sprite* g_ghostSprite = nullptr;
static Sprite* g_basutaSprite = nullptr;

static bool fadeStarted = false;

static float alpha[3] = { 1.0f, 0.0f, 0.0f };

// 幽霊アニメーション用
static XMFLOAT2 g_ghostOffset = { 0.0f, 0.0f };
static float g_ghostBobRotation = 0.0f;
static float g_ghostAngle = 0.0f;
static float g_ghostTargetAngle = 0.0f;
static bool g_ghostFacingLeft = false; // 幽霊の向き（true = 左向き）
static float g_ghostScale = 1.0f;      // 幽霊のスケール（縮小用）

// basuta アニメーション用
static XMFLOAT2 g_basutaOffset = { 0.0f, 0.0f };

// 状態管理
enum GhostState { GHOST_IDLE = 0, GHOST_ALERT, GHOST_MOVE_TO_HOUSE };
static GhostState g_ghostState = GHOST_IDLE;
static const XMFLOAT2 g_imageSize = { 500.0f, 500.0f };

// 位置管理
static XMFLOAT2 g_yakataPos = { 0.0f, 0.0f };
static XMFLOAT2 g_ghostPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaTarget = { 0.0f, 0.0f };
static bool g_positionsInitialized = false;
static bool g_basutaMoving = false;

// OpAnim_Update用の静的変数
static float g_updateTimer = 0.0f;
static bool g_waitStarted = false;
static float g_waitTimer = 0.0f;

static const float PI = 3.14159265358979323846f;

static float AngleDelta(float target, float current)
{
    float diff = target - current;
    while (diff > PI) diff -= 2.0f * PI;
    while (diff < -PI) diff += 2.0f * PI;
    return diff;
}

void OpAnim_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    SetFPS(30);

    // Sprite インスタンスの作成
    // 屋敷スプライト
    g_yakataSprite = new Sprite(
        XMFLOAT2(0.0f, 0.0f),           // 初期位置
        g_imageSize,                      // サイズ
        0.0f,                             // 回転
        XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // 色
        BLENDSTATE_ALFA,                  // ブレンドステート
        L"asset\\yureihen\\yakata_jimen1.png"  // テクスチャパス
    );

    // 幽霊スプライト
    g_ghostSprite = new Sprite(
        XMFLOAT2(0.0f, 0.0f),           // 初期位置
        g_imageSize,                      // サイズ
        0.0f,                             // 回転
        XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), // 色（初期状態では透明）
        BLENDSTATE_ALFA,                  // ブレンドステート
        L"asset\\yureihen\\yurei1.png"    // テクスチャパス
    );

    // basuta スプライト
    g_basutaSprite = new Sprite(
        XMFLOAT2(0.0f, 0.0f),           // 初期位置
        g_imageSize,                      // サイズ
        0.0f,                             // 回転
        XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), // 色（初期状態では透明）
        BLENDSTATE_ALFA,                  // ブレンドステート
        L"asset\\yureihen\\basuta1.png"   // テクスチャパス
    );

    alpha[0] = 1.0f;
    alpha[1] = 0.0f;
    alpha[2] = 0.0f;

    g_ghostAngle = 0.0f;
    g_ghostTargetAngle = 0.0f;
    g_ghostState = GHOST_IDLE;
    g_ghostFacingLeft = false;
    g_ghostScale = 1.0f;
}

void OpAnim_Finalize()
{
    SetFPS(60);

    // Sprite インスタンスの削除
    delete g_yakataSprite;
    g_yakataSprite = nullptr;
    delete g_ghostSprite;
    g_ghostSprite = nullptr;
    delete g_basutaSprite;
    g_basutaSprite = nullptr;

    // グローバル変数の初期化
    fadeStarted = false;
    
    alpha[0] = 1.0f;
    alpha[1] = 0.0f;
    alpha[2] = 0.0f;

    // 幽霊アニメーション用
    g_ghostOffset = { 0.0f, 0.0f };
    g_ghostBobRotation = 0.0f;
    g_ghostAngle = 0.0f;
    g_ghostTargetAngle = 0.0f;
    g_ghostFacingLeft = false;
    g_ghostScale = 1.0f;

    // basuta アニメーション用
    g_basutaOffset = { 0.0f, 0.0f };

    // 状態管理
    g_ghostState = GHOST_IDLE;

    // 位置管理
    g_yakataPos = { 0.0f, 0.0f };
    g_ghostPos = { 0.0f, 0.0f };
    g_basutaPos = { 0.0f, 0.0f };
    g_basutaTarget = { 0.0f, 0.0f };
    g_positionsInitialized = false;
    g_basutaMoving = false;

    // OpAnim_Update用の静的変数の初期化
    g_updateTimer = 0.0f;
    g_waitStarted = false;
    g_waitTimer = 0.0f;
}

void OpAnim_Update()
{
    const float delta = 1.0f / 60.0f;
    g_updateTimer += delta;

    if (!g_positionsInitialized)
    {
        XMFLOAT2 center = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

        const float leftMargin = 20.0f;
        g_yakataPos.x = (g_imageSize.x / 2.0f) + leftMargin;
        g_yakataPos.y = center.y;

        g_ghostPos.x = g_yakataPos.x + (g_imageSize.x * 0.35f);
        g_ghostPos.y = g_yakataPos.y - 40.0f;

        g_basutaPos.x = SCREEN_WIDTH + (g_imageSize.x / 2.0f) + 100.0f;
        g_basutaPos.y = center.y + 30.0f;

        g_basutaTarget.x = g_yakataPos.x + (g_imageSize.x * 0.25f);
        g_basutaTarget.y = g_yakataPos.y + 20.0f;

        g_positionsInitialized = true;
        g_basutaMoving = false;

        // Sprite の初期位置を設定
        if (g_yakataSprite) g_yakataSprite->SetPos(g_yakataPos);
        if (g_ghostSprite) g_ghostSprite->SetPos(g_ghostPos);
        if (g_basutaSprite) g_basutaSprite->SetPos(g_basutaPos);
    }

    const float ghostStart = 0.5f;
    const float basutaStart = 2.0f;
    const float fadeDuration = 0.8f;

    if (g_updateTimer > ghostStart)
    {
        alpha[1] += delta / fadeDuration;
        if (alpha[1] > 1.0f) alpha[1] = 1.0f;
    }

    if (g_updateTimer > basutaStart)
    {
        alpha[2] += delta / fadeDuration;
        if (alpha[2] > 1.0f) alpha[2] = 1.0f;
        g_basutaMoving = true;
    }

    // basuta の移動処理
    if (g_basutaMoving)
    {
        const float moveSpeed = 220.0f;
        float dx = g_basutaTarget.x - g_basutaPos.x;
        float dy = g_basutaTarget.y - g_basutaPos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 1.0f)
        {
            float nx = dx / dist;
            float ny = dy / dist;
            float step = moveSpeed * delta;
            if (step >= dist)
            {
                g_basutaPos.x = g_basutaTarget.x;
                g_basutaPos.y = g_basutaTarget.y;
                g_basutaMoving = false;
            }
            else
            {
                g_basutaPos.x += nx * step;
                g_basutaPos.y += ny * step;
            }
        }
        else
        {
            g_basutaPos.x = g_basutaTarget.x;
            g_basutaPos.y = g_basutaTarget.y;
            g_basutaMoving = false;
        }
    }

    // 幽霊の状態管理
    {
        const float triggerDist = 320.0f;  // 検知距離（より遠くから反応）
        const float fleeDist = 200.0f;     // 逃げ始める距離

        float dx = g_basutaPos.x - g_ghostPos.x;
        float dy = g_basutaPos.y - g_ghostPos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (g_ghostState == GHOST_IDLE)
        {
            // basutaが近づいてきたら警戒開始
            if (dist <= triggerDist && alpha[2] > 0.3f)
            {
                g_ghostState = GHOST_ALERT;
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
                // basutaの方を向く（手前で反転）
                g_ghostFacingLeft = (g_basutaPos.x < g_ghostPos.x);
            }
        }
        else if (g_ghostState == GHOST_ALERT)
        {
            // さらに近づいてきたら館に逃げる
            if (dist <= fleeDist)
            {
                g_ghostState = GHOST_MOVE_TO_HOUSE;
                // 館の方向を向くように反転を更新
                float tx = g_yakataPos.x - (g_imageSize.x * 0.15f);
                g_ghostFacingLeft = (tx < g_ghostPos.x);
            }
            else
            {
                // まだ距離があるので警戒しながらbasutaを見続ける
                const float rotSpeed = 3.5f;
                float deltaAngle = AngleDelta(g_ghostTargetAngle, g_ghostAngle);
                float maxStep = rotSpeed * delta;
                if (fabsf(deltaAngle) <= maxStep)
                {
                    g_ghostAngle = g_ghostTargetAngle;
                }
                else
                {
                    g_ghostAngle += (deltaAngle > 0.0f ? 1.0f : -1.0f) * maxStep;
                }
                // basutaの位置を追跡
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
            }
        }
        else if (g_ghostState == GHOST_MOVE_TO_HOUSE)
        {
            const float ghostMoveSpeed = 180.0f; // 逃げる時は速めに
            float tx = g_yakataPos.x - (g_imageSize.x * 0.15f); // 館の奥に入る
            float ty = g_yakataPos.y;
            float dxh = tx - g_ghostPos.x;
            float dyh = ty - g_ghostPos.y;
            float distH = sqrtf(dxh * dxh + dyh * dyh);

            // 館に近づくにつれて縮小（遠近感）
            const float startDist = 200.0f; // 縮小開始距離
            if (distH < startDist)
            {
                // 距離に応じて 1.0 → 0.3 まで縮小
                g_ghostScale = 0.3f + (distH / startDist) * 0.7f;
            }
            else
            {
                g_ghostScale = 1.0f;
            }

            if (distH > 2.0f)
            {
                float nx = dxh / distH;
                float ny = dyh / distH;
                float step = ghostMoveSpeed * delta;
                if (step >= distH)
                {
                    g_ghostPos.x = tx;
                    g_ghostPos.y = ty;
                    g_ghostState = GHOST_IDLE;
                    // 館の中に入ったので完全に消す
                    alpha[1] = 0.0f;
                }
                else
                {
                    g_ghostPos.x += nx * step;
                    g_ghostPos.y += ny * step;
                    float targetMoveAngle = atan2f(ny, nx);
                    float moveDeltaAngle = AngleDelta(targetMoveAngle, g_ghostAngle);
                    float moveRotSpeed = 2.5f;
                    float moveMaxStep = moveRotSpeed * delta;
                    if (fabsf(moveDeltaAngle) <= moveMaxStep)
                    {
                        g_ghostAngle = targetMoveAngle;
                    }
                    else
                    {
                        g_ghostAngle += (moveDeltaAngle > 0.0f ? 1.0f : -1.0f) * moveMaxStep;
                    }
                }
            }
            else
            {
                g_ghostState = GHOST_IDLE;
                // 館の中に入ったので完全に消す
                alpha[1] = 0.0f;
            }
        }
    }

    if (!fadeStarted && g_updateTimer > 7.0f && GetFadeState() == FADE_NONE)
    {
        fadeStarted = true;
        XMFLOAT4 color = { 0, 0, 0, 1 };
        StartFade(SCENE_GAME);
    }

    if (fadeStarted && !g_waitStarted && GetFadeState() == FADE_NONE)
    {
        g_waitStarted = true;
        g_waitTimer = 0.0f;
    }

    if (g_waitStarted)
    {
        g_waitTimer += delta;
        if (g_waitTimer >= 1.5f)
        {
        }
    }

    // 幽霊のふわふわ
    if (g_ghostState == GHOST_MOVE_TO_HOUSE)
    {
        g_ghostOffset.y = sinf(g_updateTimer * 2.0f) * 6.0f;
        g_ghostOffset.x = sinf(g_updateTimer * 0.7f) * 3.0f;
        g_ghostBobRotation = sinf(g_updateTimer * 1.2f) * 0.03f;
    }
    else
    {
        g_ghostOffset.y = sinf(g_updateTimer * 2.5f) * 12.0f;
        g_ghostOffset.x = sinf(g_updateTimer * 0.9f) * 6.0f;
        g_ghostBobRotation = sinf(g_updateTimer * 1.2f) * 0.06f;
    }

    // basuta のふわふわ（歩いているような揺れ）
    if (g_basutaMoving)
    {
        // 歩行中は上下に揺らす（より速いリズム）
        g_basutaOffset.y = sinf(g_updateTimer * 5.0f) * 8.0f;
        g_basutaOffset.x = sinf(g_updateTimer * 10.0f) * 3.0f; // 左右の微妙な揺れ
    }
    else
    {
        // 停止中は控えめな揺れ
        g_basutaOffset.y = sinf(g_updateTimer * 2.0f) * 4.0f;
        g_basutaOffset.x = sinf(g_updateTimer * 1.5f) * 2.0f;
    }

    // Sprite の位置と色を更新
    if (g_yakataSprite)
    {
        g_yakataSprite->SetPos(g_yakataPos);
    }

    if (g_ghostSprite)
    {
        XMFLOAT2 ghostDrawPos = { g_ghostPos.x + g_ghostOffset.x, g_ghostPos.y + g_ghostOffset.y };
        g_ghostSprite->SetPos(ghostDrawPos);
        // スケール更新
        XMFLOAT2 scaledSize = { g_imageSize.x * g_ghostScale, g_imageSize.y * g_ghostScale };
        g_ghostSprite->SetSize(scaledSize);
        // 色のアルファを更新
        XMFLOAT4 ghostCol = { 1.0f, 1.0f, 1.0f, alpha[1] };
        g_ghostSprite->SetColor(ghostCol);
    }

    if (g_basutaSprite)
    {
        XMFLOAT2 basutaDrawPos = { g_basutaPos.x + g_basutaOffset.x, g_basutaPos.y + g_basutaOffset.y };
        g_basutaSprite->SetPos(basutaDrawPos);
        // 色のアルファを更新
        XMFLOAT4 basutaCol = { 1.0f, 1.0f, 1.0f, alpha[2] };
        g_basutaSprite->SetColor(basutaCol);
    }

    //if (Keyboard_IsKeyDownTrigger(KK_E))
    //{
    //    SetScene(SCENE_TITLE);
    //}
}

void OpAnimDraw(void)
{
    Shader_SetMatrix(XMMatrixOrthographicOffCenterLH
    (0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

    SetBlendState(BLENDSTATE_ALFA);

    // 1) 屋敷を描画
    if (g_yakataSprite)
    {
        g_yakataSprite->Draw();
    }

    // 2) 幽霊を描画（反転 + スケール変化）
    if (alpha[1] > 0.0f && g_ghostSprite)
    {
        // flip処理：幽霊が左向きの場合は左右反転
        if (g_ghostFacingLeft)
        {
            g_ghostSprite->SetFlipType(FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL);
        }
        else
        {
            g_ghostSprite->SetFlipType(FLIPTYPE2D::FLIPTYPE2D_NONE);
        }
        g_ghostSprite->Draw();
    }

    // 3) basutaを描画（ふわふわ追加）
    if (alpha[2] > 0.0f && g_basutaSprite)
    {
        g_basutaSprite->Draw();
    }
}