#include "game/match.h"
#include "core/debug_ostream.h"
#include "core/gametime.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/arm.h"
#include "input/action.h"
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


static MatchCalc g_Match = {};


void MatchInitialize(void)
{
	g_Match.Start(MATCH_SECONDS, DummyGetTotalOutCount());
	PadRumble(0.0f, 0.0f, 0.0f);
	PadSetLightColor(PLAYING_LIGHT_COLOR[0], PLAYING_LIGHT_COLOR[1], PLAYING_LIGHT_COLOR[2]);
}

void MatchFinalize(void)
{

}

void MatchUpdate(void)
{
	// やり直し：離す → 戻す（ダミーを戻す前に触腕を離す。カメラはプレイヤーを見るので、プレイヤーのあと）
	if (ActionIsTrigger(ACTION_RESTART)) {
		ArmReset();
		PlayerInitialize();
		DummyInitialize();
		CameraInitialize();
		MatchInitialize();
#ifdef _DEBUG
		hal::dout << "試合：やり直し" << std::endl;
#endif
		return;
	}

	const MATCH_EVENT event = g_Match.Step(GameTimeGetDeltaTime(), DummyGetTotalOutCount());
#ifdef _DEBUG
	if (event == MATCH_EVENT_SCORED) {
		hal::dout << "試合：得点 " << g_Match.GetScore() << std::endl;
	}
#endif
	if (event == MATCH_EVENT_TIME_UP) {
		PadRumble(0.0f, 0.0f, 0.0f);
		PadSetLightColor(FINISHED_LIGHT_COLOR[0], FINISHED_LIGHT_COLOR[1], FINISHED_LIGHT_COLOR[2]);
		ArmReset();	// つかんでいる敵を離してから止める
#ifdef _DEBUG
		hal::dout << "試合：時間切れ（得点 " << g_Match.GetScore() << "）" << std::endl;
#endif
	}
}

bool MatchIsPlaying(void)
{
	return g_Match.GetState() == MATCH_STATE_PLAYING;
}

int MatchGetRemainingSeconds(void)
{
	return g_Match.GetDisplaySeconds();
}

int MatchGetScore(void)
{
	return g_Match.GetScore();
}

MATCH_STATE MatchGetState(void)
{
	return g_Match.GetState();
}
