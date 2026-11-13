#include "game/fighter.h"
#include "game/fightermanager.h"
#include "game/stage.h"
#include "game/camera.h"
#include "game/arm.h"
#include "core/gametime.h"
#include "physics/cameracalc.h"
#include "render/shape.h"

// 調整用の値（従来のPlayer・DummyManagerと同じ）
static constexpr float MOVE_SPEED = 6.0f;
static constexpr float TURN_SPEED = XMConvertToRadians(720.0f);
static constexpr float JUMP_SPEED = 9.0f;
static constexpr float GRAVITY = 25.0f;
static constexpr float MAX_FALL_SPEED = 12.0f; // 重力で落ちる速さの上限（ふつうのジャンプの着地は約 9 なので、ジャンプの感触は変わらない）
static constexpr int AIR_JUMPS = 1; // 空中でジャンプできる回数（着地で戻る）
static constexpr float AIR_CONTROL = 12.0f; // 吹っ飛び中に、移動の入力の向きへ1秒ごとに足せる横の速さ
static constexpr float DASH_SPEED = 18.0f; // ダッシュの速さ
static constexpr float DASH_SECONDS = 0.2f; // ダッシュで進み続ける秒数（約 3.6 進む）
static constexpr float DASH_COOLDOWN = 0.6f; // ダッシュが終わってから次を出せるまでの秒数
static constexpr int FLINCH_FRAMES = 2; // 吹っ飛ばされた直後に腕を使えないフレーム数（出していた腕を中断させるため。次のフレームから動ける）
static constexpr float BODY_RADIUS = 0.5f;
static constexpr float BODY_HEIGHT = 1.8f;
static constexpr float RESPAWN_SECONDS = 2.0f;
static constexpr float RUN_SPEED_THRESHOLD = 0.25f;
static constexpr float ANIMATION_BLEND_SECONDS = 0.15f;
static constexpr float HIT_FLINCH_SECONDS = 10.0f / 24.0f; // 当たった瞬間ののけぞりを見せる秒数（hit_flinch のクリップの長さ）
// 見た目の動き（描画だけ。当たり判定と m_Yaw は変えない）
static constexpr float LEG_TURN_SPEED = XMConvertToRadians(900.0f); // 走るとき脚が進む向きへ回る速さ（体の向きより速く、脚が先に向く）
static constexpr float LEG_IDLE_TURN_SPEED = XMConvertToRadians(360.0f); // 止まっているとき脚が狙いの向きへ回る速さ
static constexpr float MAX_TWIST = XMConvertToRadians(75.0f); // 上半身を脚からねじれる角度の上限
static constexpr float BACK_RUN_ANGLE = XMConvertToRadians(110.0f); // 進む向きと狙いの差がこれを超えると後ろ走り
static constexpr float BACK_RUN_HYSTERESIS = XMConvertToRadians(10.0f);
static constexpr float LAND_SECONDS = 0.2f; // 着地の動きを見せる秒数
static constexpr float MIN_RUN_PLAY_SPEED = 0.5f; // 走りの再生速度の下限
static constexpr float LEAN_FORWARD = XMConvertToRadians(10.0f); // 全速で走ったときの前傾
static constexpr float LEAN_TURN = 0.06f; // 脚が 1 ラジアン／秒で回るときの内側への傾き（ラジアン）
static constexpr float LEAN_SIDE_MAX = XMConvertToRadians(10.0f); // 内側への傾きの上限
static constexpr float LEAN_RATE = 8.0f; // 傾きが目標へ近づく速さ（1秒あたりの割合）
static constexpr XMFLOAT4 PLAYER_COLORS[FIGHTER_COUNT] = {
	{ 0.95f, 0.2f, 0.2f, 1.0f }, { 0.2f, 0.45f, 1.0f, 1.0f },
	{ 0.2f, 0.85f, 0.3f, 1.0f }, { 1.0f, 0.85f, 0.15f, 1.0f }
};
static constexpr float TARGET_MARKER_RADIUS = 0.16f;
static constexpr float TARGET_MARKER_HEIGHT = 0.2f;
static constexpr XMFLOAT4 DUMMY_COLOR{ 0.35f, 0.55f, 0.95f, 1.0f };
static constexpr XMFLOAT4 TARGET_COLOR{ 1.0f, 0.55f, 0.15f, 1.0f };
static constexpr XMFLOAT4 LAUNCHED_COLOR{ 0.95f, 0.85f, 0.3f, 1.0f };
static constexpr float STATUS_MARKER_RADIUS = 0.1f; // 頭の上の状態異常の球
static constexpr float STATUS_MARKER_HEIGHT = 0.5f;
static constexpr float STATUS_MARKER_GAP = 0.22f;
static constexpr XMFLOAT4 STATUS_COLORS[STATUS_COUNT] = {
	{ 1.0f, 0.95f, 0.2f, 1.0f }, { 0.5f, 0.85f, 1.0f, 1.0f }, { 1.0f, 0.45f, 0.1f, 1.0f }, { 0.6f, 0.6f, 0.6f, 1.0f }
}; // しびれ・遅い・燃える・隠れる（隠れるは描かない）
// しびれ中の見た目（描画だけ。当たり判定・攻撃の起点は動かさない）
static constexpr float STUN_SHAKE_WIDTH = 0.08f; // 体が左右に震える幅
static constexpr float STUN_SHAKE_SPEED = 70.0f; // 震える速さ（ラジアン／秒。約 11 回／秒）
static constexpr int STUN_STAR_COUNT = 3; // 頭の上を回る星の数
static constexpr float STUN_STAR_RADIUS = 0.09f; // 星の球の半径
static constexpr float STUN_STAR_ORBIT = 0.4f; // 星が回る輪の半径
static constexpr float STUN_STAR_HEIGHT = 0.3f; // 星の輪の、頭の上からの高さ
static constexpr float STUN_STAR_BOB = 0.06f; // 星が上下に揺れる幅
static constexpr float STUN_STAR_SPIN = 7.0f; // 星が回る速さ（ラジアン／秒）
static constexpr float KNOCKBACK_BASE_SPEED = 4.0f;
static constexpr float KNOCKBACK_GROWTH = 0.06f; // 100% で今までの 50% と同じ速さ（2026-11-12、管理者）
static constexpr float KNOCKBACK_ANGLE = XMConvertToRadians(30.0f);
static constexpr float KNOCKBACK_AIR_DRAG = 0.5f;
static constexpr float KNOCKBACK_FRICTION = 20.0f;
static constexpr float KNOCKBACK_STOP_SPEED = 0.5f;

