#pragma once

#include "core/dx11.h"
#include "input/action.h"
#include "physics/playercalc.h"
#include "physics/dummycalc.h"
#include "physics/knockbackcalc.h"
#include "physics/tentaclecalc.h"
#include "physics/versuscalc.h"

class Stage;
class Camera;
class ArmManager;
class FighterManager;

/// @brief 人とダミー共通の1体（移動・%・吹っ飛び・保持・場外からの復活）
class Fighter
{
public:
	/// @brief ステージ・機器・最初の足元位置を設定して体を初期化する
	void Initialize(const Stage& stage, int device, const VEC3& spawn);
	/// @brief 最初の位置・0%・生存状態に戻す（機器の割り当てはそのまま）
	void Reset();
	/// @brief 体を1フレーム進める（人だけカメラ・腕を渡す。ダミーはnullptr）
	/// @return このフレームの場外・復活イベント
	DUMMY_EVENT Update(const Camera* camera, const ArmManager* arms);
	/// @brief カメラ更新後、ターゲットか移動方向へ向く（保持中・吹っ飛び中・復活待ちは回らない）
	void UpdateFacing(const Camera& camera, const ArmManager& arms, const FighterManager& fighters, int slot);
	/// @brief 生存している体を描く
	/// @param highlighted 強調を設定する（true：ターゲットの色、false：通常の色）
	void Draw(bool highlighted) const;
	/// @brief 足元のワールド座標を取得する
	XMFLOAT3 GetPosition() const;
	/// @brief 足元の位置を設定する（速度はそのまま）
	void SetPosition(const XMFLOAT3& position);
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
	/// @brief 腕を操作できる状態か取得する
	bool CanUseArms() const { return IsActive() && !IsHeld() && !m_Launched; }
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
	/// @brief ダメージを足し、固定の上向き角度で吹っ飛ばす
	void Hit(const VEC3& direction, float damage, float power);
	/// @brief ダメージを足し、上下を含む指定方向へ吹っ飛ばす
	void Launch(const VEC3& direction, float damage, float power);
	/// @brief %を変えず、指定速度で吹っ飛ばす
	void Push(const VEC3& velocity);
	/// @brief この体の機器の移動入力を取得する（1人用合成入力にも対応）
	ACTION_AXIS GetMoveInput() const;
	/// @brief この体の機器のカメラ入力を取得する（1人用合成入力にも対応）
	ACTION_AXIS GetLookInput() const;
	/// @brief この体の機器のボタンを押した瞬間か取得する
	bool IsTrigger(ACTION action) const;

private:
	void PlaceAtSpawn();
	void SetLaunchVelocity(const VEC3& velocity);
	KNOCKBACK_PARAMS KnockbackParams() const;
	PlayerBodyCalc m_Body;
	DummyLifeCalc m_Life;
	VEC3 m_Spawn = {};
	float m_Percent = 0.0f;
	float m_Yaw = 0.0f;
	bool m_Launched = false;
	int m_HeldBy = -1;
	bool m_Pulled = false;
	bool m_Respawned = false;
	int m_Device = VERSUS_DEVICE_DUMMY;
	const Stage* m_Stage = nullptr;
};
