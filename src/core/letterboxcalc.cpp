#include "core/letterboxcalc.h"

LETTERBOX LetterboxCalc(float windowWidth, float windowHeight, float gameWidth, float gameHeight)
{
	if (windowWidth <= 0.0f || windowHeight <= 0.0f || gameWidth <= 0.0f || gameHeight <= 0.0f) return { 0.0f, 0.0f, 1.0f };
	const float scaleX = windowWidth / gameWidth;
	const float scaleY = windowHeight / gameHeight;
	const float scale = scaleX < scaleY ? scaleX : scaleY;
	return { (windowWidth - gameWidth * scale) * 0.5f, (windowHeight - gameHeight * scale) * 0.5f, scale };
}
