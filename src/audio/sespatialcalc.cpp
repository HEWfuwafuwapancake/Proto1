#include "audio/sespatialcalc.h"

#include <cmath>

namespace
{
	// 調整用の値
	static constexpr float NEAR_DISTANCE = 6.0f; // この距離（m）までは小さくしない
	static constexpr float FAR_DISTANCE = 40.0f; // この距離（m）でいちばん小さくなる
	static constexpr float FAR_VOLUME = 0.3f; // いちばん遠いときの大きさ
	static constexpr float THIRD_PARTY_VOLUME = 0.7f; // この PC で遊ぶ人が関わらない音の大きさ
	static constexpr float THIRD_PARTY_SILENT_DISTANCE = 30.0f; // この PC で遊ぶ人が関わらない音は、この距離（m）で聞こえなくなる
	static constexpr float PAN_MAX = 0.7f; // 左右の寄せ方の最大（1.0 で片方だけから鳴る）
	static constexpr float PAN_FULL_DISTANCE = 2.0f; // この距離（m）より近い音は、近いほど真ん中に寄せる（自分の体の音が左右に跳ねない）

	static constexpr float HALF_PI = 1.57079632679f;
}

SE_MIX SeSpatialCalcMix(const SE_LISTENER* listeners, int count, const VEC3& source, int slotA, int slotB)
{
	SE_MIX mix;
	if (listeners == nullptr || count <= 0) return mix;
	// 音の出た場所にいちばん近い聞き手を選ぶ（同じ距離なら前の人）。
	float best = -1.0f;
	for (int i = 0; i < count && i < SE_LISTENER_MAX; ++i) {
		const float dx = source.X - listeners[i].Position.X;
		const float dy = source.Y - listeners[i].Position.Y;
		const float dz = source.Z - listeners[i].Position.Z;
		const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
		if (best < 0.0f || distance < best) {
			best = distance;
			mix.Listener = i;
		}
	}
	const SE_LISTENER& listener = listeners[mix.Listener];

	// この PC で遊ぶ人が当てた・当てられた音か。
	bool involved = false;
	for (int i = 0; i < count && i < SE_LISTENER_MAX; ++i) {
		const int slot = listeners[i].Slot;
		if (slot >= 0 && (slot == slotA || slot == slotB)) involved = true;
	}

	if (involved) {
		// 自分の関わる音：近くはそのまま、遠くへ行くほど FAR_VOLUME へ近づける（遠くで当てても聞こえる）。
		float t = (best - NEAR_DISTANCE) / (FAR_DISTANCE - NEAR_DISTANCE);
		t = fminf(1.0f, fmaxf(0.0f, t));
		mix.Volume = 1.0f + (FAR_VOLUME - 1.0f) * t;
	}
	else {
		// ほかの人どうしの音：少し小さくし、遠いほど小さく、THIRD_PARTY_SILENT_DISTANCE で聞こえなくする。
		float t = (best - NEAR_DISTANCE) / (THIRD_PARTY_SILENT_DISTANCE - NEAR_DISTANCE);
		t = fminf(1.0f, fmaxf(0.0f, t));
		mix.Volume = THIRD_PARTY_VOLUME * (1.0f - t);
	}

	// 左右：聞き手から音への水平の向きと、カメラの右向きの内積。
	const float dx = source.X - listener.Position.X;
	const float dz = source.Z - listener.Position.Z;
	const float flat = sqrtf(dx * dx + dz * dz);
	const float rightLength = sqrtf(listener.Right.X * listener.Right.X + listener.Right.Z * listener.Right.Z);
	if (flat > 0.0001f && rightLength > 0.0001f) {
		const float side = (dx * listener.Right.X + dz * listener.Right.Z) / (flat * rightLength);
		const float nearScale = fminf(1.0f, flat / PAN_FULL_DISTANCE);
		mix.Pan = side * PAN_MAX * nearScale;
	}
	return mix;
}

void SeSpatialCalcPanGains(float pan, float& left, float& right)
{
	pan = fminf(1.0f, fmaxf(-1.0f, pan));
	const float angle = (pan + 1.0f) * 0.5f * HALF_PI;
	left = cosf(angle);
	right = sinf(angle);
}
