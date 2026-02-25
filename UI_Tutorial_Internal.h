// UTF-8 BOM
#pragma once
// ==========================================
// UI_Tutorial 内部ヘルパー（Tutorial_Pages.cpp から使う）
// UI_Tutorial.cpp 以外からは include しないこと
// ==========================================

#include "define.h"
#include "UI_Tutorial.h"
#include "TutorialFlag.h"
#include <vector>
#include <string>
#include <functional>

// ------------------------------------------
// ページ追加ヘルパー
// ------------------------------------------

void AddPage(
    const XMFLOAT2& holeCenter, float holeRadius,
    const std::vector<std::string>& texts,
    XMFLOAT2 textPos = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
    float fontSize = 40.0f);

void AddPage_Play(
    const std::vector<std::string>& texts, bool* pFlag,
    XMFLOAT2 textPos = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
    float fontSize = 40.0f,
    int delayFrames = 0);

// FlagWithDelay を受け取るオーバーロード
void AddPage_Play(
    const std::vector<std::string>& texts, FlagWithDelay flagWithDelay,
    XMFLOAT2 textPos = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
    float fontSize = 40.0f);

void AddPage_Camera(
    const XMFLOAT2& holeCenter, float holeRadius,
    const std::vector<std::string>& texts,
    const XMFLOAT3& targetPos, const XMFLOAT3& targetAt,
    XMFLOAT2 textPos = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
    float fontSize = 40.0f);

void SetCameraFocusPoint(const XMFLOAT3& pos);

// ------------------------------------------
// オブジェクト制御ヘルパー
// 次に登録する AddPage 系の onEnter コールバックに積まれる
// ------------------------------------------
void SetTutorialMarker(bool use, const XMFLOAT3& pos = { 0.0f, 0.0f, 0.0f });
void SetTutorialBuster(bool use, const XMFLOAT3& pos = { 0.0f, 0.0f, 0.0f });
void SetEnbanVisible(bool visible);

// バスターズに調査対象座標を設定する（次ページの onEnter に積まれる）
void SetTutorialBusterTarget(const XMFLOAT3& pos);

// ------------------------------------------
// Tutorial_Pages.cpp が実装する関数
// ------------------------------------------
void Tutorial_Pages_Init();
