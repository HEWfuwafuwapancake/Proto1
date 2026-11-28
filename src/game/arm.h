#pragma once

#include "core/dx11.h"
#include "game/stage.h"
#include "physics/armcostcalc.h"
#include "physics/armpresscalc.h"
#include "physics/armusecalc.h"
#include "physics/parrycalc.h"
#include "physics/guardcalc.h"
#include "physics/scenecalc.h"
#include "physics/tuningcalc.h"
#include "render/animcalc.h"
#include "physics/vec3.h"
#include "audio/se.h"
#include <memory>

class Fighter;
class FighterManager;
class ArmManager;
class Camera;

/// @brief 腕の種類（1段目の数字キー 1〜8 の順）
enum ARM_TYPE
{
	ARM_TYPE_TENTACLE, ///< 触手（1段目 1）
	ARM_TYPE_HAMMER, ///< ハンマー（1段目 2）
	ARM_TYPE_KNUCKLE, ///< ナックル（1段目 3）
	ARM_TYPE_LASER, ///< レーザー（1段目 4）
	ARM_TYPE_KATANA, ///< 刀（1段目 5）
	ARM_TYPE_FLAIL, ///< モーニングスター（1段目 6）
	ARM_TYPE_SPEAR, ///< 槍（1段目 7）
	ARM_TYPE_MISSILE, ///< ミサイル（1段目 8）
	ARM_TYPE_NONE, ///< 素手（装備なし）
	ARM_TYPE_COUNT ///< 腕の種類の数（装備ではない）
};

/// @brief 腕が使うゲームの部品への参照（所有しない）
struct ARM_LINKS
{
	FighterManager* Fighters; ///< 自分と相手の体・候補・保持・命中を受け持つ管理
	int Slot; ///< 自分のスロット番号（0〜3。相手の候補から除く）
	const Camera* CameraObject; ///< 狙いと投げる向きを読むカメラ
	Stage* StageObject; ///< 浮島と場外の線を読み、倒れる柱に攻撃を当てるステージ
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
	int Hand = -1; ///< 防いだ手（左0・右1・記録なし-1。描画専用）
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
	/// @brief 装備を外すときに呼ぶ（基本は待機に戻す。地面に残る攻撃は残して、外したあとも UpdateLeftover で進める）
	virtual void Unequip() { Reset(); }
	/// @brief 装備していない間も、地面に残る攻撃を進める（試合中だけ。基本は何もしない）
	virtual void UpdateLeftover() {}
	/// @brief 地面に残る攻撃を描画する（装備しているかに関係なく毎回呼ぶ。基本は何もしない）
	virtual void DrawLeftover() {}
	/// @brief 地面に残る攻撃を全部消す（試合の始めとやり直し。基本は何もしない）
	virtual void ClearLeftover() {}
	/// @brief 待機以外なら true を返す
	virtual bool IsActive() const = 0;
	/// @brief 動作後も弾や遅れて効く攻撃が残っていれば true を返す（基本は false）
	virtual bool IsBusy() const { return false; }
	/// @brief 敵をつかんでいる間だけ true を返す
	virtual bool IsHolding() const = 0;
	/// @brief この腕の動作中、もう片手の腕に新しい動作を始めさせないなら true を返す（基本は false）
	virtual bool LocksOtherHand() const { return false; }
	/// @brief 単押しをすぐ受け付けられるなら true を返す（false の間の単押しは少しの間覚えておく。基本は待機中だけ）
	virtual bool CanTakeTap() const { return !IsActive(); }
	/// @brief 長押しをすぐ始められるなら true を返す（false の間は押し続けていれば待つ。基本は待機中だけ）
	virtual bool CanTakeHold() const { return !IsActive(); }
	/// @brief 歩く速さの倍率を取得する（基本は 1.0、派生クラスで変更できる）
	virtual float GetMoveScale() const { return 1.0f; }
	/// @brief この腕の動作中、ジャンプをできなくするなら true を返す（基本は false）
	virtual bool BlocksJump() const { return false; }
	/// @brief この腕の動作中、ダッシュ（ステップ）をできなくするなら true を返す（基本は false）
	virtual bool BlocksDash() const { return false; }
	/// @brief 突進のワールド速度を取得する（水平のみ。突進していなければゼロ）
	virtual VEC3 GetRushVelocity() const { return {}; }
	/// @brief 今のガードを取得する（基本はガードなし）
	virtual GUARD GetGuard() const { return {}; }
	/// @brief 今の動作で体に見せる攻撃クリップを取得する（基本は使わない）
	virtual ARM_BODY_CLIP GetBodyClip() const { return {}; }
	/// @brief 狙って撃つ腕が、狙う向きへ体を向けている間なら true を返す（レーザーの溜めと撃った直後。基本は false）
	virtual bool IsAiming() const { return false; }
	/// @brief 溜めの割合を取得する（0：溜め始め、1：溜めきった。溜めていなければ -1。基本は -1）
	virtual float GetAimCharge() const { return -1.0f; }
	/// @brief 受け持つ手（左 0・右 1）を取得する
	int GetHand() const { return m_Hand; }

protected:
	/// @brief 手とゲームの部品への参照を保持する（モデルは派生クラスの生成時に読む）
	Arm(int hand, const ARM_LINKS& links) : m_Hand(hand), m_Links(links) {}

