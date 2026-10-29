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
static constexpr float PLAYING_LIGHT_COLOR[3] = { 0.95f, 0.45f, 0.35f }; // プレイヤーのサーモンピンク
static constexpr float FINISHED_LIGHT_COLOR[3] = { 1.0f, 0.0f, 0.0f };   // 時間切れは赤


void Match::Reset(void)
{
	m_Match.Start(MATCH_SECONDS, m_Fighters->GetScore(0));
	PadRumble(0, 0.0f, 0.0f, 0.0f);
	PadSetLightColor(0, PLAYING_LIGHT_COLOR[0], PLAYING_LIGHT_COLOR[1], PLAYING_LIGHT_COLOR[2]);
}

void Match::Finalize(void)
{

}

void Match::Update(void)
{
	// MatchCalcには場外総数の代わりに、VersusCalcで確定した1PのKO累計を渡す。
	const MATCH_EVENT event = m_Match.Step(GameTimeGetDeltaTime(), m_Fighters->GetScore(0));
#ifdef _DEBUG
	if (event == MATCH_EVENT_SCORED) {
		hal::dout << "試合：得点 " << m_Match.GetScore() << std::endl;
	}
#endif
	if (event == MATCH_EVENT_TIME_UP) {
		PadRumble(0, 0.0f, 0.0f, 0.0f);
		PadSetLightColor(0, FINISHED_LIGHT_COLOR[0], FINISHED_LIGHT_COLOR[1], FINISHED_LIGHT_COLOR[2]);
		m_Arms->Reset();	// つかんでいる敵を離してから止める
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

int Match::GetScore(void) const
{
	return m_Match.GetScore();
}

MATCH_STATE Match::GetState(void) const
{
	return m_Match.GetState();
}

void Match::Initialize(const FighterManager& fighters, ArmManager& arms)
{
	m_Fighters = &fighters;
	m_Arms = &arms;
	Reset();
}
