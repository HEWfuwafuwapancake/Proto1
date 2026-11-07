#pragma once

#include "core/dx11.h"
#include "physics/guardcalc.h"
#include "physics/vec3.h"
#include <memory>

class Fighter;
class FighterManager;
class Camera;
class Stage;

/// @brief 腕の種類（10個ずつが1段。段の中は数字キー 1〜9・0 と同じ順）
enum ARM_TYPE
{
	ARM_TYPE_TENTACLE, ///< 触腕（1段目 1）
	ARM_TYPE_GRAPPLE, ///< グラップリング（1段目 2）
	ARM_TYPE_DRILL, ///< ドリル（1段目 3）
	ARM_TYPE_MAGNET, ///< 磁石（1段目 4）
	ARM_TYPE_SHIELD, ///< デカ盾（1段目 5）
	ARM_TYPE_HASAMI, ///< ハサミ（1段目 6）
	ARM_TYPE_KNUCKLE, ///< ナックル（1段目 7）
	ARM_TYPE_BLADE, ///< ブレード（1段目 8）
	ARM_TYPE_SPEAR, ///< スピア（1段目 9）
	ARM_TYPE_HAMMER, ///< ハンマー（1段目 0）
	ARM_TYPE_WHIP, ///< ウィップ（2段目 1）
	ARM_TYPE_LASER, ///< レーザー（2段目 2）
	ARM_TYPE_SHOTGUN, ///< ショットガン（2段目 3）
	ARM_TYPE_SNIPER, ///< スナイパー（2段目 4）
	ARM_TYPE_WARP, ///< ワープ（2段目 5）
	ARM_TYPE_REPAIR, ///< リペア（2段目 6）
	ARM_TYPE_ROCKET, ///< ロケットパンチ（2段目 7）
	ARM_TYPE_CANNON, ///< キャノン（2段目 8）
	ARM_TYPE_GATLING, ///< ガトリング（2段目 9）
	ARM_TYPE_MISSILE, ///< ミサイル（2段目 0）
	ARM_TYPE_FLAME, ///< 火炎放射（3段目 1）
	ARM_TYPE_FREEZE, ///< フリーズ（3段目 2）
	ARM_TYPE_THUNDER, ///< サンダー（3段目 3）
	ARM_TYPE_SMOKE, ///< スモーク（3段目 4）
	ARM_TYPE_BARRIER, ///< バリア（3段目 5）
	ARM_TYPE_AXE, ///< アックス（3段目 6）
	ARM_TYPE_COUNT ///< 腕の種類の数（装備ではない）
};

/// @brief 腕が使うゲームの部品への参照（所有しない）
struct ARM_LINKS
{
	FighterManager* Fighters; ///< 自分と相手の体・候補・保持・命中を受け持つ管理
	int Slot; ///< 自分のスロット番号（0〜3。相手の候補から除く）
	const Camera* CameraObject; ///< 狙いと投げる向きを読むカメラ
	const Stage* StageObject; ///< 浮島と場外の線を読むステージ
};

/// @brief 片手の腕の基本クラス。左 0・右 1 を1インスタンスずつ受け持つ
class Arm
{
public:
	/// @brief すべての腕のモデルに掛ける大きさの倍率（調整用の値。キャラモデルの細い体に合わせて小さくする）
	static constexpr float MODEL_SIZE = 0.5f;
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
	/// @brief 歩く速さの倍率を取得する（基本は 1.0、派生クラスで変更できる）
	virtual float GetMoveScale() const { return 1.0f; }
	/// @brief 今のガードを取得する（基本はガードなし。デカ盾・バリア・ワープで変更する）
	virtual GUARD GetGuard() const { return {}; }
	/// @brief 受け持つ手（左 0・右 1）を取得する
	int GetHand() const { return m_Hand; }

protected:
	/// @brief 手とゲームの部品への参照を保持する（モデルは派生クラスの生成時に読む）
	Arm(int hand, const ARM_LINKS& links) : m_Hand(hand), m_Links(links) {}

