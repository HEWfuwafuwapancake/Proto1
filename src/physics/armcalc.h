#pragma once

/// @file armcalc.h
/// @brief 使える腕の巡回・直接選択（DirectX と Windows を使わない）

/// @brief 指定した向きへ次の使える腕を選ぶ（端は反対の端へ回る）
/// @param current 今の腕の番号（0〜count-1）
/// @param direction 次は +1、前は -1
/// @param available 腕ごとの使用可否（true：使える、false：使えない）の並び
/// @param count 腕の数
/// @return 次の使える番号。今の1つしか使えなければ current。不正な引数・使える腕なしは -1
int ArmCalcCycle(int current, int direction, const bool* available, int count);

/// @brief 指定した番号の腕が使えるか調べる
/// @param number 選びたい腕の番号
/// @param available 腕ごとの使用可否（true：使える、false：使えない）の並び
/// @param count 腕の数
/// @return 使えるなら number。使えない腕・範囲外・不正な引数は -1
int ArmCalcSelect(int number, const bool* available, int count);
