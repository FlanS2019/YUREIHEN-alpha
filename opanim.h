#pragma once
#include <d3d11.h>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cmath>

// オープニングアニメーションシステム
// 各要素をクラス化してカプセル化
void OpAnim_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void OpAnim_Finalize(void);
void OpAnim_Update();
void OpAnimDraw(void);

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ユーティリティ
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
namespace OpAnimUtil {
	static unsigned int g_RandomSeed = 0xC0FFEEu;
	inline float Rand01() {
		g_RandomSeed = g_RandomSeed * 1664525u + 1013904223u;
		return (float)(g_RandomSeed & 0x00FFFFFFu) / (float)0x01000000u;
	}
	inline float EaseOutCubic(float t) {
		if (t <= 0.0f) return 0.0f;
		if (t >= 1.0f) return 1.0f;
		float inv = 1.0f - t;
		return 1.0f - inv * inv * inv;
	}
	inline float Clamp(float value, float min, float max) {
		if (value < min) return min;
		if (value > max) return max;
		return value;
	}
	inline float Distance(const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b) {
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		return sqrtf(dx * dx + dy * dy);
	}
	inline float Lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}
}

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// トゥイーンシステム（汎用アニメーション制御）
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
enum class EasingType {
	Linear,
	EaseOutCubic,
	EaseInOutQuad
};

template<typename T>
class Tween {
private:
	T m_start;
	T m_end;
	float m_duration;
	float m_elapsed;
	EasingType m_easing;
	bool m_active;
	std::function<void(const T&)> m_onUpdate;

	float ApplyEasing(float t) {
		switch (m_easing) {
		case EasingType::EaseOutCubic: return OpAnimUtil::EaseOutCubic(t);
		case EasingType::EaseInOutQuad: {
			t *= 2.0f;
			if (t < 1.0f) return 0.5f * t * t;
			t -= 1.0f;
			return -0.5f * (t * (t - 2.0f) - 1.0f);
		}
		default: return t;
		}
	}

public:
	Tween() : m_duration(0), m_elapsed(0), m_active(false), m_easing(EasingType::Linear) {}

	void Start(const T& start, const T& end, float duration, EasingType easing = EasingType::Linear) {
		m_start = start;
		m_end = end;
		m_duration = duration;
		m_elapsed = 0.0f;
		m_easing = easing;
		m_active = true;
	}

	void SetCallback(std::function<void(const T&)> callback) { m_onUpdate = callback; }

	bool Update(float deltaTime) {
		if (!m_active) return false;
		m_elapsed += deltaTime;
		float t = OpAnimUtil::Clamp(m_elapsed / m_duration, 0.0f, 1.0f);
		float easedT = ApplyEasing(t);

		if (m_onUpdate) {
			UpdateValue(easedT);
		}

		if (m_elapsed >= m_duration) {
			m_active = false;
			return false;
		}
		return true;
	}

	bool IsActive() const { return m_active; }

private:
	void UpdateValue(float easedT) {}
};

template<>
class Tween<float> {
private:
	float m_start;
	float m_end;
	float m_duration;
	float m_elapsed;
	EasingType m_easing;
	bool m_active;
	std::function<void(const float&)> m_onUpdate;

	float ApplyEasing(float t) {
		switch (m_easing) {
		case EasingType::EaseOutCubic: return OpAnimUtil::EaseOutCubic(t);
		case EasingType::EaseInOutQuad: {
			t *= 2.0f;
			if (t < 1.0f) return 0.5f * t * t;
			t -= 1.0f;
			return -0.5f * (t * (t - 2.0f) - 1.0f);
		}
		default: return t;
		}
	}

public:
	Tween() : m_duration(0), m_elapsed(0), m_active(false), m_easing(EasingType::Linear) {}

	void Start(const float& start, const float& end, float duration, EasingType easing = EasingType::Linear) {
		m_start = start;
		m_end = end;
		m_duration = duration;
		m_elapsed = 0.0f;
		m_easing = easing;
		m_active = true;
	}

	void SetCallback(std::function<void(const float&)> callback) { m_onUpdate = callback; }

	bool Update(float deltaTime) {
		if (!m_active) return false;
		m_elapsed += deltaTime;
		float t = OpAnimUtil::Clamp(m_elapsed / m_duration, 0.0f, 1.0f);
		float easedT = ApplyEasing(t);

		if (m_onUpdate) {
			m_onUpdate(OpAnimUtil::Lerp(m_start, m_end, easedT));
		}

		if (m_elapsed >= m_duration) {
			m_active = false;
			return false;
		}
		return true;
	}

