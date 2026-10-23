#pragma once

/// @file hud.h
/// @brief 画面表示（残り秒数・スコア・ダミーの頭の上のダメージ%・時間切れの画面）
/// @details 数字は 7 セグ、英字は 5×7 ドット（render/number2d.h）。置き場所・大きさ・色は hud.cpp の「調整用の値」。
///          時間切れでは画面を半透明の黒でおおい、TIME UP・スコア・PRESS R / OPTIONS を出す

/// @brief 画面表示の初期化（今は何もしない。Number2DInitialize のあとに呼ぶ）
void HudInitialize(void);

/// @brief 画面表示の終了処理（今は何もしない）
void HudFinalize(void);

/// @brief 画面表示の更新（今は何もしない。値は描くときに読む）
void HudUpdate(void);

/// @brief 画面表示を描く（2D 描画の中で、深度テストを切りライトを消してから呼ぶ）
void HudDraw(void);
