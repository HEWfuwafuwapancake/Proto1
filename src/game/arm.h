#pragma once

#include "core/dx11.h"
#include "physics/armpresscalc.h"
#include "physics/armusecalc.h"
#include "physics/parrycalc.h"
#include "physics/guardcalc.h"
#include "physics/scenecalc.h"
#include "physics/tuningcalc.h"
#include "render/animcalc.h"
#include "physics/vec3.h"
#include <memory>

class Fighter;
class FighterManager;
class ArmManager;
class Camera;
class Stage;

/// @brief 腕の種類（1段目の数字キー 1〜6 の順）
enum ARM_TYPE
{
	ARM_TYPE_TENTACLE, ///< 触手（1段目 1）
	ARM_TYPE_HAMMER, ///< ハンマー（1段目 2）
	ARM_TYPE_KNUCKLE, ///< ナックル（1段目 3）
	ARM_TYPE_C4, ///< C4（1段目 4）
	ARM_TYPE_GUNLANCE, ///< ガンランス（1段目 5）
	ARM_TYPE_KATANA, ///< 刀（1段目 6）
	ARM_TYPE_NONE, ///< 素手（装備なし）
	ARM_TYPE_COUNT ///< 腕の種類の数（装備ではない）
};

/// @brief 腕が使うゲームの部品への参照（所有しない）
struct ARM_LINKS
{
	FighterManager* Fighters; ///< 自分と相手の体・候補・保持・命中を受け持つ管理
	int Slot; ///< 自分のスロット番号（0〜3。相手の候補から除く）
	const Camera* CameraObject; ///< 狙いと投げる向きを読むカメラ
	const Stage* StageObject; ///< 浮島と場外の線を読むステージ
	const ArmManager* Arms; ///< 片手の残り耐久を読む腕の管理
};

/// @brief 腕が今の動作で体に見せる攻撃クリップ
struct ARM_BODY_CLIP
{
	bool Active = false; ///< 攻撃クリップを使うかを設定する（true：使う、false：使わない）
	ATTACK_CLIP Kind = ATTACK_CLIP::KatanaIai; ///< 攻撃の種類
	float Time = 0.0f; ///< クリップの再生位置の秒
	bool FullBody = false; ///< 重ねる範囲を設定する（true：全身、false：上半身だけ）
};

/// @brief ガード結果の文字表示に使う描画専用の情報
struct GUARD_FEEDBACK
{
	bool Active = false; ///< 結果を表示中ならtrue
	bool Parry = false; ///< 結果を設定する（true：パリィ、false：通常のガード）
	float Elapsed = 0.0f; ///< 防いだ瞬間からの経過秒数
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
	/// @param input 単押し・長押し開始・押下中の状態
	virtual void Update(const ARM_INPUT& input) = 0;
	/// @brief 片手の腕を描画する
	virtual void Draw() = 0;
	/// @brief 待機に戻す（保持している敵はダメージなしで離す）
	virtual void Reset() = 0;
	/// @brief 吹っ飛びやしびれで動作を中断する（基本は待機に戻す）
	virtual void Interrupt() { Reset(); }
	/// @brief 待機以外なら true を返す
	virtual bool IsActive() const = 0;
	/// @brief 動作後も弾や遅れて効く攻撃が残っていれば true を返す（基本は false）
	virtual bool IsBusy() const { return false; }
	/// @brief 敵をつかんでいる間だけ true を返す
	virtual bool IsHolding() const = 0;
	/// @brief 単押しをすぐ受け付けられるなら true を返す（false の間の単押しは少しの間覚えておく。基本は待機中だけ）
	virtual bool CanTakeTap() const { return !IsActive(); }
	/// @brief 長押しをすぐ始められるなら true を返す（false の間は押し続けていれば待つ。基本は待機中だけ）
	virtual bool CanTakeHold() const { return !IsActive(); }
	/// @brief 歩く速さの倍率を取得する（基本は 1.0、派生クラスで変更できる）
	virtual float GetMoveScale() const { return 1.0f; }
	/// @brief 突進のワールド速度を取得する（水平のみ。突進していなければゼロ）
	virtual VEC3 GetRushVelocity() const { return {}; }
	/// @brief 今のガードを取得する（基本はガードなし）
	virtual GUARD GetGuard() const { return {}; }
	/// @brief 今の動作で体に見せる攻撃クリップを取得する（基本は使わない）
	virtual ARM_BODY_CLIP GetBodyClip() const { return {}; }
	/// @brief 受け持つ手（左 0・右 1）を取得する
	int GetHand() const { return m_Hand; }

protected:
	/// @brief 手とゲームの部品への参照を保持する（モデルは派生クラスの生成時に読む）
	Arm(int hand, const ARM_LINKS& links) : m_Hand(hand), m_Links(links) {}

