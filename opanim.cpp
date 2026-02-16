// opanim.cpp
#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "opanim.h"
#include "mouse.h"
#include "sound.h"
#include "font.h"
#include <timeapi.h>
#include <cmath>
#include <memory>
#include "define.h"
#pragma comment(lib, "winmm.lib")

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 背景（変更なし）
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class OpBackground {
private:
	std::unique_ptr<Sprite> m_sprite;
public:
	void Initialize() {
		m_sprite = std::make_unique<Sprite>(
			XMFLOAT2{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
			XMFLOAT2{ SCREEN_WIDTH, SCREEN_HEIGHT },
			0.0f, XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f },
			BLENDSTATE_ALFA, L"asset\\yureihen\\Alpha_Tex\\yoru.png"
		);
	}
	void Draw() { if (m_sprite) m_sprite->Draw(); }
	void Finalize() { m_sprite.reset(); }
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 雨エフェクト（簡略化）
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class OpRainEffect {
private:
	struct RainDrop {
		std::unique_ptr<Sprite> sprite;
		XMFLOAT2 pos;
		float speed, alpha, swayPhase, swayAmp;

		void Reset() {
			pos.x = OpAnimUtil::Rand01() * SCREEN_WIDTH;
			pos.y = -(OpAnimUtil::Rand01() * 120.0f + 20.0f);
			speed = 200.0f + OpAnimUtil::Rand01() * 400.0f;
			alpha = 0.3f + OpAnimUtil::Rand01() * 0.6f;
			swayPhase = OpAnimUtil::Rand01() * 6.2831853f;
			swayAmp = 6.0f + OpAnimUtil::Rand01() * 10.0f;
		}
	};

	std::vector<RainDrop> m_rainDrops;
	static constexpr int RAIN_COUNT = 1000;

public:
	void Initialize() {
		m_rainDrops.resize(RAIN_COUNT);
		for (auto& rd : m_rainDrops) {
			rd.Reset();
			rd.sprite = std::make_unique<Sprite>(
				rd.pos, XMFLOAT2{ 100.0f, 100.0f }, 0.0f,
				XMFLOAT4{ 1.0f, 1.0f, 1.0f, rd.alpha },
				BLENDSTATE_ALFA, L"asset\\yureihen\\Op_rain.png"
			);
		}
	}

	void Update(float elapsedSeconds) {
		constexpr float dt = 1.0f / 40.0f;
		for (auto& rd : m_rainDrops) {
			rd.pos.y += rd.speed * dt;
			float sway = sinf(elapsedSeconds * 2.0f + rd.swayPhase) * rd.swayAmp;
			rd.sprite->SetPos({ rd.pos.x + sway, rd.pos.y });
			if (rd.pos.y > SCREEN_HEIGHT + 100.0f) rd.Reset();
		}
	}

	void Draw() { for (auto& rd : m_rainDrops) rd.sprite->Draw(); }
	void Finalize() { m_rainDrops.clear(); }
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 稲妻（トゥイーン使用）
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class OpLightning {
private:
	std::unique_ptr<Sprite> m_sprite;
	float m_nextTrigger;
	Tween<float> m_flashTween;

public:
	void Initialize() {
		m_sprite = std::make_unique<Sprite>(
			XMFLOAT2{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
			XMFLOAT2{ 1280.0f, 720.0f }, 0.0f,
			XMFLOAT4{ 0.95f, 0.95f, 1.0f, 0.5f },
			BLENDSTATE_ADD, L"asset\\yureihen\\inazuma2.png"
		);
		m_nextTrigger = 0.5f + OpAnimUtil::Rand01() * 1.5f;

		m_flashTween.SetCallback([this](const float& alpha) {
			m_sprite->SetColor({ 0.95f, 0.95f, 1.0f, 0.5f + alpha });
			});
	}

	void Update(float dt) {
		if (!m_flashTween.Update(dt)) {
			m_nextTrigger -= dt;
			if (m_nextTrigger <= 0.0f) {
				float duration = 0.08f + OpAnimUtil::Rand01() * 0.12f;
				m_flashTween.Start(1.0f, 0.0f, duration, EasingType::EaseOutCubic);
				m_nextTrigger = 1.0f + OpAnimUtil::Rand01() * 2.5f;
			}
		}
	}

	void Draw() { if (m_sprite) m_sprite->Draw(); }
	void Finalize() { m_sprite.reset(); }
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 屋敷（変更なし）
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class OpMansion {
private:
	std::unique_ptr<Sprite> m_sprite;
	XMFLOAT2 m_position;
public:
	void Initialize() {
		m_position = { SCREEN_WIDTH / 2.0f - 320.0f, SCREEN_HEIGHT / 2.0f + 20 };// 少し左寄せ
		m_sprite = std::make_unique<Sprite>(
			m_position, XMFLOAT2{ 700.0f, 700.0f }, 0.0f,
			XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f },
			BLENDSTATE_ALFA, L"asset\\yureihen\\Alpha_Tex\\yakata_jimen.png"
		);
	}
	void Draw() { if (m_sprite) m_sprite->Draw(); }
	void Finalize() { m_sprite.reset(); }
	XMFLOAT2 GetPosition() const { return m_position; }
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// バスター（ステートマシン + トゥイーン）
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class OpBuster {
private:
	std::unique_ptr<Sprite> m_sprite;
	XMFLOAT2 m_currentPos;
	float m_wobblePhase;
	StateMachine<OpBuster> m_stateMachine;
	Tween<XMFLOAT2> m_moveTween;
	Tween<float> m_alphaTween;

	void IdleState(float dt) {}

	void MovingState(float dt) {
		m_wobblePhase += dt * 3.5f;
		XMFLOAT2 pos = m_currentPos;
		pos.y += sinf(m_wobblePhase) * 8.0f;
		m_sprite->SetPos(pos);
	}

public:
	OpBuster() : m_stateMachine(this), m_wobblePhase(0.0f) {}

	void Initialize(const XMFLOAT2& mansionPos) {
		XMFLOAT2 start = { SCREEN_WIDTH + 200.0f, SCREEN_HEIGHT / 2.0f + 50.0f };
		XMFLOAT2 target = { mansionPos.x - 150.0f, mansionPos.y + 50.0f };
		m_currentPos = start;

		m_sprite = std::make_unique<Sprite>(
			m_currentPos, XMFLOAT2{ 500.0f, 500.0f }, 0.0f,
			XMFLOAT4{ 1.0f, 1.0f, 1.0f, 0.0f },
			BLENDSTATE_ALFA, L"asset\\yureihen\\Busters_OP.png"
		);

		m_stateMachine.RegisterState(AnimState::Idle, [this](float dt) { IdleState(dt); });
		m_stateMachine.RegisterState(AnimState::Moving, [this](float dt) { MovingState(dt); });

		// トゥイーン設定
		m_moveTween.SetCallback([this](const XMFLOAT2& pos) { m_currentPos = pos; });
		m_alphaTween.SetCallback([this](const float& alpha) {
			m_sprite->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
			});
	}

	void StartMoving(const XMFLOAT2& start, const XMFLOAT2& target) {
		m_moveTween.Start(start, target, 13.0f, EasingType::Linear);
		m_alphaTween.Start(0.0f, 1.0f, 2.0f, EasingType::Linear);
		m_stateMachine.SetState(AnimState::Moving);
	}

	void Update(float dt) {
		m_moveTween.Update(dt);
		m_alphaTween.Update(dt);
		m_stateMachine.Update(dt);
	}

	void Draw() { if (m_sprite) m_sprite->Draw(); }
	void Finalize() { m_sprite.reset(); }
	XMFLOAT2 GetPosition() const { return m_currentPos; }
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 幽霊（ステートマシン + トゥイーン）
// 変更点：振り向いてからワンテンポ（遅延）置いてから斜め左に消える処理を追加
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class OpGhost {
private:
	std::unique_ptr<Sprite> m_sprite;
	std::unique_ptr<Sprite> m_exclamation;
	XMFLOAT2 m_basePos, m_currentPos, m_currentSize;
	float m_wobblePhase;
	bool m_flipped;
	StateMachine<OpGhost> m_stateMachine;
	Tween<float> m_appearTween, m_exitTween, m_exclamationTween;
	Tween<XMFLOAT2> m_moveTween, m_sizeTween;

	float m_reactDelay;
	bool m_reactTweenStarted;

	void IdleState(float dt) {
		m_wobblePhase += dt * 4.5f;
		m_currentPos.y = m_basePos.y + sinf(m_wobblePhase) * 8.0f;
		m_sprite->SetPos(m_currentPos);
	}

	void ReactingState(float dt) {
		m_wobblePhase += dt * 4.5f;
		m_currentPos.y = m_basePos.y + sinf(m_wobblePhase) * 8.0f;
		m_sprite->SetPos(m_currentPos);
	}

public:
	OpGhost() : m_stateMachine(this), m_wobblePhase(0.0f), m_flipped(false), m_reactDelay(0.0f), m_reactTweenStarted(false) {}

	void Initialize(const XMFLOAT2& mansionPos) {
		m_basePos = { mansionPos.x + 40.0f, mansionPos.y - 50.0f };
		m_currentPos = m_basePos;
		m_currentSize = { 600.0f, 425.0f };

		m_sprite = std::make_unique<Sprite>(
			m_currentPos, m_currentSize, 0.0f,
			XMFLOAT4{ 1.0f, 1.0f, 1.0f, 0.0f },
			BLENDSTATE_ALFA, L"asset\\yureihen\\Alpha_Tex\\yurei2.png"
		);

		m_exclamation = std::make_unique<Sprite>(
			XMFLOAT2{ m_currentPos.x, m_currentPos.y - 50.0f },
			XMFLOAT2{ 180.0f, 135.0f }, 0.0f,
			XMFLOAT4{ 1.0f, 1.0f, 1.0f, 0.0f },
			BLENDSTATE_ALFA, L"asset\\yureihen\\Alpha_Tex\\bikkuri2.png"
		);

		m_stateMachine.RegisterState(AnimState::Idle, [this](float dt) { IdleState(dt); });
		m_stateMachine.RegisterState(AnimState::Reacting, [this](float dt) { ReactingState(dt); });

		m_appearTween.SetCallback([this](const float& alpha) {
			m_sprite->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		});
		m_moveTween.SetCallback([this](const XMFLOAT2& pos) {
			m_currentPos = pos;
			m_sprite->SetPos(m_currentPos);
		});
		m_sizeTween.SetCallback([this](const XMFLOAT2& size) {
			m_currentSize = size;
			m_sprite->SetSize(size);
		});
		m_exitTween.SetCallback([this](const float& alpha) {
			m_sprite->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		});
		m_exclamationTween.SetCallback([this](const float& alpha) {
			m_exclamation->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		});
	}

	void StartAppearing() {
		m_appearTween.Start(0.0f, 1.0f, 1.5f, EasingType::Linear);
		m_stateMachine.SetState(AnimState::Idle);
	}

	void StartReacting() {
		m_flipped = true;
		m_sprite->SetFlipType(FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL);
		m_exclamation->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

		m_reactDelay = 0.6f;
		m_reactTweenStarted = false;

		m_stateMachine.SetState(AnimState::Reacting);
	}

	void Update(float dt, const XMFLOAT2& busterPos) {
		m_appearTween.Update(dt);
		m_moveTween.Update(dt);
		m_sizeTween.Update(dt);
		m_exitTween.Update(dt);
		m_exclamationTween.Update(dt);
		m_stateMachine.Update(dt);

		if (m_reactDelay > 0.0f) {
			m_reactDelay -= dt;
			if (m_reactDelay <= 0.0f && !m_reactTweenStarted) {
				// ビックリマークをすぐに消す
				m_exclamationTween.Start(1.0f, 0.0f, 0.3f, EasingType::Linear);

				// 画面左に移動
				XMFLOAT2 exitPos = { -300.0f, m_currentPos.y };
				m_moveTween.Start(m_currentPos, exitPos, 6.0f, EasingType::Linear);
				m_sizeTween.Start(m_currentSize, XMFLOAT2{ 150.0f, 106.25f }, 6.0f, EasingType::EaseOutCubic);
				m_exitTween.Start(1.0f, 0.0f, 6.0f, EasingType::Linear);
				m_reactTweenStarted = true;
			}
		}

		if (m_exclamation) {
			m_exclamation->SetPos({ m_currentPos.x, m_currentPos.y - 100.0f });
		}
	}

	void Draw() {
		if (m_sprite) m_sprite->Draw();
		if (m_exclamation) m_exclamation->Draw();
	}

	void Finalize() {
		m_sprite.reset();
		m_exclamation.reset();
	}

	bool ShouldReact(const XMFLOAT2& busterPos) const {
		return OpAnimUtil::Distance(busterPos, m_basePos) < 3600.0f;
	}
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// テキスト（トゥイーン使用）
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class OpTextRenderer {
private:
	std::unique_ptr<FontRenderer> m_fonts[3];
	Tween<float> m_alphaTweens[3];

public:
	void Initialize() {
		// 初期は透明に設定しておく（出現時にフェードインするため）
		m_fonts[0] = std::make_unique<FontRenderer>(
			XMFLOAT2{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 60 },
			70.0f, 0.0f, XMFLOAT4{ 1.0f, 1.0f, 1.0f, 0.0f }, // alpha = 0.0f
			"平和に暮らしていたはずの幽霊ちゃん・・・。"
		);
		m_fonts[1] = std::make_unique<FontRenderer>(
			XMFLOAT2{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 60 },
			70.0f, 0.0f, XMFLOAT4{ 1.0f, 1.0f, 1.0f, 0.0f }, // alpha = 0.0f
			"おや、怪しい人達が現れましたよ？"
		);
		m_fonts[2] = std::make_unique<FontRenderer>(
			XMFLOAT2{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 60 },
			70.0f, 0.0f, XMFLOAT4{ 1.0f, 1.0f, 1.0f, 0.0f }, // alpha = 0.0f
			"さてさて、追い出す準備でもいたしましょうか・・・。"
		);

		for (int i = 0; i < 3; ++i) {
			m_alphaTweens[i].SetCallback([this, i](const float& alpha) {
				if (m_fonts[i]) {
					XMFLOAT4 color = m_fonts[i]->GetColor();
					color.w = alpha;
					m_fonts[i]->SetColor(color);
				}
			});
		}
	}

	// 指定したフォントをフェードインして表示する（デフォルト1.0秒）
	void ShowFont(int index, float duration = 1.0f) {
		if (index < 0 || index >= 3) return;
		if (!m_fonts[index]) return;
		// 現在の alpha から 1.0f へフェード（既に一部見えていても対応）
		float cur = m_fonts[index]->GetColor().w;
		m_alphaTweens[index].Start(cur, 1.0f, duration, EasingType::Linear);
	}

	// 指定したフォントをフェードアウトする
	void HideFont(int index, float duration = 0.5f) {
		if (index < 0 || index >= 3) return;
		if (!m_fonts[index]) return;
		// 現在の alpha から 0.0f へフェード
		float cur = m_fonts[index]->GetColor().w;
		m_alphaTweens[index].Start(cur, 0.0f, duration, EasingType::Linear);
	}

	// 全体をフェードアウトする（現在の alpha -> 0.0f へ）
	void StartFades() {
		for (int i = 0; i < 3; ++i) {
			if (!m_fonts[i]) continue;
			float cur = m_fonts[i]->GetColor().w;
			m_alphaTweens[i].Start(cur, 0.0f, 0.5f, EasingType::Linear);
		}
	}

	void Update(float dt) {
		for (auto& tween : m_alphaTweens) tween.Update(dt);
	}

	void Draw() {
		for (auto& font : m_fonts) if (font) font->Draw();
	}

	void Finalize() {
		for (auto& font : m_fonts) font.reset();
	}
};
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// BGM管理（変更なし）
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class OpBGMManager {
private:
	SoundData* m_bgmGhost;
	SoundData* m_bgmLightningRain;
	bool m_played;
public:
	OpBGMManager() : m_bgmGhost(nullptr), m_bgmLightningRain(nullptr), m_played(false) {}
	void Initialize() {
		m_bgmGhost = LoadMP3("asset/sound/bgm/BGM1op.mp3");
		m_bgmLightningRain = LoadMP3("asset/sound/se/lightning_rain.mp3");
	}
	void Update() {
		if (!m_played) {
			if (m_bgmGhost) PlaySound(m_bgmGhost, true);
			if (m_bgmLightningRain) PlaySound(m_bgmLightningRain, true);
			m_played = true;
		}
	}
	void Finalize() {
		if (m_bgmGhost) { StopSound(m_bgmGhost); UnloadSound(m_bgmGhost); }
		if (m_bgmLightningRain) { StopSound(m_bgmLightningRain); UnloadSound(m_bgmLightningRain); }
	}
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// タイムライン制御統合システム
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class OpAnimationSystem {
private:
	OpBackground m_background;
	OpRainEffect m_rain;
	OpLightning m_lightning;
	OpMansion m_mansion;
	OpBuster m_buster;
	OpGhost m_ghost;
	OpTextRenderer m_text;
	OpBGMManager m_bgm;
	Timeline m_timeline;
	DWORD m_startTime;

public:
	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
		m_background.Initialize();
		m_rain.Initialize();
		m_lightning.Initialize();
		m_mansion.Initialize();
		m_buster.Initialize(m_mansion.GetPosition());
		m_ghost.Initialize(m_mansion.GetPosition());
		m_text.Initialize();
		m_bgm.Initialize();

		// タイムラインイベント設定（時間ベース管理を一元化）
		m_timeline.AddEvent(2.5f, [this]() 
		{
			m_ghost.StartAppearing();
			m_text.ShowFont(0, 0.5f);// テキスト0を2秒かけてフェードイン
		});

		m_timeline.AddEvent(10.0f, [this]()// 7.0秒：バスター出現開始
		{
			XMFLOAT2 start = { SCREEN_WIDTH + 200.0f, SCREEN_HEIGHT / 2.0f + 50.0f };
			XMFLOAT2 target = { m_mansion.GetPosition().x - 150.0f, m_mansion.GetPosition().y + 50.0f };
			m_buster.StartMoving(start, target);
		});

		m_timeline.AddEvent(8.0f, [this]()// 10.0秒：テキスト切り替え
		{
			m_text.HideFont(0, 1.0f);		// テキスト0を1秒かけてフェードアウト
			m_text.ShowFont(1, 1.0f);		// 同時にテキスト1を1秒かけてフェードイン
			
			if (m_ghost.ShouldReact(m_buster.GetPosition())) {
				m_ghost.StartReacting();
			}
		});

		m_timeline.AddEvent(13.0f, [this]() // 18.0秒：テキスト切り替え
			{
			m_text.HideFont(1, 0.5f);		// テキスト1を1秒かけてフェードアウト
			m_text.ShowFont(2, 0.5f);		// 同時にテキスト2を1秒かけてフェードイン
		});

		// 25.0秒：シーン遷移フェード開始
		m_timeline.AddEvent(17.0f, [this]() {
			if (GetFadeState() == FADE_NONE) {
				StartFade(SCENE_GAME);
			}
		});

		Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
		Mouse_SetVisible(true);
		m_startTime = timeGetTime();
	}

	void Update() {
		float elapsedSeconds = (timeGetTime() - m_startTime) / 1000.0f;
		float dt = 1.0f / 40.0f;

		m_bgm.Update();
		m_timeline.Update(dt);
		m_rain.Update(elapsedSeconds);
		m_lightning.Update(dt);
		m_buster.Update(dt);
		m_ghost.Update(dt, m_buster.GetPosition());
		m_text.Update(dt);

		if (Keyboard_IsKeyDownTrigger(KK_E)) {
			SetScene(SCENE_TITLE);
		}
	}

	void Draw() {
		m_background.Draw();
		m_rain.Draw();
		m_mansion.Draw();
		m_buster.Draw();
		m_ghost.Draw();
		m_lightning.Draw();
		m_text.Draw();
	}

	void Finalize() {
		m_background.Finalize();
		m_rain.Finalize();
		m_lightning.Finalize();
		m_mansion.Finalize();
		m_buster.Finalize();
		m_ghost.Finalize();
		m_text.Finalize();
		m_bgm.Finalize();
	}
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// グローバルインスタンス
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
static std::unique_ptr<OpAnimationSystem> g_OpAnimSystem = nullptr;

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 外部インターフェース
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
void OpAnim_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
	g_OpAnimSystem = std::make_unique<OpAnimationSystem>();
	g_OpAnimSystem->Initialize(pDevice, pContext);
}

void OpAnim_Finalize(void) {
	if (g_OpAnimSystem) {
		g_OpAnimSystem->Finalize();
		g_OpAnimSystem.reset();
	}
}

void OpAnim_Update() {
	if (g_OpAnimSystem) g_OpAnimSystem->Update();
}

void OpAnimDraw(void) {
	if (g_OpAnimSystem) g_OpAnimSystem->Draw();
}
