#pragma once

// スコアをサーバーへ送信する（同期版・ブロッキング）
// 戻り値: 成功なら true
bool Score_SendToServer(int score);

// スコアをローカルに保存する
void Score_SaveLocal(int score);
	
// ローカルに保存されたスコアを取得する
int Score_GetLocal	();