KNOCKBACK_PARAMS Fighter::KnockbackParams() const
{
	return { KNOCKBACK_BASE_SPEED, KNOCKBACK_GROWTH, KNOCKBACK_ANGLE,
		KNOCKBACK_AIR_DRAG, KNOCKBACK_FRICTION, KNOCKBACK_STOP_SPEED, GRAVITY,
		MAX_FALL_SPEED, AIR_CONTROL, MOVE_SPEED };
}

XMFLOAT4 Fighter::GetPlayerColor(int slot)
{
	return slot >= 0 && slot < FIGHTER_COUNT ? PLAYER_COLORS[slot] : XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
}

void Fighter::Initialize(const Stage& stage, int slot, int device, const VEC3& spawn, const SkinnedModel* models)
{
	m_Stage = &stage;
	m_Slot = slot;
	m_Device = device;
	m_Spawn = spawn;
	m_Models = models;
	m_Input = {};
	m_PreviousInput = {};
	Reset();
}

void Fighter::SetCharacter(CHARACTER_TYPE character)
{
	if (m_Character == character) return;
	m_Character = character;
	if (m_Models) m_Models[m_Character].Reset(m_Pose);
}

void Fighter::PlaceAtSpawn()
{
	m_Body = {};
	m_Body.SetPosition(m_Spawn);
	m_Body.SetOnGround(true);
	m_Percent = 0.0f;
	m_Yaw = 0.0f;
	m_Launched = false;
	m_FlinchFrames = 0;
	m_HitAnimSeconds = HIT_FLINCH_SECONDS;
	m_Dash.Reset();
	m_HeldBy = -1;
	m_Pulled = false;
	m_Status.Reset();
	m_Models[m_Character].Reset(m_Pose);
	m_Motion.Reset(m_Yaw);
}

