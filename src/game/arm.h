#pragma once

#include "core/dx11.h"
#include "physics/vec3.h"

/// @file arm.h
/// @brief 左右の装備と腕ごとの処理の振り分け・腕に共通の描画部品

/// @brief 腕の種類（数字キー 1〜6 と同じ順）
enum ARM_TYPE
{
	ARM_TYPE_TENTACLE, ///< 触腕
	ARM_TYPE_GRAPPLE, ///< グラップリング
	ARM_TYPE_DRILL, ///< ドリル
	ARM_TYPE_MAGNET, ///< 磁石
	ARM_TYPE_SHIELD, ///< デカ盾
	ARM_TYPE_HASAMI, ///< ハサミ
	ARM_TYPE_COUNT ///< 腕の種類の数（装備ではない）
};

/// @brief 腕1種類の処理。手は左 0・右 1。登録する腕は全メンバーを設定する
struct ARM_FUNCTIONS
{
	void (*Initialize)(void); ///< 起動時に1回呼ぶ（左右の状態・モデルの初期化）
	void (*Finalize)(void); ///< 終了時に1回呼ぶ（保持している敵も離す）
	void (*ResetHand)(int hand); ///< 片手を待機に戻す（敵はダメージなしで離す）
	void (*UpdateHand)(int hand, bool trigger); ///< 片手を更新（trigger は true：押した瞬間、false：それ以外）
	void (*DrawHand)(int hand); ///< 片手を描画する
	bool (*IsActiveHand)(int hand); ///< 片手が効いているか（待機以外なら true）
	bool (*IsHoldingHand)(int hand); ///< 片手が敵をつかんでいるか（保持中なら true）
};

// 各腕の .cpp が定義する登録用の表。未実装の間は参照せず、arm.cpp の登録を nullptr にする。
/// @brief グラップリングの処理の表（No.36 で定義する）
extern const ARM_FUNCTIONS ARM_GRAPPLE_FUNCTIONS;
/// @brief ドリルの処理の表（No.37 で定義する）
extern const ARM_FUNCTIONS ARM_DRILL_FUNCTIONS;
/// @brief 磁石の処理の表（No.38 で定義する）
extern const ARM_FUNCTIONS ARM_MAGNET_FUNCTIONS;
/// @brief デカ盾の処理の表（No.39 で定義する）
extern const ARM_FUNCTIONS ARM_SHIELD_FUNCTIONS;
/// @brief ハサミの処理の表（No.40 で定義する）
extern const ARM_FUNCTIONS ARM_HASAMI_FUNCTIONS;

/// @brief 腕の処理を初期化し、左右とも触腕を装備する（ダミー初期化後）
void ArmInitialize(void);
/// @brief 腕の処理を終了し、保持している敵を離す
void ArmFinalize(void);
/// @brief 左右の腕を待機に戻す（装備はそのまま。時間切れ・やり直し用）
void ArmReset(void);
/// @brief 装備を切り替え、試合中だけ左手・右手の順に更新する（カメラとキャラの向きの更新後）
/// @param playing 腕の更新を設定する（true：試合中として更新、false：装備切り替えだけ）
void ArmUpdate(bool playing);
/// @brief 左手・右手の順に装備している腕を描画する
void ArmDraw(void);
/// @brief どちらかの腕が効いているか（待機以外なら true）
bool ArmIsActive(void);
/// @brief どちらかの腕が敵をつかんでいるか（保持中なら true）
bool ArmIsHolding(void);
/// @brief 片手の装備を取得する
/// @param hand 左 0・右 1
/// @return 装備している種類。手が範囲外なら ARM_TYPE_COUNT
ARM_TYPE ArmGetType(int hand);
/// @brief タイトルバー用の日本語名（マルチバイト文字列）を取得する
/// @param type 腕の種類
/// @return 日本語名。種類が範囲外なら「不明」
const char* ArmGetName(ARM_TYPE type);
/// @brief キャラの向きに合わせた肩の位置を取得する
/// @param hand 左 0・右 1
/// @return 肩のワールド座標
VEC3 ArmGetShoulder(int hand);
/// @brief 左右の基本色または効いている間の色を取得する
/// @param hand 左 0・右 1
/// @param active 色の切り替えを設定する（true：効いている色、false：左右の基本色。触腕は保持中だけ true）
/// @return 色（RGBA）
XMFLOAT4 ArmGetColor(int hand, bool active);
/// @brief モデルの -Y を始点から終点へ向けて描く（ひねり 0 なら X は横、-Z はなるべく下）
/// @param model モデル番号
/// @param start 始点のワールド座標
/// @param end 終点のワールド座標
/// @param sourceLength 元のモデルの長さ（cm。0 より大きい値）
/// @param thickness 横の太さの倍率（cm からゲーム単位への倍率）
/// @param color 色（RGBA）
/// @param roll 腕の長さ方向のひねり（ラジアン。省略すると 0）
void ArmDrawPiece(int model, VEC3 start, VEC3 end, float sourceLength, float thickness, const XMFLOAT4& color, float roll = 0.0f);
/// @brief 片手用の OBJ を読む（右手だけ X を鏡写しにする）
/// @param path 作業フォルダからの相対パス
/// @param hand 左 0・右 1
/// @return モデル番号。読み込み失敗は -1
int ArmLoadModel(const char* path, int hand);
