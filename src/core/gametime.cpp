#include "core/gametime.h"


/*------------------------------------------------------------------------------
	調整用の値
------------------------------------------------------------------------------*/
static constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;	// 1フレームの秒数（メインループは1秒に60回）


static float g_Elapsed = 0.0f;	// ゲームが始まってからの秒数
static float g_Scale = 1.0f;	// 時間の進む速さ（0 で止まる、1 でふつう）


void GameTimeInitialize(void)
{
	g_Elapsed = 0.0f;
	g_Scale = 1.0f;
}

void GameTimeUpdate(void)
{
	g_Elapsed += GameTimeGetDeltaTime();
}

float GameTimeGetDeltaTime(void)
{
	return FIXED_DELTA_TIME * g_Scale;
}

float GameTimeGetElapsed(void)
{
	return g_Elapsed;
}

void GameTimeSetScale(float scale)
{
	// 負の速さ（時間が戻る）は使わない
	g_Scale = scale < 0.0f ? 0.0f : scale;
}

float GameTimeGetScale(void)
{
	return g_Scale;
}