void Fighter::Reset()
{
	m_Life.Reset();
	m_Respawned = false;
	PlaceAtSpawn();
}

ACTION_AXIS Fighter::GetMoveInput() const
{
	return ActionCalcPlayerMove(m_Input);
}

ACTION_AXIS Fighter::GetLookInput() const
{
	return ActionCalcPlayerLook(m_Input);
}

bool Fighter::IsPress(ACTION action) const
{
	return ActionCalcPlayerPress(m_Input, action);
}

bool Fighter::IsTrigger(ACTION action) const
{
	return ActionCalcPlayerTrigger(m_Input, m_PreviousInput, action);
}

void Fighter::SetInput(const PLAYER_INPUT& current, const PLAYER_INPUT& previous)
{
	m_Input = IsHuman() ? current : PLAYER_INPUT{};
	m_PreviousInput = IsHuman() ? previous : PLAYER_INPUT{};
}

DUMMY_EVENT Fighter::Update(const Camera* camera, const ArmManager* arms)
{
	m_Respawned = false;
	const float dt = GameTimeGetDeltaTime();
	const STAGE& stage = m_Stage->GetStage();
	if (m_FlinchFrames > 0) --m_FlinchFrames;
	const DASH_PARAMS dashParams{ DASH_SPEED, DASH_SECONDS, DASH_COOLDOWN };
	if (IsActive() && !IsHeld() && !m_Pulled) {
		PLAYER_PARAMS params{};
		params.Gravity = GRAVITY;
		params.MaxFallSpeed = MAX_FALL_SPEED;
		params.AirJumps = AIR_JUMPS;
		float moveX = 0.0f, moveZ = 0.0f;
		bool jump = false;
		bool dash = false;
		VEC3 rushVelocity{};
		if (IsHuman() && camera != nullptr && arms != nullptr) {
			const ACTION_AXIS move = GetMoveInput();
			const float yaw = camera->GetYaw();
			moveX = cosf(yaw) * move.X + sinf(yaw) * move.Y;
			moveZ = -sinf(yaw) * move.X + cosf(yaw) * move.Y;
			params.MoveSpeed = MOVE_SPEED * arms->GetMoveScale() * m_Status.GetMoveScale();
			params.JumpSpeed = JUMP_SPEED;
			if (!m_Status.IsActive(STATUS_STUN)) {
				jump = IsTrigger(ACTION_JUMP);
				rushVelocity = arms->GetRushVelocity();
				// 腕の突進中と、腕が足を止めている間（刀が空間を斬っている間など）はダッシュできない。
				dash = IsTrigger(ACTION_DASH) && rushVelocity.X == 0.0f && rushVelocity.Z == 0.0f && arms->GetMoveScale() > 0.0f;
			}
		}
		// ダッシュの向きは移動の入力、入力がなければキャラの向き。
		const bool hasMove = moveX != 0.0f || moveZ != 0.0f;
		const VEC3 dashDirection = hasMove ? VEC3{ moveX, 0.0f, moveZ } : VEC3{ sinf(m_Yaw), 0.0f, cosf(m_Yaw) };
		const bool onGround = m_Body.GetOnGround();
		if (m_Launched) {
			// 吹っ飛び中もすぐ動ける。空中ジャンプとダッシュは吹っ飛びの速度を消さずに足す。
			if (m_Status.IsActive(STATUS_STUN)) moveX = moveZ = 0.0f;
			if (jump) m_Body.Jump(JUMP_SPEED, AIR_JUMPS);
			if (dash && m_Dash.TryStart(dashDirection, onGround, dashParams, false)) {
				const VEC3 push = Vec3Scale(m_Dash.GetDirection(), DASH_SPEED);
				m_Body.SetVelocity({ m_Body.GetVelocityX() + push.X, m_Body.GetVelocityY(), m_Body.GetVelocityZ() + push.Z });
			}
			m_Dash.Step(dt, m_Body.GetOnGround(), dashParams);
			m_Launched = KnockbackCalcStep(m_Body, dt, KnockbackParams(), stage, moveX, moveZ);
		}
		else {
			if (dash) m_Dash.TryStart(dashDirection, onGround, dashParams, true);
			// ジャンプを押したらダッシュをやめて跳ぶ。
			if (jump && m_Dash.IsDashing()) m_Dash.Cancel();
			if (rushVelocity.X != 0.0f || rushVelocity.Z != 0.0f) m_Dash.Cancel();
			const bool dashing = m_Dash.IsDashing();
			const VEC3 dashVelocity = m_Dash.Step(dt, onGround, dashParams);
			if (dashing) {
				// ダッシュ中は落ちない。
				rushVelocity = dashVelocity;
				m_Body.SetVelocity({ dashVelocity.X, 0.0f, dashVelocity.Z });
			}
			m_Body.Step(moveX, moveZ, jump, dt, params, stage, rushVelocity);
		}
	}
	else m_Dash.Step(dt, m_Body.GetOnGround(), dashParams);
	const DUMMY_EVENT event = m_Life.Update(StageCalcIsOut(stage, m_Body.GetY()), RESPAWN_SECONDS, dt);
	if (event == DUMMY_EVENT_OUT) {
		m_HeldBy = -1;
		m_Pulled = false;
	}
	else if (event == DUMMY_EVENT_RESPAWN) {
		PlaceAtSpawn();
		m_Respawned = true;
	}
	return event;
}

