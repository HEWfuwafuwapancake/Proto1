#include <math.h>
#include "physics/stagecalc.h"


bool StageCalcIsOnPlatform(const STAGE& stage, float x, float z)
{
	return fabsf(x - stage.CenterX) <= stage.HalfWidth
		&& fabsf(z - stage.CenterZ) <= stage.HalfDepth;
}

bool StageCalcHasSupport(const STAGE& stage, float x, float y, float z)
{
	float landingY = 0.0f;
	return StageCalcFindLanding(stage, x, z, y, y, landingY);
}

bool StageCalcFindLanding(const STAGE& stage, float x, float z, float previousY, float y, float& landingY)
{
	if (y > previousY) return false;

	bool found = y <= stage.TopY && stage.TopY <= previousY && StageCalcIsOnPlatform(stage, x, z);
	if (found) landingY = stage.TopY;
	for (int i = 0; i < stage.PlatformCount; ++i) {
		const STAGE_PLATFORM& platform = stage.Platforms[i];
		if (platform.TopY < y || platform.TopY > previousY || (found && platform.TopY <= landingY)) continue;
		if (fabsf(x - platform.CenterX) <= platform.HalfWidth
			&& fabsf(z - platform.CenterZ) <= platform.HalfDepth) {
			landingY = platform.TopY;
			found = true;
		}
	}
	return found;
}

bool StageCalcIsOut(const STAGE& stage, float y)
{
	return y < stage.OutY;
}