	bool IsActive() const { return m_active; }
};

template<>
class Tween<DirectX::XMFLOAT2> {
private:
	DirectX::XMFLOAT2 m_start;
	DirectX::XMFLOAT2 m_end;
	float m_duration;
	float m_elapsed;
	EasingType m_easing;
	bool m_active;
	std::function<void(const DirectX::XMFLOAT2&)> m_onUpdate;

	float ApplyEasing(float t) {
		switch (m_easing) {
		case EasingType::EaseOutCubic: return OpAnimUtil::EaseOutCubic(t);
		case EasingType::EaseInOutQuad: {
			t *= 2.0f;
			if (t < 1.0f) return 0.5f * t * t;
			t -= 1.0f;
			return -0.5f * (t * (t - 2.0f) - 1.0f);
		}
		default: return t;
		}
	}

public:
	Tween() : m_duration(0), m_elapsed(0), m_active(false), m_easing(EasingType::Linear) {}

	void Start(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float duration, EasingType easing = EasingType::Linear) {
		m_start = start;
		m_end = end;
		m_duration = duration;
		m_elapsed = 0.0f;
		m_easing = easing;
		m_active = true;
	}

	void SetCallback(std::function<void(const DirectX::XMFLOAT2&)> callback) { m_onUpdate = callback; }

	bool Update(float deltaTime) {
		if (!m_active) return false;
		m_elapsed += deltaTime;
		float t = OpAnimUtil::Clamp(m_elapsed / m_duration, 0.0f, 1.0f);
		float easedT = ApplyEasing(t);

		if (m_onUpdate) {
			DirectX::XMFLOAT2 value = {
				OpAnimUtil::Lerp(m_start.x, m_end.x, easedT),
				OpAnimUtil::Lerp(m_start.y, m_end.y, easedT)
			};
			m_onUpdate(value);
		}

		if (m_elapsed >= m_duration) {
			m_active = false;
			return false;
		}
		return true;
	}

	bool IsActive() const { return m_active; }
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// タイムラインイベントシステム
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class TimelineEvent {
private:
	float m_triggerTime;
	bool m_triggered;
	std::function<void()> m_action;

public:
	TimelineEvent(float triggerTime, std::function<void()> action)
		: m_triggerTime(triggerTime), m_triggered(false), m_action(action) {
	}

	bool Update(float currentTime) {
		if (!m_triggered && currentTime >= m_triggerTime) {
			m_triggered = true;
			if (m_action) m_action();
			return true;
		}
		return false;
	}

	bool IsTriggered() const { return m_triggered; }
	void Reset() { m_triggered = false; }
};

class Timeline {
private:
	std::vector<TimelineEvent> m_events;
	float m_currentTime;

public:
	Timeline() : m_currentTime(0.0f) {}

	void AddEvent(float time, std::function<void()> action) {
		m_events.emplace_back(time, action);
	}

	void Update(float deltaTime) {
		m_currentTime += deltaTime;
		for (auto& event : m_events) {
			event.Update(m_currentTime);
		}
	}

	void Reset() {
		m_currentTime = 0.0f;
		for (auto& event : m_events) {
			event.Reset();
		}
	}

	float GetCurrentTime() const { return m_currentTime; }
};

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ステートマシン
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
enum class AnimState {
	Idle,
	Moving,
	Reacting,
	Exiting,
	Finished
};

template<typename Owner>
class StateMachine {
private:
	Owner* m_owner;
	AnimState m_currentState;
	std::unordered_map<int, std::function<void(float)>> m_states;

public:
	StateMachine(Owner* owner) : m_owner(owner), m_currentState(AnimState::Idle) {}

	void RegisterState(AnimState state, std::function<void(float)> updateFunc) {
		m_states[static_cast<int>(state)] = updateFunc;
	}

	void SetState(AnimState newState) {
		m_currentState = newState;
	}

	void Update(float deltaTime) {
		auto it = m_states.find(static_cast<int>(m_currentState));
		if (it != m_states.end()) {
			it->second(deltaTime);
		}
	}

	AnimState GetCurrentState() const { return m_currentState; }
};