void Fighter::UpdateFacing(const Camera& camera, const ArmManager& arms, const FighterManager& fighters, int slot)
{
	if (!CanUseArms() || m_Status.IsActive(STATUS_STUN)) return;
	if (m_Respawned) {
		m_Yaw = camera.GetYaw();
		return;
	}
	// 単押しは離したときに始まるので、押した瞬間では向きを変えない。
	const bool armActive = arms.IsActive();
	const int target = camera.GetTarget();
	float targetYaw = m_Yaw;
	if (target >= 0) {
		const CAPSULE body = fighters.GetTarget(target, slot).Body;
		const VEC3 center = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
		targetYaw = CameraCalcYawTo({ m_Body.GetX(), m_Body.GetY(), m_Body.GetZ() }, center, m_Yaw);
	}
	m_Yaw = PlayerCalcFacing(m_Yaw, m_Body.GetVelocityX(), m_Body.GetVelocityZ(), camera.GetYaw(),
		armActive, TURN_SPEED, GameTimeGetDeltaTime(), target >= 0, targetYaw);
}

void Fighter::UpdateAnimation(const ArmManager* arms, float dt)
{
	if (!IsActive() || !m_Models[m_Character].IsLoaded()) return;
	if (m_Respawned) m_Motion.Reset(m_Yaw);
	MOTION_PARAMS params;
	params.FullSpeed = MOVE_SPEED;
	params.RunThreshold = RUN_SPEED_THRESHOLD;
	params.LegTurnSpeed = LEG_TURN_SPEED;
	params.IdleTurnSpeed = LEG_IDLE_TURN_SPEED;
	params.MaxTwist = MAX_TWIST;
	params.BackAngle = BACK_RUN_ANGLE;
	params.BackHysteresis = BACK_RUN_HYSTERESIS;
	params.LandSeconds = LAND_SECONDS;
	params.MinPlaySpeed = MIN_RUN_PLAY_SPEED;
	params.LeanForward = LEAN_FORWARD;
	params.LeanTurn = LEAN_TURN;
	params.LeanSideMax = LEAN_SIDE_MAX;
	params.LeanRate = LEAN_RATE;
	MOTION_INPUT input;
	input.VelocityX = m_Body.GetVelocityX();
	input.VelocityY = m_Body.GetVelocityY();
	input.VelocityZ = m_Body.GetVelocityZ();
	input.OnGround = m_Body.GetOnGround();
	input.AimYaw = m_Yaw;
	// 吹っ飛び中・保持中・引っぱられ中は走らせない（脚は狙いの向き、空中なら落下）。
	input.Locomotion = !m_Launched && !IsHeld() && !m_Pulled;
	m_Motion.Step(input, params, dt);
	m_HitAnimSeconds = std::min(m_HitAnimSeconds + dt, HIT_FLINCH_SECONDS);

	// 脚は走り・ジャンプなどを続け、腕を使っている手のクリップは上半身だけに重ねる。
	SKINNED_REQUEST request;
	request.Clip = m_Motion.GetClip();
	request.Speed = m_Motion.GetPlaySpeed();
	request.Twist = m_Motion.GetTwist();
	// つかまれ・しびれ・吹っ飛び中は、全身をやられのクリップにする。
	const SKINNED_CLIP reaction = MotionCalcReactionClip(IsHeld() || m_Pulled, m_Status.IsActive(STATUS_STUN), m_Launched);
	if (reaction != SKINNED_CLIP::Count) {
		request.Clip = reaction;
		request.Speed = 1.0f;
	}
	const int hand = IsHuman() && arms ? arms->GetActiveHand() : -1;
	if (hand >= 0) {
		const ARM_BODY_CLIP attack = arms->GetBodyClip(hand);
		if (attack.Active) {
			request.Upper = AnimCalcAttackClip(attack.Kind, hand);
			request.UpperTime = attack.Time;
			request.UpperFull = attack.FullBody;
			m_AttackHand = hand;
		}
		else request.Upper = hand == 0 ? SKINNED_CLIP::Left : SKINNED_CLIP::Right;
		// もう一方の手も攻撃していれば、その腕だけ自分のクリップで動かす。
		const int other = 1 - hand;
		const ARM_BODY_CLIP otherAttack = arms->GetBodyClip(other);
		if (otherAttack.Active) {
			request.Off = AnimCalcAttackClip(otherAttack.Kind, other);
			request.OffTime = otherAttack.Time;
			request.OffHand = other;
		}
	}
	// 当たった直後は、攻撃していなければのけぞりを全身に重ねる（当たるたびに最初から）。
	if (m_HitAnimSeconds < HIT_FLINCH_SECONDS && !AnimCalcIsAttackClip(request.Upper)) {
		request.Upper = SKINNED_CLIP::HitFlinch;
		request.UpperTime = m_HitAnimSeconds;
		request.UpperFull = true;
	}
	m_Models[m_Character].Update(m_Pose, request, dt, ANIMATION_BLEND_SECONDS);
}

