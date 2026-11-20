#include "render/quadcalc.h"
#include <cmath>

QUAD2D QuadCalcRect(const RECT2D& rect)
{
	const float right = rect.X + rect.Width;
	const float bottom = rect.Y + rect.Height;
	return { { rect.X, right, rect.X, right }, { rect.Y, rect.Y, bottom, bottom } };
}

QUAD2D QuadCalcSkew(const RECT2D& rect, float lean)
{
	QUAD2D quad = QuadCalcRect(rect);
	const float half = lean * 0.5f;
	quad.X[0] += half;
	quad.X[1] += half;
	quad.X[2] -= half;
	quad.X[3] -= half;
	return quad;
}

QUAD2D QuadCalcRotate(const QUAD2D& quad, float centerX, float centerY, float angle)
{
	// 画面は下がプラスなので、この式で時計回りに回る。
	const float c = cosf(angle);
	const float s = sinf(angle);
	QUAD2D rotated{};
	for (int i = 0; i < 4; ++i) {
		const float dx = quad.X[i] - centerX;
		const float dy = quad.Y[i] - centerY;
		rotated.X[i] = centerX + dx * c - dy * s;
		rotated.Y[i] = centerY + dx * s + dy * c;
	}
	return rotated;
}
