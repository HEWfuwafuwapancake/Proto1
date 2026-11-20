#pragma once

/// @file menucalc.h
/// @brief メニューの動き（背景のカメラの回り方・選んだときの弾み）の計算（DirectX を使わない）

/// @brief 3D の点
struct MENU_POINT
{
	float X; ///< 横（ワールド座標）
	float Y; ///< 高さ（ワールド座標）
	float Z; ///< 奥行き（ワールド座標）
};

/// @brief 背景のカメラの位置を求める（中心のまわりを、斜め上からゆっくり1周する）
/// @details 0 秒で中心の +X 側、1周の 1/4 で +Z 側にいる
/// @param seconds メニューを開いてからの秒数
/// @param center 見る中心
/// @param radius 中心から水平にはなれる距離
/// @param height 中心からの高さ
/// @param periodSeconds 1周にかける秒数
/// @return カメラの位置
MENU_POINT MenuCalcOrbitEye(float seconds, const MENU_POINT& center, float radius, float height, float periodSeconds);

/// @brief 少し行き過ぎてから戻る動き（選んだボタンが弾むのに使う）
/// @param progress 進み具合（0：始め、1：終わり。範囲の外は 0〜1 にそろえる）
/// @return 0 から始まり、途中で 1 を少し超えて、1 で終わる値
float MenuCalcEaseOutBack(float progress);
