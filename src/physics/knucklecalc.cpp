#include "physics/knucklecalc.h"

KNUCKLE_HIT KnuckleCalcPick(float distance, const KNUCKLE_PARAMS& params)
{
	return distance <= params.PileRange ? params.PileHit : params.Normal;
}
