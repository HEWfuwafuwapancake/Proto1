#include "game/match.h"
#include "core/debug_ostream.h"
#include "core/gametime.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/tentacle.h"
#include "input/action.h"

#ifdef _DEBUG
#include <iostream>
#endif


/*------------------------------------------------------------------------------
	調整用の値
------------------------------------------------------------------------------*/
static constexpr float MATCH_SECONDS = 90.0f;	// 制限時間（秒）


static MATCH g_Match = {};


void MatchInitialize(void)
{
	MatchCalcStart(g_Match, MATCH_SECONDS, DummyGetTotalOutCount());
}

void MatchFinalize(void)
{

}

void MatchUpdate(void)
{
	// やり直し：離す → 戻す（ダミーを戻す前に触腕を離す。カメラはプレイヤーを見るので、プレイヤーのあと）
	if (ActionIsTrigger(ACTION_RESTART)) {
		TentacleReset();
		PlayerInitialize();
		DummyInitialize();
		CameraInitialize();
		MatchInitialize();
#ifdef _DEBUG
		hal::dout << "試合：やり直し" << std::endl;
#endif
		return;
	}

	const MATCH_EVENT event = MatchCalcStep(g_Match, GameTimeGetDeltaTime(), DummyGetTotalOutCount());
#ifdef _DEBUG
	if (event == MATCH_EVENT_SCORED) {
		hal::dout << "試合：得点 " << g_Match.Score << std::endl;
	}
#endif
	if (event == MATCH_EVENT_TIME_UP) {
		TentacleReset();	// つかんでいる敵を離してから止める
#ifdef _DEBUG
		hal::dout << "試合：時間切れ（得点 " << g_Match.Score << "）" << std::endl;
#endif
	}
}

bool MatchIsPlaying(void)
{
	return g_Match.State == MATCH_STATE_PLAYING;
}

int MatchGetRemainingSeconds(void)
{
	return MatchCalcDisplaySeconds(g_Match);
}

int MatchGetScore(void)
{
	return g_Match.Score;
}

MATCH_STATE MatchGetState(void)
{
	return g_Match.State;
}
