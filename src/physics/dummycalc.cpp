#include "physics/dummycalc.h"


static constexpr float MIN_HORIZONTAL = 0.1f;		// 押す向きの横の成分がこれより小さければ「真上から重なった」とみなす
static constexpr float SEPARATION_MARGIN = 0.001f;	// 押し出すときに余分に離す距離（ちょうど触れたままにならないように）


void DummyLifeCalc::Reset()
{
	m_State = DUMMY_STATE_ACTIVE;
	m_RespawnTimer = {};
	m_OutCount = 0;
}

DUMMY_EVENT DummyLifeCalc::Update(bool isOut, float respawnSeconds, float dt)
{
	if (m_State == DUMMY_STATE_ACTIVE) {
		if (isOut) {
			m_State = DUMMY_STATE_WAITING;
			m_OutCount++;
			TimerStart(m_RespawnTimer, respawnSeconds);
			return DUMMY_EVENT_OUT;
		}
		return DUMMY_EVENT_NONE;
	}

	// 復活を待っている
	TimerUpdate(m_RespawnTimer, dt);
	if (TimerIsFinished(m_RespawnTimer)) {
		m_State = DUMMY_STATE_ACTIVE;
		return DUMMY_EVENT_RESPAWN;
	}
	return DUMMY_EVENT_NONE;
}

CAPSULE DummyCalcBodyCapsule(const VEC3& feet, float radius, float height)
{
	float top = height - radius;
	if (top < radius) {
		top = radius;	// 高さが半径の2倍より短いときは球
	}
	CAPSULE capsule = {};
	capsule.Start = { feet.X, feet.Y + radius, feet.Z };
	capsule.End = { feet.X, feet.Y + top, feet.Z };
	capsule.Radius = radius;
	return capsule;
}

bool DummyCalcPushByPlayer(VEC3& playerFeet, VEC3& dummyFeet, float radius, float height)
{
	// Normal は「ダミーをプレイヤーから押し出す向き」
	const HIT_RESULT hit = CollisionCapsuleCapsule(
		DummyCalcBodyCapsule(dummyFeet, radius, height),
		DummyCalcBodyCapsule(playerFeet, radius, height));
	if (!hit.Hit) {
		return false;
	}

	const float horizontal = sqrtf(hit.Normal.X * hit.Normal.X + hit.Normal.Z * hit.Normal.Z);
	if (horizontal >= MIN_HORIZONTAL) {
		// 横から：ダミーだけを横に押す。向きを横だけにした分、離れるのに要る距離は Depth / horizontal になる
		const float distance = hit.Depth / horizontal + SEPARATION_MARGIN;
		dummyFeet.X += hit.Normal.X / horizontal * distance;
		dummyFeet.Z += hit.Normal.Z / horizontal * distance;
	}
	else {
		// 真上から：プレイヤーを押し出す（ダミーを地面にめり込ませないように）
		playerFeet = Vec3Sub(playerFeet, Vec3Scale(hit.Normal, hit.Depth + SEPARATION_MARGIN));
	}
	return true;
}
