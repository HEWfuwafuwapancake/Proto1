#pragma once

#include "core/dx11.h"
#include "input/actioncalc.h"
#include "physics/playercalc.h"
#include "physics/dashcalc.h"
#include "physics/dummycalc.h"
#include "physics/knockbackcalc.h"
#include "physics/statuscalc.h"
#include "physics/tentaclecalc.h"
#include "physics/versuscalc.h"
#include "game/character.h"
#include "render/motioncalc.h"

class Stage;
class Camera;
class ArmManager;
class FighterManager;

/// @brief 人とダミー共通の1体（移動・%・吹っ飛び・保持・場外からの復活）
class Fighter
{
public:
	/// @brief ステージ・スロット・機器・最初の足元位置を設定して体を初期化する
	void Initialize(const Stage& stage, int slot, int device, const VEC3& spawn, const SkinnedModel* models);
	/// @brief 描画するキャラを設定し、変わったら待機姿勢に戻す
	void SetCharacter(CHARACTER_TYPE character);
	/// @brief 描画するキャラを取得する
	CHARACTER_TYPE GetCharacter() const { return m_Character; }
	/// @brief 人ごとの共通色を取得する（1P赤・2P青・3P緑・4P黄。範囲外は白）
	static XMFLOAT4 GetPlayerColor(int slot);
	/// @brief 最初の位置・0%・生存状態に戻す（機器の割り当てはそのまま）
	void Reset();
	/// @brief 次のリセットと復活で使う最初の足元位置を設定する（今の位置は動かさない）
	void SetSpawn(const VEC3& spawn) { m_Spawn = spawn; }
	/// @brief ダメージと吹っ飛び速度の倍率を試合開始時に設定する
	void SetTuningScales(float damage, float knockback) { m_DamageScale = damage; m_KnockbackScale = knockback; }
	/// @brief 体を1フレーム進める（人だけカメラ・腕を渡す。ダミーはnullptr）
	/// @return このフレームの場外・復活イベント
	DUMMY_EVENT Update(const Camera* camera, const ArmManager* arms);
	/// @brief カメラ更新後、ターゲットか移動方向へ向く（保持中・吹っ飛び中・復活待ちは回らない）
	void UpdateFacing(const Camera& camera, const ArmManager& arms, const FighterManager& fighters, int slot);
	/// @brief 腕の更新後、ゲームの経過秒で描画専用のポーズを進める
	void UpdateAnimation(const ArmManager* arms, float dt);
	/// @brief 生存している体を描く
	/// @param highlighted 強調を設定する（true：ターゲットの色、false：通常の色）
	void Draw(bool highlighted) const;
	/// @brief 描画するモデルのワールド行列（大きさ・足元・傾き・脚の向き・位置。しびれ中は震えのずれも足す）を取得する
	XMMATRIX GetModelWorld() const;
	/// @brief アニメの腕の骨から、描画用の腕の付け根・向き・長さを取得する（描画だけに使う）
	/// @param hand 0：左手、1：右手
	/// @param shoulder 肩の骨のワールド座標を入れる先
	/// @param direction 肩から肘への単位方向を入れる先
	/// @param length 肩から手先までの骨の長さの合計を入れる先
	/// @return 取得できたら true（腕の骨がないキャラ・モデルがないときは false）
	bool GetArmBone(int hand, VEC3& shoulder, VEC3& direction, float& length) const;
	/// @brief 攻撃クリップの腕と武器の向きを、アニメの骨から取得する（描画だけに使う）
	/// @param hand 0：左手、1：右手
	/// @param arm 肩から手首への単位方向を入れる先
	/// @param weapon 手首から手先への単位方向を入れる先
	/// @param own その手が攻撃しているかを入れる先（true：その手の攻撃クリップ、false：もう一方の手の攻撃につられて動いている）
	/// @return クリップの重み（0〜1。攻撃クリップを再生していない・骨がないなら 0）
	float GetAttackArm(int hand, VEC3& arm, VEC3& weapon, bool& own) const;
	/// @brief 描画の上半身の向き（狙いの向きと、走りの傾き）を、直交する単位方向で取得する（描画だけに使う）
	/// @param up 上半身の上の方向を入れる先
	/// @param forward 上半身の前の方向を入れる先
	/// @param right 上半身の右の方向を入れる先
	void GetUpperBodyAxes(VEC3& up, VEC3& forward, VEC3& right) const;
	/// @brief 足元のワールド座標を取得する
	XMFLOAT3 GetPosition() const;
	/// @brief 足元の位置を設定する（速度はそのまま）
	void SetPosition(const XMFLOAT3& position);
	/// @brief 足元の位置を置きかえ、水平の速さを 0 にする（ワープ。接地と上下の速さはそのまま）
	void Teleport(const VEC3& position);
	/// @brief 引っぱられ中を設定する（true：通常物理を止めて速度0・空中にする、false：通常物理に戻す）
	void SetPulled(bool pulled);
	/// @brief 体の高さを取得する
	float GetHeight() const;
	/// @brief 左右の向き（ラジアン）を取得する
	float GetYaw() const { return m_Yaw; }
	/// @brief このフレームで復活したか取得する
	bool DidRespawn() const { return m_Respawned; }
	/// @brief 復活待ち以外ならtrueを返す
	bool IsActive() const { return m_Life.GetState() == DUMMY_STATE_ACTIVE; }
	/// @brief 保持されているか取得する
	bool IsHeld() const { return m_HeldBy >= 0; }
	/// @brief 指定したスロットに保持されているか取得する
	bool IsHeldBy(int holder) const { return IsActive() && IsHeld() && m_HeldBy == holder; }
	/// @brief グラップリングで引っぱられているか取得する
	bool IsPulled() const { return m_Pulled; }
	/// @brief 腕を操作できる状態か取得する（吹っ飛び中も使える。吹っ飛ばされた直後の一瞬だけ使えず、出していた腕が中断される）
	bool CanUseArms() const { return IsActive() && !IsHeld() && m_FlinchFrames <= 0; }
	/// @brief 体当たりで押し合える状態か取得する（吹っ飛び中・保持中・復活待ちは押し合わない）
	bool CanPushBodies() const { return IsActive() && !IsHeld() && !m_Launched; }
	/// @brief 吹っ飛ばされている途中か取得する
	bool IsLaunched() const { return m_Launched; }
	/// @brief 浮島に立っているか取得する
	bool IsOnGround() const { return m_Body.GetOnGround(); }
	/// @brief 人の体か取得する
	bool IsHuman() const { return m_Device != VERSUS_DEVICE_DUMMY; }
	/// @brief 入力機器番号を取得する
	int GetDevice() const { return m_Device; }
	/// @brief ダメージ%を取得する
	float GetPercent() const { return m_Percent; }
	/// @brief 足元から作った体のカプセルを取得する
	CAPSULE GetBody() const;
	/// @brief 保持中でなければ指定スロットにつかまれ、速度と吹っ飛びを止める（成功時true）
	bool Grab(int holder);
	/// @brief 保持中の体の中心位置を設定する（所有者が違う場合は動かさない）
	void MoveHeld(int holder, const VEC3& center);
	/// @brief 指定した所有者の保持を解除する（解除できたらtrue）
	bool Release(int holder);
	/// @brief ダメージ%を足す（負のダメージは0）
	void AddDamage(float damage);
	/// @brief ダメージ%を減らす（0より下にしない。リペア）
	void Heal(float amount);
	/// @brief ダメージを足し、固定の上向き角度で吹っ飛ばす
	void Hit(const VEC3& direction, float damage, float power);
	/// @brief ダメージを足し、上下を含む指定方向へ吹っ飛ばす
	void Launch(const VEC3& direction, float damage, float power);
	/// @brief %を変えず、指定速度で吹っ飛ばす
	void Push(const VEC3& velocity);
	/// @brief 状態異常をかける（生存中だけ。残り時間は長い方、値は上書き）
	/// @param type 種類
	/// @param seconds 続く秒数
	/// @param value 遅いなら歩く速さの倍率、燃えるなら1秒あたりの %
	/// @param attacker かけた人のスロット番号（なしは -1）
	void ApplyStatus(STATUS_TYPE type, float seconds, float value, int attacker);
	/// @brief 状態異常を1フレーム進め、燃えた分を返す（復活待ちは何もしない）
	STATUS_STEP StepStatus(float dt);
	/// @brief その状態異常がかかっているか取得する
	bool IsStatus(STATUS_TYPE type) const { return IsActive() && m_Status.IsActive(type); }
	/// @brief Game が決めた今と前のフレームの入力を渡す
	void SetInput(const PLAYER_INPUT& current, const PLAYER_INPUT& previous);
	/// @brief このフレームの装備の直接選択を取得する
	ACTION_EQUIP_KEY GetEquipInput() const { return m_Input.Equip; }
	/// @brief この体に渡された移動入力を取得する
	ACTION_AXIS GetMoveInput() const;
	/// @brief フリーカメラのときの攻撃の左右の向きを取得する（左スティックを倒していればその向き、なければ体の向き）
	/// @param cameraYaw この体のカメラの左右の向き（ラジアン）
	float GetFreeAimYaw(float cameraYaw) const;
	/// @brief この体に渡されたカメラ入力を取得する
	ACTION_AXIS GetLookInput() const;
	/// @brief この体に渡されたボタンが押されているか取得する
	bool IsPress(ACTION action) const;
	/// @brief この体に渡されたボタンを押した瞬間か取得する
	bool IsTrigger(ACTION action) const;

private:
	void PlaceAtSpawn();
	void SetLaunchVelocity(const VEC3& velocity);
	KNOCKBACK_PARAMS KnockbackParams() const;
	XMFLOAT3 GetStunShake() const;
	void DrawStunStars() const;
	void StartDashAnimation(bool onGround); // ダッシュのクリップを頭から見せる（描画だけ）
	PlayerBodyCalc m_Body;
	StatusCalc m_Status;
	DummyLifeCalc m_Life;
	VEC3 m_Spawn = {};
	float m_Percent = 0.0f;
	float m_DamageScale = 1.0f;
	float m_KnockbackScale = 1.0f;
	float m_Yaw = 0.0f;
	bool m_Launched = false;
	int m_FlinchFrames = 0; // 吹っ飛ばされた直後に腕を使えないフレーム数（出していた腕を中断させる）
	DashCalc m_Dash;
	int m_HeldBy = -1;
	bool m_Pulled = false;
	bool m_Respawned = false;
	int m_ArmActiveHands = 0;	// 前のフレームで使っていた手（左 1・右 2 のビット。使い始めを見つける）
	int m_Device = VERSUS_DEVICE_DUMMY;
	int m_Slot = 0;
	const SkinnedModel* m_Models = nullptr;
	CHARACTER_TYPE m_Character = CHARACTER_DEFAULT;
	SKINNED_POSE m_Pose;
	int m_AttackHand = -1; // 最後に攻撃クリップを再生した手（左 0・右 1。描画だけ）
	MotionCalc m_Motion; // 描画だけの脚の向き・ねじれ・傾き
	float m_LaunchAnimElapsed = 0.0f; // 吹っ飛ばされてからの秒数（描画だけ。のけぞりが終わったら進めない）
	float m_DashAnimElapsed = -1.0f; // ダッシュを始めてからの秒数（描画だけ。負ならダッシュのクリップを使わない）
	bool m_DashAnimAir = false; // 空中で始めたダッシュか（描画だけ）
	float m_DashAnimYaw = 0.0f; // ダッシュの向き（描画だけ）
	PLAYER_INPUT m_Input;
	PLAYER_INPUT m_PreviousInput;
	const Stage* m_Stage = nullptr;
};
