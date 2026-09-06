#pragma once
#include "Hedder.h"

struct AimNeedle
{
	SDL_Texture* texture = nullptr;

	int frameW = 0;
	int frameH = 0;
	int framesPerRow = 0;
	int totalFrames = 0;

	int startX = 0;
	int startY = 0;

	float centerX = 0.0f;
	float centerY = 0.0f;


}; 

bool InitAimNeedle
(
	AimNeedle& needle,
	SDL_Renderer* renderer,
	const char* spritePath,
	int startX,int startY,
	int frameW,int frameH,
	int framesPerRow,
	int totalFrames,
	int centerX,int centerY
);

int GetNeedleFrameIndex(const AimNeedle& needle, float angleDeg);
void RenderAimNeedle(SDL_Renderer* renderer, const AimNeedle& needle, float angleDeg);
void DestroyAimNeedle(AimNeedle& needle);