// UTF-8 BOM
#pragma execution_character_set("utf-8")

#include "UI_Tutorial_Internal.h"
#include "Tutorial_Object.h"
#include "define.h"

// ==========================================
// チュートリアルページ登録
// ここがあなたが触る部分です。
// AddPage / AddPage_Play / AddPage_Camera /
// SetCameraFocusPoint / SetTutorialMarker /
// SetTutorialBuster を使ってページを追加してください。
// ==========================================
void Tutorial_Pages_Init()
{
	// --- ウェルカムメッセージ ---
	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"遊んでくれてありがとう！「幽霊変」の遊び方を説明していくね！"
		});

	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"まずは操作説明！",
		"[W][A][S][D] で移動、マウスで視点を動かせるよ",
		"前に進んで円盤に触れてみよう！"
		});

	// --- 移動操作テストプレイ ---
	SetTutorialMarker(true, { -5.0f, 0.5f, 17.0f });
	SetEnbanVisible(true);
	AddPage_Play(
		{ "[W][A][S][D] 移動、マウスで視点" },
		TutorialObject_GetEnbanTouchedPtr(),
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	SetTutorialMarker(false);
	SetEnbanVisible(false);
	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"移動は完璧！",
		"次はゲームの目的、「敵を驚かせて追い払う！」について説明するね。"
		});

	AddPage_Camera({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 300.0f,
		{ "これが家具の一つのピアノ。"
		  "[スペースキー]で憑依だよ！" },
		{ -15.0f, 3.0f, 16.5f }, { -24.5f, 0.5f, 16.5f },
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	// --- ピアノ憑依テストプレイ ---
	SetCameraFocusPoint({ -15.0f, 3.0f, 16.5f });
	AddPage_Play(
		{ "[W][A][S][D]移動 [マウス]視点 [スペースキー]憑依" },
		TutorialObject_GetPianoPossessedPtr(),
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"憑依できたね！ おや？この影は？"
		});

	SetTutorialBuster(true, { -23.0f, BUSTERS_HEIGHT, 5.0f });

	AddPage_Camera({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 300.0f,
		{ "うわっ！侵入者の「バスターず」だ！",
		  "近づいてきたら、[スペースキー]で驚かせよう！" },
		{ -23.0f, 1.0f, 10.0f }, { -23.0f, 1.0f, 5.0f },
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	// バスターズへの驚かせテストプレイ
	SetTutorialBusterTarget({ -24.5f, 0.5f, 16.5f });
	AddPage_Play(
		{ "近づいてくるまで待ち、[スペースキー]驚かせ" },
		TutorialObject_GetBustersStunnedPtr(),
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	AddPage({ 120.0f, 120.0f }, 150.0f, {
		"制限時間はにつき２分。過ぎると強制的に負けちゃうよ"
		});

	AddPage({ 1023.0f, 83.0f }, 200.0f, {
		"これは「恐怖ゲージ」バスターズをうまく驚かせられると、溜まっていくよ！",
		"右まで貯めるとバスターズが逃げてステージクリア！次の階へ進もう"
		});

	AddPage({ 1163.0f, 201.0f }, 90.0f, {
		"これは「恐怖コンボ」。バスターズへの驚かせが連鎖すると、恐怖ゲージが倍増するよ！"
		});

	AddPage({ 147.0f, 563.0f }, 150.0f, {
		"これはミニマップ。動きの参考にしよう"
		});

	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 150.0f, {
		"あとは実際にやってみるターンを作る。一旦ゲームスタート（仮テキスト）"
		});
}
