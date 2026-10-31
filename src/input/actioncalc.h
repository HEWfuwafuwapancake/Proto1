#pragma once

#include <cstdint>

/// @brief 行動の種類（PLAYER_INPUT のボタンビットの順番も兼ねる）
enum ACTION
{
	ACTION_JUMP, ///< ジャンプ
	ACTION_ARM_LEFT, ///< 左腕を使う
	ACTION_ARM_RIGHT, ///< 右腕を使う
	ACTION_RESTART, ///< やり直し
	ACTION_RUMBLE_TOGGLE, ///< 振動の切り替え
	ACTION_DECIDE, ///< 決定
	ACTION_TARGET_LEFT, ///< 左の敵への切り替え（1フレームのイベント）
	ACTION_TARGET_RIGHT, ///< 右の敵への切り替え（1フレームのイベント）
	ACTION_COUNT ///< 行動の数
};

/// @file actioncalc.h
/// @brief 入力を「行動」に変えるときの計算（キーから移動の向きを作る・複数の機器の入力をまとめる）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 2つの向き（スティック1本分）の入力
struct ACTION_AXIS
{
	float X;	///< 左右（右がプラス）
	float Y;	///< 前後・上下（前・上がプラス）
};

/// @brief 数字キーを押した瞬間の装備の直接選択（DirectX・Windows に依存しない）
struct ACTION_EQUIP_KEY
{
	int Hand; ///< 選ぶ手（左 0・右 1。入力なしは -1）
	int Number; ///< 選ぶ腕の番号（0〜5。入力なしは -1）
};

/// @brief ローカルと通信で共用する1人・1フレーム分の入力（メモリーを直接送らず項目ごとに符号化する）
struct PLAYER_INPUT
{
	int16_t MoveX = 0; ///< 左右の移動（-32767〜32767）
	int16_t MoveY = 0; ///< 前後の移動（-32767〜32767）
	int16_t LookX = 0; ///< カメラの左右角（1単位＝1/4096ラジアン）
	int16_t LookY = 0; ///< カメラの上下角（1単位＝1/4096ラジアン）
	uint16_t Buttons = 0; ///< ACTION の番号のビットが1なら押下。ターゲット切り替えだけはイベント
	ACTION_EQUIP_KEY Equip = { -1, -1 }; ///< このフレームの装備の直接選択（なしは両方-1）
};

/// @brief 移動・カメラの値を通信と同じ整数へ丸める（装備の選択は呼び出し側で設定する）
PLAYER_INPUT ActionCalcPlayerInput(ACTION_AXIS move, ACTION_AXIS look, uint16_t buttons);
/// @brief 丸めた移動入力を小数に戻す
ACTION_AXIS ActionCalcPlayerMove(const PLAYER_INPUT& input);
/// @brief 丸めたカメラ入力をラジアンに戻す
ACTION_AXIS ActionCalcPlayerLook(const PLAYER_INPUT& input);
/// @brief 今と前の入力から押した瞬間を調べる（ターゲット切り替えのイベントは連続フレームでも有効）
bool ActionCalcPlayerTrigger(const PLAYER_INPUT& current, const PLAYER_INPUT& previous, ACTION action);

/// @brief 切り替えモードのキーと数字キーから、装備の直接選択を作る
/// @details 左のモードキーで左手、右のモードキーで右手の選択モードに入る（同じキーをもう一度押すと抜ける。両方押したら右手）。
///          モード中に数字キーを押すとその手を選んでモードを抜ける。モードでないときの数字キーは、Shift なしで左手・ありで右手
/// @param mode 今の選択モード（左 0・右 1・モードなし -1）。このフレームの結果に書き換える
/// @param leftMode 左手のモードキーを押した瞬間か（true：押した、false：押していない）
/// @param rightMode 右手のモードキーを押した瞬間か（true：押した、false：押していない）
/// @param number 押した瞬間の数字キーの腕の番号（0〜5。押していなければ -1）
/// @param shift Shift を押しているか（true：押している、false：押していない）
/// @return 選んだ手と腕の番号。数字キーを押していなければ両方 -1
ACTION_EQUIP_KEY ActionCalcEquipKey(int& mode, bool leftMode, bool rightMode, int number, bool shift);

/// @brief 4つのキーから移動の向きを作る
/// @details 斜めは長さを 1 に直す（斜めだけ速くならないように）。反対のキーを同時に押すと、その向きは 0
/// @param forward 前のキーを押しているか（true：押している、false：押していない）
/// @param back 後ろのキーを押しているか（true：押している、false：押していない）
/// @param left 左のキーを押しているか（true：押している、false：押していない）
/// @param right 右のキーを押しているか（true：押している、false：押していない）
/// @return 向き（X・Y とも -1〜1、長さは 0 か 1）
ACTION_AXIS ActionCalcMoveFromKeys(bool forward, bool back, bool left, bool right);

/// @brief 向きの長さを 1 までに収める（向きは変えない）
/// @param axis 向き
/// @return 長さが 1 より長ければ長さ 1 に縮めた向き、それ以外はそのまま
ACTION_AXIS ActionCalcClampLength(ACTION_AXIS axis);

/// @brief 2つの機器の向きの入力を1つにまとめる（キーボードとパッドなど）
/// @details 長いほう（強く倒しているほう）を使う。足し合わせると、両方入れたときに速くなりすぎるため
/// @param a 1つ目の機器の向き
/// @param b 2つ目の機器の向き
/// @return 長いほうの向き（同じ長さなら a）
ACTION_AXIS ActionCalcCombineAxis(ACTION_AXIS a, ACTION_AXIS b);
