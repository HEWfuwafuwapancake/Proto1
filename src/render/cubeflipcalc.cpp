#include "render/cubeflipcalc.h"
#include <cmath>

// 90度の回転をラジアンで表す。
static constexpr float QUARTER_TURN = 1.57079632679f;

CUBE_FLIP CubeFlipCalc(float progress, float height, int direction)
{
	const float angle = progress * QUARTER_TURN;
	const float previousFacing = progress >= 1.0f ? 0.0f : cosf(angle);
	const float nextFacing = progress <= 0.0f ? 0.0f : sinf(angle);
	// cos と sin の比を保ち、2面が表示枠からはみ出さないよう高さを配分する。
	const float previousHeight = height * (previousFacing / (previousFacing + nextFacing));
	const float nextHeight = height - previousHeight;
	return {
		{ direction < 0 ? nextHeight : 0.0f, previousHeight, previousFacing },
		{ direction < 0 ? 0.0f : previousHeight, nextHeight, nextFacing },
	};
}
