#pragma once

/// @file cpucalc.h
/// @brief CPU が今の状況から、人と同じ入力（PLAYER_INPUT）を作る計算（DirectX・Windows 不使用）
/// @details 同じ種と同じ状況なら同じ入力になる。腕の種類ごとの使い方は呼び出し側（game/cpu）が CPU_ARM にして渡す。

#include "input/actioncalc.h"
#include "physics/randomcalc.h"
#include "physics/stagecalc.h"
#include "physics/versuscalc.h"

/// @brief CPU が見るカプセルの最大数
static constexpr int CPU_CAPSULE_MAX = 16;

/// @brief 腕を場外からの復帰に使うときの押し方
enum CPU_RECOVERY
{
	CPU_RECOVERY_NONE, ///< 復帰に使わない
	CPU_RECOVERY_TAP, ///< 空中で単押しする（ハンマーの跳ねなど）
	CPU_RECOVERY_HOLD, ///< 空中で長押しして離す（ナックルの突進など）
};

/// @brief 片手の腕の使い方（腕の種類から作る）
struct CPU_ARM
{
	bool Usable = false; ///< 攻撃に使えるか（素手・耐久切れは false）
	float Range = 0.0f; ///< 相手がこの距離（m）以内なら攻撃する
	float HoldSeconds = 0.0f; ///< 長押しで使うときの押す秒数（0 なら長押しは使わない）
	float HoldChance = 0.0f; ///< 攻撃のうち長押しにする割合（0〜1）
	bool CanGuard = false; ///< ガードに使うか
	CPU_RECOVERY Recovery = CPU_RECOVERY_NONE; ///< 場外からの復帰に使うときの押し方
};

/// @brief CPU から見た相手1体
struct CPU_OPPONENT
{
	bool Available = false; ///< 狙える相手か（自分・復活待ち・隠れている相手は false）
	VEC3 Position = {}; ///< 足元の位置
	float Percent = 0.0f; ///< ダメージ%
};

/// @brief CPU が1フレームに見る状況
struct CPU_VIEW
{
	float Dt = 0.0f; ///< このフレームの経過秒数
	VEC3 Position = {}; ///< 自分の足元の位置
	bool OnGround = false; ///< 足場に立っているか
	bool CanAct = false; ///< 腕を使える状態か（吹っ飛ばされた直後・つかまれ中は false）
	float CameraYaw = 0.0f; ///< 自分のカメラの左右の向き（ラジアン。移動の入力はこの向きが前）
	int CameraTarget = -1; ///< 自分のカメラがロックオンしている相手のスロット（なしは -1）
	CPU_ARM Arms[2]; ///< 左手（0）・右手（1）の使い方
	bool WantsArm = false; ///< カプセルを拾いに行くか（GAME で空いている手があるとき true）
	CPU_OPPONENT Opponents[FIGHTER_COUNT]; ///< スロット番号ごとの相手（自分の番号は Available を false にする）
	VEC3 Capsules[CPU_CAPSULE_MAX] = {}; ///< 拾えるカプセルの位置
	int CapsuleCount = 0; ///< Capsules の有効な数
	STAGE Stage = {}; ///< ステージの形
};

/// @brief CPU の強さごとの値
struct CPU_PARAMS
{
	float ThinkSeconds; ///< 相手・攻める手・ガードを考え直す間隔（秒。反応の遅れ）
	float AttackSeconds; ///< 攻撃の間隔（秒）
	float GuardChance; ///< 相手が近いとき、考え直すたびにガードする割合（0〜1）
	float RangeScale; ///< 腕の届く距離に掛ける倍率（1 より小さいと近づいてから振る）
};

/// @brief 強さ（VERSUS_CPU_LEVEL）ごとの値を取得する（範囲外は「ふつう」）
CPU_PARAMS CpuCalcParams(int level);

/// @brief 足元から見た世界の水平の向きを、カメラの向きを前とする移動の入力（X：右、Y：前）に変える
/// @details Fighter の「入力 → 世界の向き」の逆の変換
ACTION_AXIS CpuCalcWorldToMove(float worldX, float worldZ, float cameraYaw);

/// @brief CPU 1体の考え（相手を選ぶ・近づく・攻撃する・守る・場外から戻る）を持ち、毎フレームの入力を作る
class CpuCalc
{
public:
	/// @brief 種・自分のスロット・強さで最初の状態にする（試合の始めとやり直しで呼ぶ）
	/// @param seed 乱数の種
	/// @param slot 自分のスロット番号（0〜3）
	/// @param level 強さ（VERSUS_CPU_EASY〜VERSUS_CPU_HARD）
	void Reset(uint32_t seed, int slot, int level);
	/// @brief 状況から1フレーム分の入力を作る
	PLAYER_INPUT Step(const CPU_VIEW& view);
	/// @brief 今ねらっている相手のスロットを取得する（なしは -1）
	int GetTarget() const { return m_Target; }

private:
	void Think(const CPU_VIEW& view);
	int ChooseHand(const CPU_VIEW& view, float distance) const;
	RandomCalc m_Random;
	CPU_PARAMS m_Params = {};
	int m_Slot = 0;
	int m_Target = -1;
	float m_ThinkTimer = 0.0f; // 次に考え直すまでの秒数
	float m_AttackTimer = 0.0f; // 次に攻撃できるまでの秒数
	float m_JumpTimer = 0.0f; // 次に空中ジャンプできるまでの秒数
	float m_TargetTimer = 0.0f; // 次にロックオンを切り替えられるまでの秒数
	float m_GuardTimer = 0.0f; // ガードを続ける残り秒数
	int m_GuardHand = -1; // ガードしている手（なしは -1）
	float m_HoldTimer = 0.0f; // 長押しを続ける残り秒数
	int m_HoldHand = -1; // 長押ししている手（なしは -1）
	bool m_AirDashed = false; // 足場を離れてからダッシュを使ったか
	bool m_AirRecovered = false; // 足場を離れてから腕の復帰技を使ったか
};
