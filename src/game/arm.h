#pragma once

#include "core/dx11.h"
#include "physics/vec3.h"
#include <memory>

class Player;
class DummyManager;
class Camera;
class Stage;

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

/// @brief 腕が使うゲームの部品への参照（所有しない）
struct ARM_LINKS
{
	Player* PlayerObject; ///< プレイヤー（引き寄せによる位置・状態の更新にも使う）
	DummyManager* Dummies; ///< 敵の候補・保持・命中を受け持つダミー管理
	const Camera* CameraObject; ///< 狙いと投げる向きを読むカメラ
	const Stage* StageObject; ///< 浮島と場外の線を読むステージ
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
	/// @brief 手とゲームの部品への参照を保持する（モデルは派生クラスの生成時に読む）
	Arm(int hand, const ARM_LINKS& links) : m_Hand(hand), m_Links(links) {}

	/// @brief プレイヤーへの参照を取得する
	Player& GetPlayer() const { return *m_Links.PlayerObject; }
	/// @brief ダミー管理への参照を取得する
	DummyManager& GetDummies() const { return *m_Links.Dummies; }
	/// @brief カメラへの参照を取得する
	const Camera& GetCamera() const { return *m_Links.CameraObject; }
	/// @brief ステージへの参照を取得する
	const Stage& GetStage() const { return *m_Links.StageObject; }
	/// @brief 肩のワールド座標を取得する
	VEC3 GetShoulder() const;
	/// @brief キャラの向きについてくる、少し外へ開いた待機方向を取得する
	/// @param pitchOffset 共通の上下角度への加算（ラジアン）
	VEC3 GetRestDirection(float pitchOffset = 0.0f) const;
	/// @brief 色の選択を設定する（true：効いている色、false：左右の基本色）
	/// @return 選んだ色（RGBA）
	XMFLOAT4 GetColor(bool active) const;
	/// @brief モデルの -Y を始点から終点へ向け、長さ・太さ・色・ひねりを指定して描く
	void DrawPiece(int model, VEC3 start, VEC3 end, float sourceLength, float thickness, const XMFLOAT4& color, float roll = 0.0f) const;
	/// @brief 片手用の OBJ を読む（右手だけ X を鏡写しにする。失敗は -1）
	int LoadModel(const char* path) const;

private:
	int m_Hand;
	ARM_LINKS m_Links;
};

/// @brief 左右の装備と全種類の腕の寿命・更新・描画を管理する
class ArmManager
{
public:
	/// @brief 腕の処理を初期化し、左右とも触腕を装備する（ダミー初期化後）
	void Initialize(const Stage& stage, Player& player, DummyManager& dummies, const Camera& camera);
	/// @brief 腕の処理を終了し、保持している敵を離す
	void Finalize(void);
	/// @brief 左右の腕を待機に戻す（装備はそのまま。時間切れ・やり直し用）
	void Reset(void);
	/// @brief 装備を切り替え、試合中だけ左手・右手の順に更新する（カメラとキャラの向きの更新後）
	/// @param playing 腕の更新を設定する（true：試合中として更新、false：装備切り替えだけ）
	void Update(bool playing);
	/// @brief 左手・右手の順に装備している腕を描画する
	void Draw(void) const;
	/// @brief どちらかの腕が効いているか（待機以外なら true）
	bool IsActive(void) const;
	/// @brief どちらかの腕が敵をつかんでいるか（保持中なら true）
	bool IsHolding(void) const;
	/// @brief 片手の装備を取得する
	/// @param hand 左 0・右 1
	/// @return 装備している種類。手が範囲外なら ARM_TYPE_COUNT
	ARM_TYPE GetType(int hand) const;

private:
	void ChangeType(int hand, int next);
	ARM_LINKS m_Links = {};
	ARM_TYPE m_Types[2] = { ARM_TYPE_TENTACLE, ARM_TYPE_TENTACLE };
	// 左右それぞれ全種類を起動時に作る。装備切り替えでは作り直さない。
	std::unique_ptr<Arm> m_Arms[2][ARM_TYPE_COUNT];
};

/// @brief タイトルバー用の日本語名を取得する（範囲外は「不明」）
const char* ArmGetName(ARM_TYPE type);