	/// @brief 耐久の減る量に試合の倍率を適用し、このフレームの消費に加える
	void ConsumeDurability(int cost);
	/// @brief 耐久が減る量の表から1欄を取得する（表はデバッグ用ウィンドウで変えられる）
	/// @param row 腕の行（ARM_COST_ROW）
	/// @param item 攻撃の列（ARM_COST_ITEM）
	int GetCost(ARM_COST_ROW row, ARM_COST_ITEM item) const;
	/// @brief 残り耐久を取得する（無制限の段だけ -1）
	int GetDurability() const;
	/// @brief 残り耐久があるか、DEBUGで0でも使えるならtrueを返す
	bool CanUse() const;
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
	/// @brief 肩からの狙いの単位方向を取得する（ロックオン中は相手の体の中心へ、いなければ画面の正面へ水平。フリーカメラでは体の向きへ水平。使い始めはスティックの向き）
	/// @param keepHeight 上下の扱いを設定する（true：相手への上下も含める、false：水平だけ）
	VEC3 GetAim(bool keepHeight) const;
	/// @brief ロックオン中の相手のスロット番号を取得する（なし・つかめない相手は -1）
	int GetAimTarget() const;
	/// @brief 腕の音（振り・発射・溜めなど）を、この腕の肩の位置で鳴らす
	/// @param id 鳴らす音
	/// @param volume 表の音量に掛ける倍率
	void PlaySe(SE_ID id, float volume = 1.0f) const;
	/// @brief 当たった音を、当てた相手の体の位置で鳴らす
	/// @param id 鳴らす音
	/// @param target 当てた相手のスロット（-1 なら肩の位置で鳴らす）
	/// @param volume 表の音量に掛ける倍率
	void PlayHitSe(SE_ID id, int target, float volume = 1.0f) const;
	/// @brief 音を決めた場所で鳴らす（爆発など）
	/// @param id 鳴らす音
	/// @param position 音の出た場所
	/// @param target 当てた相手のスロット（いなければ -1）
	/// @param volume 表の音量に掛ける倍率
	void PlaySeAt(SE_ID id, const VEC3& position, int target = -1, float volume = 1.0f) const;
	/// @brief 攻撃の当たりを探す向きを取得する（狙う相手がいれば、左右は flat のまま相手への上下の傾きを足す）
	/// @details 高台の上・下にいる相手をロックオンして攻撃したとき、相手の高さではなく足元の平面へ攻撃しないようにする
	/// @param flat 攻撃の左右の向き（上下は使わない）
	/// @return 長さ1の向き（狙う相手がいなければ flat を水平にしたもの）
	VEC3 GetStrikeDirection(const VEC3& flat) const;
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
	/// @brief 攻撃の範囲に入った柱（コロシアムの倒れる柱）に、耐久を減らす攻撃を当てる（柱のないマップでは何もしない）
	/// @details 相手の体と同じ範囲の判定（AreaCalcIn〜・CollisionCapsuleCapsule など）に、柱の当たりのカプセルを渡して使う。
	///          同じ手の攻撃は、同じ柱に続けて当たらない（1振りで1回）
	/// @param inside 柱の当たりのカプセルを受け取り、攻撃の範囲に入っていれば true を返す関数
	/// @param damage 柱の耐久を減らす量（相手に入れるダメージ%と同じ値を渡す）
	/// @param direction 攻撃の進む向き（耐久が尽きたら、水平にしたこの向きへ倒れる）
	template <typename Inside>
	void StrikePillars(Inside inside, float damage, const VEC3& direction) const
	{
		StrikePillarsBy(inside, damage, [&](const CAPSULE&) { return direction; });
	}
	/// @brief 攻撃の範囲に入った柱に、中心から外へ倒れる攻撃を当てる（爆発・回転など、まわり全部に当たる攻撃）
	/// @param inside 柱の当たりのカプセルを受け取り、攻撃の範囲に入っていれば true を返す関数
	/// @param damage 柱の耐久を減らす量
	/// @param center 攻撃の中心（柱はこの点から外の向きへ倒れる）
	template <typename Inside>
	void StrikePillarsOutward(Inside inside, float damage, const VEC3& center) const
	{
		StrikePillarsBy(inside, damage, [&](const CAPSULE& body) { return Vec3Sub(Vec3Scale(Vec3Add(body.Start, body.End), 0.5f), center); });
	}
	/// @brief 線（太さのあるカプセル）にかかる柱に攻撃を当てる（突き・光線・通り道など。柱のないマップでは何もしない）
	/// @param line 攻撃の線
	/// @param damage 柱の耐久を減らす量
	/// @param direction 攻撃の進む向き
	void StrikePillarsOnLine(const CAPSULE& line, float damage, const VEC3& direction) const
	{
		StrikePillars([&](const CAPSULE& body) { return CollisionCapsuleCapsule(line, body).Hit; }, damage, direction);
	}
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
	// 範囲に入った立っている柱に、柱ごとの向き（direction(柱の当たり)）で攻撃を当てる。元は「スロット × 2 ＋ 手」。
	template <typename Inside, typename Direction>
	void StrikePillarsBy(Inside inside, float damage, Direction direction) const
	{
		const PillarCalc& pillars = m_Links.StageObject->GetPillars();
		for (int i = 0; i < pillars.GetCount(); ++i) {
			CAPSULE body;
			if (pillars.GetBody(i, body) && inside(body)) m_Links.StageObject->HitPillar(i, GetSlot() * 2 + m_Hand, GetSlot(), damage, direction(body));
		}
	}
	int m_Hand;
	ARM_LINKS m_Links;
	int m_Used = 0;
};