	/// @brief プレイヤーへの参照を取得する
	Fighter& GetPlayer() const;
	/// @brief 全員の体の管理への参照を取得する
	FighterManager& GetFighters() const { return *m_Links.Fighters; }
	/// @brief 自分のスロット番号を取得する
	int GetSlot() const { return m_Links.Slot; }
	/// @brief カメラへの参照を取得する
	const Camera& GetCamera() const { return *m_Links.CameraObject; }
	/// @brief ステージへの参照を取得する
	const Stage& GetStage() const { return *m_Links.StageObject; }
	/// @brief 当たり判定に使う肩のワールド座標を取得する（共通の取付位置に GetMountOffset のずらしを足す）
	VEC3 GetShoulder() const;
	/// @brief 描画するキャラの肩のワールド座標を取得する（腕の骨があるキャラはアニメの肩の骨に付ける）
	VEC3 GetDrawShoulder() const;
	/// @brief 描画するキャラの腕のモデルの大きさの倍率を取得する（メカを 1 とする）
	float GetDrawScale() const;
	/// @brief この腕だけの取付位置のずらしを取得する（腕のモデルの肩の部分を体の取付口に合わせるため）
	/// @return X：外向き・Y：上向き・Z：前向きのずらし（m）。基本はずらしなし
	virtual VEC3 GetMountOffset() const { return {}; }
	/// @brief キャラの向きについてくる、少し外へ開いた待機方向を取得する
	/// @param pitchOffset 共通の上下角度への加算（ラジアン）
	VEC3 GetRestDirection(float pitchOffset = 0.0f) const;
	/// @brief キャラの向きの水平の単位方向を取得する
	VEC3 GetForward() const;
	/// @brief 肩からの狙いの単位方向を取得する（ロックオン中は相手の体の中心へ、いなければ画面の正面へ水平）
	/// @param keepHeight 上下の扱いを設定する（true：相手への上下も含める、false：水平だけ）
	VEC3 GetAim(bool keepHeight) const;
	/// @brief ロックオン中の相手のスロット番号を取得する（なし・つかめない相手は -1）
	int GetAimTarget() const;
	/// @brief モデルのない腕の仮の形として、肩から円柱を描く
	/// @param direction 腕の単位方向
	/// @param length 腕の長さ
	/// @param color 色（RGBA）
	/// @return 手の位置（円柱の終わり）
	VEC3 DrawStubArm(const VEC3& direction, float length, const XMFLOAT4& color) const;
	/// @brief 爆心から半径の中の相手を全部、外へ吹っ飛ばす（自分には当たらない）
	/// @param center 爆心
	/// @param radius 爆発の半径
	/// @param damage 足すダメージ%
	/// @param power 吹っ飛ばす強さ
	/// @param up 外への水平の単位方向に足す上向きの量
	/// @return 当てた相手の数
	int Explode(const VEC3& center, float radius, float damage, float power, float up) const;
	/// @brief 色の選択を設定する（true：効いている色、false：左右の基本色）
	/// @return 選んだ色（RGBA）
	XMFLOAT4 GetColor(bool active) const;
	/// @brief モデルの -Y を始点から終点へ向け、長さ・太さ・色・ひねりを指定して描く
	/// @param down モデルの -Z（吸盤側）をなるべく向ける方向（腕と平行なら補助の軸で決める）
	void DrawPiece(int model, VEC3 start, VEC3 end, float sourceLength, float thickness, const XMFLOAT4& color, float roll = 0.0f,
		const VEC3& down = { 0.0f, -1.0f, 0.0f }) const;
	/// @brief 片手用の OBJ を読む（右手だけ X を鏡写しにする。失敗は -1）
	int LoadModel(const char* path) const;
	/// @brief この腕を使う人のパッドを振動させる（キーボードの人には出力しない）
	void Rumble(float smallMotor, float largeMotor, float seconds) const;

private:
	VEC3 GetShoulderAt(const VEC3& offset) const;
	int m_Hand;
	ARM_LINKS m_Links;
};

/// @brief 左右の装備と全種類の腕の寿命・更新・描画を管理する
class ArmManager
{
public:
	/// @brief 自分のスロットと体の管理を受け取り、左右とも触腕を装備する（体の初期化後）
	void Initialize(const Stage& stage, FighterManager& fighters, int slot, const Camera& camera);
	/// @brief 腕の処理を終了し、保持している敵を離す
	void Finalize(void);
	/// @brief 左右の腕を待機に戻す（装備はそのまま。時間切れ・やり直し用）
	void Reset(void);
	/// @brief 両手を触腕に戻し、十字キーの手の選択も左手に戻す（オンラインを始めるとき。相手と装備をそろえる）
	void ResetEquip(void);
	/// @brief 装備を切り替え、試合中だけ左手・右手の順に更新する（カメラとキャラの向きの更新後）
	/// @param playing 腕の更新を設定する（true：試合中として更新、false：装備切り替えだけ）
	void Update(bool playing);
	/// @brief 左手・右手の順に装備している腕を描画する
	void Draw(void) const;
	/// @brief どちらかの腕が効いているか（待機以外なら true）
	bool IsActive(void) const;
	/// @brief 今使っている手を取得する（左 0・右 1・なし -1。両手なら後から使った手）
	int GetActiveHand() const;
	/// @brief どちらかの腕が敵をつかんでいるか（保持中なら true）
	bool IsHolding(void) const;
	/// @brief 左右の装備している腕のガードの強いほうを取得する
	GUARD GetGuard() const;
	/// @brief 左右の装備している腕の、歩く速さの倍率の小さいほうを取得する
	float GetMoveScale() const;
	/// @brief 片手の装備を取得する
	/// @param hand 左 0・右 1
	/// @return 装備している種類。手が範囲外なら ARM_TYPE_COUNT
	ARM_TYPE GetType(int hand) const;

	/// @brief 十字キーで選んでいる手を取得する（左 0・右 1）
	int GetSelectedHand() const { return m_SelectedHand; }
	/// @brief 片手の最後の装備変更の向きを取得する（左 0・右 1、次・直接選択 +1、前 -1）
	int GetChangeDirection(int hand) const;

private:
	void ChangeType(int hand, int next, int direction = 1);
	ARM_LINKS m_Links = {};
	int m_SelectedHand = 0;
	int m_LastActiveHand = -1;
	int m_ChangeDirections[2] = { 1, 1 };
	ARM_TYPE m_Types[2] = { ARM_TYPE_TENTACLE, ARM_TYPE_TENTACLE };
	// 左右それぞれ全種類を起動時に作る。装備切り替えでは作り直さない。
	std::unique_ptr<Arm> m_Arms[2][ARM_TYPE_COUNT];
};

/// @brief 腕の日本語名を取得する（範囲外は「不明」）
const char* ArmGetName(ARM_TYPE type);
