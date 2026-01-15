#pragma once

enum SCENE {
	SCENE_TITLE = 0,
	SCENE_GAME,
	SCENE_RESULT,
	SCENE_ANM_LOGO,
	SCENE_ANM_OP,
	SCENE_ANM_WIN,
	SCENE_ANM_LOSE,
	SCENE_ANM_LOSE_ED,
	SCENE_MAX,
	SCENE_NONE,
	SCENE_WIN,
};

void Init(void);
void Update(void);
void Draw(void);
void Finalize(void);

void SetScene(SCENE id);
SCENE GetScene(void);