/// @brief 左右の装備と全種類の腕の寿命・更新・描画を管理する
class ArmManager
{
public:
	/// @brief 自分のスロットと体の管理を受け取り、左右とも触手を装備する（体の初期化後）
	void Initialize(Stage& stage, FighterManager& fighters, int slot, const Camera& camera);
	/// @brief 腕の処理を終了し、保持している敵を離す
	void Finalize(void);
	/// @brief 左右の腕を待機に戻す（装備はそのまま。時間切れ・やり直し用）
	void Reset(void);
	/// @brief 試合開始や復活に備えて腕を戻す（GAME は素手、DEBUG は素手だけ触手にする）
	void ResetForMatch(GAME_RULE rule, const MATCH_TUNING& tuning);
	/// @brief 今の試合に適用済みの設定を取得する
	const MATCH_TUNING& GetTuning() const { return m_Tuning; }
	/// @brief 耐久が減る量の表を設定する（オフラインはデバッグ用ウィンドウの表、オンラインははじめの表）
	void SetCosts(const ARM_COST_TABLE& costs) { m_Costs = costs; }
	/// @brief 耐久が減る量の表を取得する
	const ARM_COST_TABLE& GetCosts() const { return m_Costs; }
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
	/// @brief 効いている手をビットで取得する（左 1・右 2。どちらも待機なら 0）
	int GetActiveHands(void) const;
	/// @brief 描画に使う手を取得する（左0・右1・なし-1。攻撃開始とガード開始のうち後の手を優先）
	int GetActiveHand() const;
	/// @brief 片手のガード結果・構え・装備の攻撃の順で、体に重ねる描画クリップを取得する
	/// @param hand 左 0・右 1
	/// @return 攻撃クリップの指定（手が範囲外なら使わない指定）
	ARM_BODY_CLIP GetBodyClip(int hand) const;
	/// @brief どちらかの腕が敵をつかんでいるか（保持中なら true）
	bool IsHolding(void) const;
	/// @brief どちらかの腕が狙って撃つ向きへ体を向けているか（レーザーの溜めと撃った直後なら true）
	bool IsAiming(void) const;
	/// @brief 左右の溜めの割合の大きいほうを取得する（0〜1。どちらも溜めていなければ -1。カメラを肩越しに寄せ、レティクルのゲージに使う）
	float GetAimCharge(void) const;
	/// @brief 左右の装備している腕のガードの強いほうを取得する
	GUARD GetGuard() const;
	/// @brief 左右の装備している腕の、歩く速さの倍率の小さいほうを取得する
	float GetMoveScale() const;
	/// @brief 左右どちらかの装備が、ジャンプをできなくしているなら true を返す
	bool BlocksJump() const;
	/// @brief 左右どちらかの装備が、ダッシュ（ステップ）をできなくしているなら true を返す
	bool BlocksDash() const;
	/// @brief 左右の装備の突進速度を取得する（両手なら後から使った手を優先）
	VEC3 GetRushVelocity() const;
	/// @brief 片手の装備を取得する
	/// @param hand 左 0・右 1
	/// @return 装備している種類。手が範囲外なら ARM_TYPE_COUNT
	ARM_TYPE GetType(int hand) const;
	/// @brief 片手の残り耐久を取得する（無制限の段だけ -1、範囲外は 0）
	int GetDurability(int hand) const;
	/// @brief 残り耐久を0〜100へ収めて設定する（待機中のGAMEの腕が0なら素手になる）
	void SetDurability(int hand, int value);
	/// @brief その手の腕を使える耐久があるか、DEBUGで壊れないならtrueを返す（範囲外はfalse）
	bool CanUseArm(int hand) const;
	/// @brief DEBUGのルールならtrueを返す（耐久は減るが壊れない）
	bool IsDebug() const { return m_Rule == GAME_RULE_DEBUG; }