void Fighter::Draw(bool highlighted) const
{
	if (!IsActive()) return;
	// 人の体は吹っ飛び中・ターゲット中も本人の色を保つ。
	const XMFLOAT4 color = IsHuman() ? GetPlayerColor(m_Slot)
		: (m_Launched ? LAUNCHED_COLOR : (highlighted ? TARGET_COLOR : DUMMY_COLOR));
	if (m_Models[m_Character].IsLoaded()) {
		m_Models[m_Character].Draw(m_Pose, GetModelWorld(), color);
	} else {
		const XMFLOAT3 p = GetPosition();
		const XMFLOAT3 shake = GetStunShake();
		ShapeDrawCapsule({ p.x + shake.x, p.y, p.z + shake.z }, BODY_RADIUS, BODY_HEIGHT, color);
	}
	DrawStunStars();
	int marker = 0;
	for (int type = 0; type < STATUS_HIDDEN; ++type) {
		// しびれは頭の上を回る星で見せる。
		if (type == STATUS_STUN || !m_Status.IsActive((STATUS_TYPE)type)) continue;
		const XMFLOAT3 p = GetPosition();
		ShapeDrawSphere({ p.x + STATUS_MARKER_GAP * marker++, p.y + BODY_HEIGHT + STATUS_MARKER_HEIGHT, p.z }, STATUS_MARKER_RADIUS, STATUS_COLORS[type]);
	}
	if (IsHuman() && highlighted) {
		const XMFLOAT3 p = GetPosition();
		ShapeDrawSphere({ p.x, p.y + BODY_HEIGHT + TARGET_MARKER_HEIGHT, p.z }, TARGET_MARKER_RADIUS, TARGET_COLOR);
	}
}

