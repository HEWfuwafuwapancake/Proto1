#pragma once

#include "physics/vec3.h"

/// @brief 対戦に使う体の数（人とダミーを合わせて4体）
static constexpr int FIGHTER_COUNT = 4;
/// @brief 入力を持たないダミーの機器番号
static constexpr int VERSUS_DEVICE_DUMMY = -1;
/// @brief 段階Bの1人用入力（パッド0とキーボードを合成する）
static constexpr int VERSUS_DEVICE_COMBINED = -2;

/// @brief 体当たりの重なりを解く（人対ダミーは従来どおり、人対人は横の押し出しを半分ずつ受ける）
/// @param firstHuman 1体目の種類を設定する（true：人、false：ダミー）
/// @param secondHuman 2体目の種類を設定する（true：人、false：ダミー）
/// @return 重なりを解いたらtrue（ダミーどうしは動かさない）
bool VersusCalcPushBodies(VEC3& first, VEC3& second, bool firstHuman, bool secondHuman, float radius, float height);

/// @brief 画面の中の1人分の描画範囲（ピクセル。左上が原点）
struct VERSUS_VIEWPORT
{
	float X; ///< 左端
	float Y; ///< 上端
	float Width; ///< 幅
	float Height; ///< 高さ
};

/// @brief 人数に合わせた、画面の順で index 番目の人の描画範囲を取得する
/// @details 1人は全画面、2人は左右に2分割、3〜4人は4分割（左上・右上・左下・右下の順）。人数・番号が範囲外なら全画面
/// @param humanCount 人の数（1〜4）
/// @param index 画面の順の番号（0〜humanCount-1。3人のときの空きは index 3 で取れる）
VERSUS_VIEWPORT VersusCalcViewport(int humanCount, int index, float screenWidth, float screenHeight);

/// @brief スロットへの機器の割り当てと、最後の攻撃者・KO得点を計算する（DirectXなし）
class VersusCalc
{
public:
	/// @brief 全スロットをダミーにし、攻撃者と得点を消す
	void Reset();
	/// @brief 機器を空いている先頭のスロットに割り当てる（同じ機器は既存スロット、満員・無効な機器は-1）
	/// @param device パッド0〜3、キーボード4、または1人用合成入力
	int AssignDevice(int device);
	/// @brief スロットの機器番号を取得する（ダミー・範囲外は-1）
	int GetDevice(int slot) const;
	/// @brief 人が入っているスロットか取得する
	bool IsHuman(int slot) const;
	/// @brief 自分以外の有効なスロットを相手の候補にできるか取得する
	bool IsOpponent(int self, int target) const;
	/// @brief 命中した攻撃者を記録する（自分への攻撃は記録しない。ダミーも記録し、前の攻撃者を上書きする）
	void RecordAttack(int attacker, int target);
	/// @brief 場外になった体の最後の攻撃者が人なら+1し、その体の攻撃記録を消す
	void RecordOut(int target);
	/// @brief スロットのKO数を取得する（範囲外は0）
	int GetScore(int slot) const;
	/// @brief 最後に攻撃したスロットを取得する（記録なし・範囲外は-1）
	int GetLastAttacker(int slot) const;

private:
	int m_Devices[FIGHTER_COUNT] = { -1, -1, -1, -1 };
	int m_LastAttackers[FIGHTER_COUNT] = { -1, -1, -1, -1 };
	int m_Scores[FIGHTER_COUNT] = {};
};
