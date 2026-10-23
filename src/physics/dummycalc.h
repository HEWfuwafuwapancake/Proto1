#pragma once

#include "core/timer.h"
#include "physics/collision.h"

/// @file dummycalc.h
/// @brief ダミーの敵の計算（場外に落ちたら待って復活する流れ・プレイヤーに押される）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief ダミーの状態
enum DUMMY_STATE
{
	DUMMY_STATE_ACTIVE,		///< 浮島の上にいる（見えている・ぶつかる）
	DUMMY_STATE_WAITING,	///< 場外に落ちて、復活を待っている（見えない・ぶつからない）
};

/// @brief DummyCalcUpdateLife で、このフレームに起きたこと
enum DUMMY_EVENT
{
	DUMMY_EVENT_NONE,		///< 何も起きていない
	DUMMY_EVENT_OUT,		///< 場外に落ちた（このフレームから復活を待つ）
	DUMMY_EVENT_RESPAWN,	///< 復活した（呼んだ側で最初の場所に戻す）
};

/// @brief ダミー1体の「落ちて復活する」流れ
struct DUMMY_LIFE
{
	DUMMY_STATE State;		///< 今の状態
	TIMER RespawnTimer;		///< 復活までの残り時間（DUMMY_STATE_WAITING の間だけ使う）
	int OutCount;			///< 場外に落ちた回数（No.14 のスコアに使う）
};

/// @brief 流れを最初の状態にする（浮島の上にいる・落ちた回数 0）
/// @param[out] life ダミー1体の流れ
void DummyCalcResetLife(DUMMY_LIFE& life);

/// @brief 流れを1フレーム分進める
/// @details 浮島の上にいて場外に落ちたら、落ちた回数を増やして復活を待ち始める。
///          待っている間は時間を数え、数え終わったら浮島の上にいる状態に戻る
/// @param[in,out] life ダミー1体の流れ
/// @param isOut 場外の下の線より下にいるか（true：いる、false：いない。待っている間は見ない）
/// @param respawnSeconds 落ちてから復活するまでの秒数
/// @param dt 1フレームの秒数
/// @return このフレームに起きたこと
DUMMY_EVENT DummyCalcUpdateLife(DUMMY_LIFE& life, bool isOut, float respawnSeconds, float dt);

/// @brief 足元の位置から、縦向きのカプセル（プレイヤー・ダミーの体の形）を作る
/// @param feet 足元の位置
/// @param radius 半径
/// @param height 高さ（半球も含めた全体の高さ）
/// @return カプセル（線分は足元から radius 上 〜 頭から radius 下）
CAPSULE DummyCalcBodyCapsule(const VEC3& feet, float radius, float height);

/// @brief プレイヤーとダミーが重なっていたら、プレイヤーがダミーを押す
/// @details 横から重なったときは、ダミーだけを横に押し出す（プレイヤーは押し戻されない）。
///          真上から重なったとき（ダミーの頭に乗ったときなど、押す向きがほとんど上下）は、プレイヤーを押し出す
/// @param[in,out] playerFeet プレイヤーの足元の位置
/// @param[in,out] dummyFeet ダミーの足元の位置
/// @param radius 体の半径（プレイヤーとダミーで同じ）
/// @param height 体の高さ（プレイヤーとダミーで同じ）
/// @return 重なっていて押し出したら true
bool DummyCalcPushByPlayer(VEC3& playerFeet, VEC3& dummyFeet, float radius, float height);
