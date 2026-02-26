// UTF-8 BOM
#pragma once

// bool* と遅延フレーム数をまとめた構造体
// AddPage_Play と TutorialObject_GetXxxPtr の間で遅延を受け渡すために使用する
struct FlagWithDelay
{
    bool* pFlag;
    int   delayFrames;
};