XMMATRIX Fighter::GetModelWorld() const
{
	const CHARACTER_DATA& character = CHARACTER_DATA::Get(m_Character);
	const float scale = BODY_HEIGHT / character.ModelHeight;
	const XMFLOAT3 p = GetPosition();
	const XMFLOAT3 shake = GetStunShake();
	// 足元を中心に、前後・左右へ傾けてから脚の向きへ回す（上半身は骨のねじれで狙いの向きになる）。
	// RotationZ はプラスで左へ傾くので、右へ傾く値の符号を変えて渡す。
	return XMMatrixScaling(scale, scale, scale) * XMMatrixRotationY(character.ModelYaw)
		* XMMatrixTranslation(0.0f, -character.ModelFootY * scale, 0.0f)
		* XMMatrixRotationX(m_Motion.GetLeanPitch()) * XMMatrixRotationZ(-m_Motion.GetLeanRoll())
		* XMMatrixRotationY(m_Motion.GetLegYaw()) * XMMatrixTranslation(p.x + shake.x, p.y, p.z + shake.z);
}

// しびれ中だけ、体を左右（体の向きに対して横）に細かく震わせるずれ。描画だけで使う。
XMFLOAT3 Fighter::GetStunShake() const
{
	if (!m_Status.IsActive(STATUS_STUN)) return { 0.0f, 0.0f, 0.0f };
	const float side = sinf(GameTimeGetElapsed() * STUN_SHAKE_SPEED) * STUN_SHAKE_WIDTH;
	return { cosf(m_Yaw) * side, 0.0f, -sinf(m_Yaw) * side };
}

// しびれ中だけ、頭の上に黄色い星を回す。
void Fighter::DrawStunStars() const
{
	if (!m_Status.IsActive(STATUS_STUN)) return;
	const XMFLOAT3 p = GetPosition();
	const float time = GameTimeGetElapsed();
	for (int i = 0; i < STUN_STAR_COUNT; ++i) {
		const float angle = time * STUN_STAR_SPIN + XM_2PI * i / STUN_STAR_COUNT;
		const float bob = sinf(angle * 2.0f) * STUN_STAR_BOB;
		ShapeDrawSphere({ p.x + cosf(angle) * STUN_STAR_ORBIT, p.y + BODY_HEIGHT + STUN_STAR_HEIGHT + bob,
			p.z + sinf(angle) * STUN_STAR_ORBIT }, STUN_STAR_RADIUS, STATUS_COLORS[STATUS_STUN]);
	}
}

