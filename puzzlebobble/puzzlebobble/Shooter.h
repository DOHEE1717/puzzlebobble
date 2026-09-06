#pragma once
#include "Hedder.h"


struct Shooter
{
	SDL_Texture* texture = nullptr;
	SDL_Rect srcFrames[2];
	SDL_Rect dstRect;

	int currentFrame = 0;
	bool isAnimating = false;
	float frameTime = 0.05f;
	float animTotalTime = 0.2f;
	float animElapsed = 0.0f;
};

bool InitShooter(
	Shooter& shooter,
	SDL_Renderer* renderer,
	const char* spritePath,
	int frame1X,int frame1Y,
	int frame2X,int frame2Y,
	int frameW,int frameH,
	int dstCenterX,int dstCenterY

);

void StartShooterAnim(Shooter& shooter);
void UpdateShooter(Shooter& shooter, float dt);
void RenderShooter(SDL_Renderer* renderer, const Shooter& shooter);
void DestroyShooter(Shooter& shooter);


