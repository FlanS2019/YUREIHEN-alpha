#pragma once

#define SCARECOMBO_MAX (5)
#define SCARECOMBO_OVER_TIME (5000)// ミリ秒
#define SCARECOMBO_BAR_SIZE_X (140.0f)
#define SCARECOMBO_BAR_POS_X (SCREEN_WIDTH - 200.0f)

// 恐怖コンボの初期化
void UI_ScareCombo_Initialize(void);

// 恐怖コンボの更新
void UI_ScareCombo_Update(void);

// 恐怖コンボの描画
void UI_ScareCombo_Draw(void);

// 恐怖コンボの終了
void UI_ScareCombo_Finalize(void);

// 恐怖コンボを加算
void ScareComboUP(void);

int UI_ScareCombo_GetNumber(void);