	/// @brief 耐久の減る量に試合の倍率を適用し、このフレームの消費に加える
	void ConsumeDurability(int cost);
	/// @brief 残り耐久を取得する（DEBUG の無制限は -1）
	int GetDurability() const;
	/// @brief 今の試合に適用済みの設定を取得する
	const MATCH_TUNING& GetTuning() const;
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
	/// @brief 攻撃クリップを再生している間、腕と武器の向きをアニメの骨の向きへ寄せる（描画だけ）
	/// @param arm 腕の単位方向（コードで決めた向きを渡し、結果を受け取る）
	/// @param weapon 武器の単位方向（コードで決めた向きを渡し、結果を受け取る）
	void BlendClipPose(VEC3& arm, VEC3& weapon) const;
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
	/// @param affected 効果が入った相手の数を書く先（不要なら nullptr）
	/// @return 範囲に入った相手の数
	int Explode(const VEC3& center, float radius, float damage, float power, float up, int* affected = nullptr) const;
	/// @brief 色の選択を設定する（true：効いている色、false：左右の基本色）
	/// @return 選んだ色（RGBA）
	XMFLOAT4 GetColor(bool active) const;
	/// @brief モデルの -Y を始点から終点へ向け、長さ・太さ・色・ひねりを指定して描く
	/// @param down モデルの -Z（吸盤側）をなるべく向ける方向（腕と平行なら補助の軸で決める）
	void DrawPiece(int model, VEC3 start, VEC3 end, float sourceLength, float thickness, const XMFLOAT4& color, float roll = 0.0f,
		const VEC3& down = { 0.0f, -1.0f, 0.0f }) const;
	/// @brief モデルの -Y を指定の向きへ向け、元の長さのまま描いて、先の位置を返す（読み込み失敗なら円柱で代わりに描く）
	/// @param model モデルの番号（失敗は -1）
	/// @param start つなぐ所
	/// @param direction 向ける単位方向
	/// @param sourceLength モデルの -Y の長さ（cm）
	/// @param scale cm からゲーム単位への倍率
	/// @param color 色（RGBA）
	/// @param roll 長さ方向のひねり（ラジアン）
	/// @return モデルの先の位置
	VEC3 DrawAlong(int model, const VEC3& start, const VEC3& direction, float sourceLength, float scale,
		const XMFLOAT4& color, float roll = 0.0f) const;
	/// @brief 水平の円の上に小さな球を並べて、当たる範囲を見せる
	/// @param center 円の中心
	/// @param radius 円の半径
	/// @param color 色（RGBA）
	void DrawRangeRing(const VEC3& center, float radius, const XMFLOAT4& color) const;
	/// @brief 片手用の OBJ を読む（右手だけ X を鏡写しにする。失敗は -1）
	int LoadModel(const char* path) const;
	/// @brief この腕を使う人のパッドを振動させる（キーボードの人には出力しない）
	void Rumble(float smallMotor, float largeMotor, float seconds) const;

private:
	friend class ArmManager;
	VEC3 GetShoulderAt(const VEC3& offset) const;
	int m_Hand;
	ARM_LINKS m_Links;
	int m_Used = 0;
};

