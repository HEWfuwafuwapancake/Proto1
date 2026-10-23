#include <math.h>
#include "physics/stagecalc.h"


bool StageCalcIsOnPlatform(const STAGE& stage, float x, float z)
{
	return fabsf(x - stage.CenterX) <= stage.HalfWidth
		&& fabsf(z - stage.CenterZ) <= stage.HalfDepth;
}

bool StageCalcIsOut(const STAGE& stage, float y)
{
	return y < stage.OutY;
}
