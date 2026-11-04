#pragma once

/// @file letterboxcalc.h
/// @brief ゲームの画面（1280×720）を、比率を保ってウィンドウの真ん中に置く計算（黒い帯。DirectX と Windows は使わない）

/// @brief ゲームの画面をウィンドウのどこに何倍で描くか
struct LETTERBOX
{
	float OffsetX; ///< ゲームの画面の左端のウィンドウ上の位置（左右の黒い帯の幅）
	float OffsetY; ///< ゲームの画面の上端のウィンドウ上の位置（上下の黒い帯の高さ）
	float Scale; ///< ゲームの1ピクセルをウィンドウの何ピクセルにするか
};

/// @brief ゲームの画面が収まる最大の倍率を求め、余りを左右か上下に半分ずつ分ける
/// @param windowWidth ウィンドウ（描く先）の幅
/// @param windowHeight ウィンドウ（描く先）の高さ
/// @param gameWidth ゲームの画面の幅
/// @param gameHeight ゲームの画面の高さ
/// @return 置き方（どれかが 0 以下なら、ずらしなし・倍率 1）
LETTERBOX LetterboxCalc(float windowWidth, float windowHeight, float gameWidth, float gameHeight);