	/// @brief 最後に防いだ結果と経過時間を取得する（描画専用、通信のハッシュには含めない）
	GUARD_FEEDBACK GetGuardFeedback() const;

	/// @brief 十字キーで選んでいる手を取得する（左 0・右 1）
	int GetSelectedHand() const { return m_SelectedHand; }
	/// @brief 片手の最後の装備変更の向きを取得する（左 0・右 1、次・直接選択 +1、前 -1）
	int GetChangeDirection(int hand) const;

private:
	void ChangeType(int hand, int next, int direction = 1);
	bool IsDurabilityLocked() const;
	bool CanBreak() const;
	void UpdateEquip(const Fighter& player);
	void BreakExhaustedArm(int hand);
	void SpawnBreakEffects(int hand, const VEC3& direction); // 腕が壊れた所に破片と白い光を出す（見た目だけ）
	void SpawnGuardEffects(int attacker, bool parried); // 防いだ・パリィしたときの火花・輪・画面の光（見た目だけ）
	void UpdateWearEffects(float dt); // 耐久が残りわずかな腕から、ときどき煙と火花を出す（見た目だけ）
	void PlayOwnSe(SE_ID id) const; // 腕の破壊・ガードの音を、自分の体の位置で鳴らす
	void UpdateGuards(const Fighter& player, float dt, bool interrupted);
	ARM_LINKS m_Links = {};
	GAME_RULE m_Rule = GAME_RULE_DEBUG;
	MATCH_TUNING m_Tuning = TuningCalcDefault();
	ARM_COST_TABLE m_Costs = ArmCostCalcDefault();
	int m_SelectedHand = 0;
	int m_LastActiveHand = -1;
	int m_LastBodyHand = -1; // 攻撃・ガードのうち後から始めた手（描画だけ。突進速度の選択とは分ける）
	int m_ChangeDirections[2] = { 1, 1 };
	ARM_TYPE m_Types[2] = { ARM_TYPE_TENTACLE, ARM_TYPE_TENTACLE };
	// 左右それぞれ全種類を起動時に作る。装備切り替えでは作り直さない。
	std::unique_ptr<Arm> m_Arms[2][ARM_TYPE_COUNT];
	ArmPressCalc m_Presses[2];
	ArmUseCalc m_Uses[2];
	ParryCalc m_Guards[2];
	float m_GuardFeedbackSeconds = 0.0f; // 描画だけの残り秒数
	bool m_GuardFeedbackParry = false;
	int m_GuardFeedbackHand = -1; // 最後に防いだ手（構えを解いたあとも描画する）
	float m_WearSeconds[2] = {}; // 耐久わずかの煙を次に出すまでの秒数（見た目だけ）
	int m_WearCount[2] = {}; // 耐久わずかの煙を出した回数（何回かに1回火花も出す）
	VEC3 m_GuardFeedbackPosition = {}; // 防いだ瞬間の肩の前の座標
	int m_GuardHand = -1; // 両手なら後から構えた手で受ける
};

/// @brief 腕の日本語名を取得する（範囲外は「不明」）
const char* ArmGetName(ARM_TYPE type);