/// @brief 左右の装備と全種類の腕の寿命・更新・描画を管理する
class ArmManager
{
public:
	/// @brief 自分のスロットと体の管理を受け取り、左右とも触手を装備する（体の初期化後）
	void Initialize(const Stage& stage, FighterManager& fighters, int slot, const Camera& camera);
	/// @brief 腕の処理を終了し、保持している敵を離す
	void Finalize(void);
	/// @brief 左右の腕を待機に戻す（装備はそのまま。時間切れ・やり直し用）
	void Reset(void);
	/// @brief 試合開始や復活に備えて腕を戻す（GAME は素手、DEBUG は素手だけ触手にする）
	void ResetForMatch(GAME_RULE rule, const MATCH_TUNING& tuning);
	/// @brief 今の試合に適用済みの設定を取得する
	const MATCH_TUNING& GetTuning() const { return m_Tuning; }
	/// @brief 両手を触手に戻し、十字キーの手の選択も左手に戻す（オンラインを始めるとき。相手と装備をそろえる）
	void ResetEquip(void);
	/// @brief 拾った腕を空いている左手、次に右手へ装備し、耐久100を入れる（装備できたら true）
	bool TryEquipPickup(ARM_TYPE type);
	/// @brief DEBUG では装備を切り替え、試合中は左手・右手の順に更新する（カメラとキャラの向きの更新後）
	/// @param playing 腕の更新を設定する（true：試合中として更新、false：装備切り替えだけ）
	void Update(bool playing);
	/// @brief 左手・右手の順に装備している腕を描画する
	void Draw(void) const;
	/// @brief どちらかの腕が効いているか（待機以外なら true）
	bool IsActive(void) const;
	/// @brief 今使っている手を取得する（左 0・右 1・なし -1。両手なら後から使った手）
	int GetActiveHand() const;
	/// @brief 片手の装備が体に見せる攻撃クリップを取得する
	/// @param hand 左 0・右 1
	/// @return 攻撃クリップの指定（手が範囲外なら使わない指定）
	ARM_BODY_CLIP GetBodyClip(int hand) const;
	/// @brief どちらかの腕が敵をつかんでいるか（保持中なら true）
	bool IsHolding(void) const;
	/// @brief 左右の装備している腕のガードの強いほうを取得する
	GUARD GetGuard() const;
	/// @brief 左右の装備している腕の、歩く速さの倍率の小さいほうを取得する
	float GetMoveScale() const;
	/// @brief 左右の装備の突進速度を取得する（両手なら後から使った手を優先）
	VEC3 GetRushVelocity() const;
	/// @brief 片手の装備を取得する
	/// @param hand 左 0・右 1
	/// @return 装備している種類。手が範囲外なら ARM_TYPE_COUNT
	ARM_TYPE GetType(int hand) const;
	/// @brief 片手の残り耐久を取得する（DEBUG は -1、範囲外は 0）
	int GetDurability(int hand) const;

	/// @brief 最後に防いだ結果と経過時間を取得する（描画専用、通信のハッシュには含めない）
	GUARD_FEEDBACK GetGuardFeedback() const;

	/// @brief 十字キーで選んでいる手を取得する（左 0・右 1）
	int GetSelectedHand() const { return m_SelectedHand; }
	/// @brief 片手の最後の装備変更の向きを取得する（左 0・右 1、次・直接選択 +1、前 -1）
	int GetChangeDirection(int hand) const;

private:
	void ChangeType(int hand, int next, int direction = 1);
	bool HasUnlimitedUses() const;
	void UpdateEquip(const Fighter& player);
	void BreakExhaustedArm(int hand);
	void UpdateGuards(const Fighter& player, float dt, bool interrupted);
	ARM_LINKS m_Links = {};
	GAME_RULE m_Rule = GAME_RULE_DEBUG;
	MATCH_TUNING m_Tuning = TuningCalcDefault();
	int m_SelectedHand = 0;
	int m_LastActiveHand = -1;
	int m_ChangeDirections[2] = { 1, 1 };
	ARM_TYPE m_Types[2] = { ARM_TYPE_TENTACLE, ARM_TYPE_TENTACLE };
	// 左右それぞれ全種類を起動時に作る。装備切り替えでは作り直さない。
	std::unique_ptr<Arm> m_Arms[2][ARM_TYPE_COUNT];
	ArmPressCalc m_Presses[2];
	ArmUseCalc m_Uses[2];
	ParryCalc m_Guards[2];
	float m_GuardFeedbackSeconds = 0.0f; // 描画だけの残り秒数
	bool m_GuardFeedbackParry = false;
	VEC3 m_GuardFeedbackPosition = {}; // 防いだ瞬間の肩の前の座標
	int m_GuardHand = -1; // 両手なら後から構えた手で受ける
};

/// @brief 腕の日本語名を取得する（範囲外は「不明」）
const char* ArmGetName(ARM_TYPE type);
