#include "game/match.h"
#include "core/debug_ostream.h"
#include "core/gametime.h"
#include "game/fightermanager.h"
#include "game/arm.h"
#include "input/pad.h"

#ifdef _DEBUG
#include <iostream>
#endif


/*------------------------------------------------------------------------------
	調整用の値
------------------------------------------------------------------------------*/
static constexpr float MATCH_SECONDS = 90.0f;	// 制限時間（秒）


void Match::Reset(void)
{
	m_Match.Start(MATCH_SECONDS, GetTotalScore());
	for (int pad = 0; pad < PAD_COUNT; ++pad) PadRumble(pad, 0.0f, 0.0f, 0.0f);
}

void Match::Finalize(void)
{

}

void Match::Update(void)
{
	// KOの持ち主はVersusCalcに任せ、MatchCalcは合計の増分と時間を扱う。
	const MATCH_EVENT event = m_Match.Step(GameTimeGetDeltaTime(), GetTotalScore());
#ifdef _DEBUG
	if (event == MATCH_EVENT_SCORED) {
		hal::dout << "試合：得点 " << m_Match.GetScore() << std::endl;
	}
#endif
	if (event == MATCH_EVENT_TIME_UP) {
		for (int pad = 0; pad < PAD_COUNT; ++pad) PadRumble(pad, 0.0f, 0.0f, 0.0f);
		// つかんでいる敵を離してから止める
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			if (m_Fighters->GetFighter(i).IsHuman()) m_Arms[i].Reset();
		}
#ifdef _DEBUG
		hal::dout << "試合：時間切れ（得点 " << m_Match.GetScore() << "）" << std::endl;
#endif
	}
}

bool Match::IsPlaying(void) const
{
	return m_Match.GetState() == MATCH_STATE_PLAYING;
}

int Match::GetRemainingSeconds(void) const
{
	return m_Match.GetDisplaySeconds();
}

int Match::GetScore(int slot) const
{
	return m_Fighters->GetScore(slot);
}

int Match::GetTotalScore() const
{
	int total = 0;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) total += m_Fighters->GetScore(slot);
	return total;
}

MATCH_STATE Match::GetState(void) const
{
	return m_Match.GetState();
}

void Match::Initialize(const FighterManager& fighters, ArmManager* arms)
{
	m_Fighters = &fighters;
	m_Arms = arms;
	Reset();
}
