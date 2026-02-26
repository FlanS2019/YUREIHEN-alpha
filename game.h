/*==============================================================================

   ポリゴン描画 [game.h]
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef GAME_H
#define GAME_H

#include <d3d11.h>

enum MOVE
{
	STOP = 0,
	RIGHT,
	LEFT
};

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Game_Finalize(void);
void Game_Update(void);
void Game_Draw(void);

// チュートリアル用：円盤接触フラグのポインタを返す
bool* Game_GetEnbanTouchedPtr(void);

// フロア降下アニメーション中かどうか
bool  Game_IsFloorExitAnimActive(void);

#endif // GAME_H