float Fighter::GetAttackArm(int hand, VEC3& arm, VEC3& weapon, bool& own) const
{
	if (hand < 0 || hand > 1 || !m_Models || !m_Models[m_Character].IsLoaded()) return 0.0f;
	const bool mainClip = AnimCalcIsAttackClip(m_Pose.Upper) && m_Pose.UpperWeight > 0.0f;
	const bool offClip = m_Pose.Off != SKINNED_CLIP::Count && m_Pose.OffWeight > 0.0f && m_Pose.OffHand == hand;
	if (!mainClip && !offClip) return 0.0f;
	own = offClip || hand == m_AttackHand;
	const float weight = offClip ? m_Pose.OffWeight : m_Pose.UpperWeight;
	const SkinnedModel& model = m_Models[m_Character];
	const auto& chain = CHARACTER_DATA::Get(m_Character).ArmChains[hand];
	const XMMATRIX world = GetModelWorld();
	XMVECTOR points[4];
	for (size_t i = 0; i < chain.size(); ++i) {
		ANIM_VEC3 p{};
		if (!model.GetNodePosition(m_Pose, model.FindNode(chain[i]), p)) return 0.0f;
		points[i] = XMVector3TransformCoord(XMVectorSet(p.X, p.Y, p.Z, 1.0f), world);
	}
	const XMVECTOR toHand = XMVectorSubtract(points[2], points[0]);
	const XMVECTOR toEnd = XMVectorSubtract(points[3], points[2]);
	if (XMVectorGetX(XMVector3LengthSq(toHand)) <= 0.0f || XMVectorGetX(XMVector3LengthSq(toEnd)) <= 0.0f) return 0.0f;
	XMFLOAT3 a, b;
	XMStoreFloat3(&a, XMVector3Normalize(toHand));
	XMStoreFloat3(&b, XMVector3Normalize(toEnd));
	arm = { a.x, a.y, a.z };
	weapon = { b.x, b.y, b.z };
	return weight;
}

bool Fighter::GetArmBone(int hand, VEC3& shoulder, VEC3& direction, float& length) const
{
	if (hand < 0 || hand > 1 || !m_Models || !m_Models[m_Character].IsLoaded()) return false;
	const SkinnedModel& model = m_Models[m_Character];
	const auto& chain = CHARACTER_DATA::Get(m_Character).ArmChains[hand];
	const XMMATRIX world = GetModelWorld();
	XMVECTOR points[4];
	for (size_t i = 0; i < chain.size(); ++i) {
		ANIM_VEC3 p{};
		if (!model.GetNodePosition(m_Pose, model.FindNode(chain[i]), p)) return false;
		points[i] = XMVector3TransformCoord(XMVectorSet(p.X, p.Y, p.Z, 1.0f), world);
	}
	// 触腕は肘のない1本の腕なので、向きは上腕（肩→肘）、長さは手先までの骨の合計にする。
	length = 0.0f;
	for (size_t i = 1; i < chain.size(); ++i) length += XMVectorGetX(XMVector3Length(XMVectorSubtract(points[i], points[i - 1])));
	const XMVECTOR upper = XMVectorSubtract(points[1], points[0]);
	if (length <= 0.0f || XMVectorGetX(XMVector3LengthSq(upper)) <= 0.0f) return false;
	XMFLOAT3 s, d;
	XMStoreFloat3(&s, points[0]);
	XMStoreFloat3(&d, XMVector3Normalize(upper));
	shoulder = { s.x, s.y, s.z };
	direction = { d.x, d.y, d.z };
	return true;
}

void Fighter::GetUpperBodyAxes(VEC3& up, VEC3& forward, VEC3& right) const
{
	// GetModelWorld と同じ順で傾ける。前は狙いの向きを、傾いた上方向に直交させる。
	const XMMATRIX lean = XMMatrixRotationX(m_Motion.GetLeanPitch()) * XMMatrixRotationZ(-m_Motion.GetLeanRoll())
		* XMMatrixRotationY(m_Motion.GetLegYaw());
	const XMVECTOR u = XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), lean));
	const XMVECTOR aim = XMVectorSet(sinf(m_Yaw), 0.0f, cosf(m_Yaw), 0.0f);
	const XMVECTOR f = XMVector3Normalize(XMVectorSubtract(aim, XMVectorScale(u, XMVectorGetX(XMVector3Dot(aim, u)))));
	const XMVECTOR r = XMVector3Cross(u, f);
	XMFLOAT3 a, b, c;
	XMStoreFloat3(&a, u);
	XMStoreFloat3(&b, f);
	XMStoreFloat3(&c, r);
	up = { a.x, a.y, a.z };
	forward = { b.x, b.y, b.z };
	right = { c.x, c.y, c.z };
}

