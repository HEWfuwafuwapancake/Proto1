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

/// @brief 片手の腕の基本クラス。左 0・右 1 を1インスタンスずつ受け持つ
class Arm
{
public:
	/// @brief 派生クラスの終了処理も呼んで腕を破棄する
	virtual ~Arm() = default;
	/// @brief カメラ更新後に片手の腕を更新する
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	virtual void Update(bool trigger) = 0;
	/// @brief 片手の腕を描画する
	virtual void Draw() = 0;
	/// @brief 待機に戻す（保持している敵はダメージなしで離す）
	virtual void Reset() = 0;
	/// @brief 待機以外なら true を返す
	virtual bool IsActive() const = 0;
	/// @brief 敵をつかんでいる間だけ true を返す
	virtual bool IsHolding() const = 0;
	/// @brief 受け持つ手（左 0・右 1）を取得する
	int GetHand() const { return m_Hand; }

protected:
	explicit Arm(int hand) : m_Hand(hand) {}

private:
	int m_Hand;
};

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
/// @brief 全種類で共通の肩の位置を取得する（描画・攻撃の始点に使う）
/// @param hand 左 0・右 1
/// @return 肩のワールド座標
VEC3 ArmGetShoulder(int hand);
/// @brief 全種類で共通の、左右へ少し開いた待機方向を取得する
/// @param hand 左 0・右 1
/// @param pitchOffset 共通の上下角度への加算（ラジアン。省略は 0。ひじを曲げる前腕などに使う）
/// @return キャラの向きについてくる長さ 1 の方向（攻撃の狙いには使わない）
VEC3 ArmGetRestDirection(int hand, float pitchOffset = 0.0f);
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
