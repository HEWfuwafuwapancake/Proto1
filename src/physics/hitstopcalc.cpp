#include "physics/hitstopcalc.h"

int HitstopCalcFrames(float damage, float launchSpeed, const HITSTOP_PARAMS& params)
{
	if (damage < params.MinDamage) return 0;
	if (launchSpeed >= params.FinishSpeed) return params.FinishFrames;
	if (damage >= params.HeavyDamage) return params.HeavyFrames;
	if (damage >= params.MediumDamage) return params.MediumFrames;
	return params.LightFrames;
}

void HitstopCalc::Reset()
{
	m_Frames = 0;
}

void HitstopCalc::Start(int frames)
{
	if (frames > m_Frames) m_Frames = frames;
}

bool HitstopCalc::Step()
{
	if (m_Frames <= 0) return false;
	--m_Frames;
	return true;
}