XMFLOAT3 Fighter::GetPosition() const
{
	return { m_Body.GetX(), m_Body.GetY(), m_Body.GetZ() };
}

void Fighter::SetPosition(const XMFLOAT3& position)
{
	m_Body.SetPosition({ position.x, position.y, position.z });
}

void Fighter::Teleport(const VEC3& position)
{
	m_Body.SetPosition(position);
	m_Body.SetVelocity({ 0.0f, m_Body.GetVelocityY(), 0.0f });
}

void Fighter::SetPulled(bool pulled)
{
	m_Pulled = pulled;
	if (pulled) {
		m_Body.SetVelocity({});
		m_Body.SetOnGround(false);
	}
}

float Fighter::GetHeight() const
{
	return BODY_HEIGHT;
}

CAPSULE Fighter::GetBody() const
{
	return DummyCalcBodyCapsule({ m_Body.GetX(), m_Body.GetY(), m_Body.GetZ() }, BODY_RADIUS, BODY_HEIGHT);
}

bool Fighter::Grab(int holder)
{
	if (!IsActive() || IsHeld() || holder < 0 || holder >= FIGHTER_COUNT) return false;
	m_HeldBy = holder;
	m_Launched = false;
	m_Pulled = false;
	m_Dash.Reset();
	m_Body.ResetAirJumps();
	m_Body.SetVelocity({});
	m_Body.SetOnGround(false);
	return true;
}

void Fighter::MoveHeld(int holder, const VEC3& center)
{
	if (IsHeldBy(holder)) m_Body.SetPosition({ center.X, center.Y - BODY_HEIGHT * 0.5f, center.Z });
}

bool Fighter::Release(int holder)
{
	if (!IsHeldBy(holder)) return false;
	m_HeldBy = -1;
	return true;
}

void Fighter::AddDamage(float damage)
{
	if (IsActive()) m_Percent = KnockbackCalcAddDamage(m_Percent, damage * m_DamageScale);
}

void Fighter::Heal(float amount)
{
	if (IsActive()) m_Percent = KnockbackCalcHeal(m_Percent, amount);
}

void Fighter::ApplyStatus(STATUS_TYPE type, float seconds, float value, int attacker)
{
	if (IsActive()) m_Status.Apply(type, seconds, value, attacker);
}

STATUS_STEP Fighter::StepStatus(float dt)
{
	return IsActive() ? m_Status.Step(dt) : STATUS_STEP{ 0.0f, -1 };
}

void Fighter::SetLaunchVelocity(const VEC3& velocity)
{
	m_Body.SetVelocity(velocity);
	m_Body.SetOnGround(false);
	m_HeldBy = -1;
	m_Pulled = false;
	m_Launched = true;
	m_FlinchFrames = FLINCH_FRAMES;
	m_HitAnimSeconds = 0.0f;
	m_Dash.Cancel();
}

void Fighter::Hit(const VEC3& direction, float damage, float power)
{
	if (!IsActive()) return;
	AddDamage(damage);
	const KNOCKBACK_PARAMS params = KnockbackParams();
	const float speed = KnockbackCalcSpeed(m_Percent, power, params) * m_KnockbackScale;
	SetLaunchVelocity(KnockbackCalcVelocity(direction, speed, params));
}

void Fighter::Launch(const VEC3& direction, float damage, float power)
{
	if (!IsActive()) return;
	AddDamage(damage);
	const float speed = KnockbackCalcSpeed(m_Percent, power, KnockbackParams()) * m_KnockbackScale;
	SetLaunchVelocity(KnockbackCalcDirectedVelocity(direction, speed));
}

void Fighter::Push(const VEC3& velocity)
{
	if (IsActive() && !IsHeld()) SetLaunchVelocity(velocity);
}
