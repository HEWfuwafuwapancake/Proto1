#pragma once

#include "core/dx11.h"

/// @brief 描画と同じ、上下を含むカメラの向きを取得する（長さ1）
/// @return カメラの正面の単位ベクトル（ワールド座標）
XMFLOAT3 CameraGetDirection(void);

/// @file camera.h
/// @brief TPS カメラ（敵へのロックオンと、保持中・敵がいないときの自由カメラ）
/// @details
///   - 開幕は正面の敵。左右の弾き入力で次の敵へ切り替え、角度・注視点をバネで追う
///   - ロックオン中は上下だけ手動で回せる。保持中・敵がいないときは左右も自由に回せる
///   - 自由カメラでは試合中の横移動について回り、自分で回したあとは少し止める
///   - 注視点の左右には遊びがあり、ジャンプ中は最後に立っていた高さを保つ。場外への落下は追う
///   - 浮島にめり込むときはすぐ近づき、離れるときはバネで戻る。復活・やり直しでは速度を0にしてすぐ合わせる
///   - 計算は physics/cameracalc.h

/// @brief 向きを最初に戻して正面の敵を選び、プレイヤーの後ろに置く（PlayerInitialize・DummyInitialize のあとに呼ぶ）
void CameraInitialize(void);

/// @brief カメラの終了処理（今は何もしない）
void CameraFinalize(void);

/// @brief ターゲット・手動入力・横移動から目標を作り、角度・距離・注視点を別々のバネで追う
/// @details 毎フレーム、PlayerUpdate のあとに呼ぶ（動いたあとのプレイヤーを追いかける）。浮島にめり込むときは手前まで近づける
void CameraUpdate(void);

/// @brief ビュー行列とプロジェクション行列をシェーダーに設定する
/// @details 3D描画の前に呼ぶ
void CameraDraw(void);

/// @brief ワールド座標の点が、画面のどこに映るかを取得する（頭の上の % の表示に使う）
/// @param world 点（ワールド座標）
/// @param[out] screen 画面の位置（左上が原点のピクセル。画面の外になることもある）
/// @return 点がカメラの前にあれば true、後ろ（映らない）なら false（そのとき screen は変えない）
bool CameraWorldToScreen(const XMFLOAT3& world, XMFLOAT2* screen);

/// @brief カメラの位置を取得する
/// @return カメラの位置（ワールド座標）
XMFLOAT3 CameraGetPosition(void);

/// @brief カメラの左右の向きを取得する（プレイヤーの移動の向きに使う）
/// @return 左右の向き（ラジアン。0 で +Z 方向、プラスで右回り）
float CameraGetYaw(void);

/// @brief 現在ロックオンしている敵のダミー番号を取得する
/// @return ダミー番号（0〜DUMMY_COUNT-1）。ターゲットなし・敵をつかんでいる間は-1
int CameraGetTarget(void);
