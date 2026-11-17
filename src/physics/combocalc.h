#pragma once

#include "physics/stagecalc.h"
#include "physics/vec3.h"

/// @file combocalc.h
/// @brief 単押しの段階攻撃（1段目 → 2段目 → …）の段の進め方と、刀の通り抜けの行き先の計算（DirectX と Windows は使わない）

/// @brief 単押しの段階攻撃で、今の段と次の段へつなげられる残りの時間を管理する
/// @details 段を出し終わる少し前（休みに入ったとき）に Open で受付を開き、受付の間に単押しすると次の段へ進む。
///          受付が閉じたあとの単押しは1段目からやり直す。最後の段のあとは1段目に戻る（ループする腕も同じ式）。
///          最後の段を出し終えたら StartCooldown で、その腕の攻撃をしばらく止める
class ComboCalc
{
public:
	/// @brief 段を持たない状態（次の単押しは1段目）に戻す
	void Reset();
	/// @brief 単押しで出す段を決めて、その段を今の段にする（受付は閉じる）
	/// @param count 段の数（1 以上。2 ならハンマーのように 1段目・2段目をくり返す）
	/// @return 出す段（0 から。受付の間なら前の段の次、受付の外なら 0）
	int Start(int count);
	/// @brief 次の段への受付を開く
	/// @param seconds 受付を開いておく秒数（0 以下なら開かない＝次は1段目）
	void Open(float seconds);
	/// @brief 最後の段のあとの、攻撃できない時間を始める（段と受付は消す）
	/// @param seconds 攻撃できない秒数（0 以下なら止めない）
	void StartCooldown(float seconds);
	/// @brief 受付と攻撃できない時間の残りを進める（受付が閉じたら段を持たない状態に戻す）
	/// @param dt 進める秒数（0 以下なら何もしない）
	void Step(float dt);
	/// @brief 今の段を取得する（段を持たないときは -1）
	int GetStep() const { return m_Step; }
	/// @brief 次の段への受付が開いているか取得する（true：開いている、false：閉じている）
	bool IsOpen() const { return m_Window > 0.0f; }
	/// @brief 最後の段のあとで攻撃できない間か取得する（true：攻撃できない、false：攻撃できる）
	bool IsCoolingDown() const { return m_Cooldown > 0.0f; }

private:
	int m_Step = -1; ///< 今の段（段を持たないときは -1）
	float m_Window = 0.0f; ///< 次の段への受付の残りの秒数
	float m_Cooldown = 0.0f; ///< 攻撃できない残りの秒数
};

/// @brief 刀の3段目で、相手を通り抜けて着く位置を求める
/// @details 浮島の上から出したときは、浮島の外へ出ないところまで縮める（場外へ自分から落ちない）。
///          浮島の外（空中）から出したときは縮めない
/// @param stage ステージ
/// @param start 足元の位置
/// @param direction 進む向き（水平の成分だけを使う。長さはいくつでもよい。水平の成分がなければ start を返す）
/// @param distance 進みたい距離
/// @param margin 浮島のふちから内側に残す距離
/// @return 着く足元の位置（高さは start と同じ）
VEC3 ComboCalcPierceEnd(const STAGE& stage, const VEC3& start, const VEC3& direction, float distance, float margin);
