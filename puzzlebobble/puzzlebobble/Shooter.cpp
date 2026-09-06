#include "Shooter.h"

bool InitShooter(Shooter& shooter, SDL_Renderer* renderer, const char* spritePath, int frame1X, int frame1Y, int frame2X, int frame2Y, int frameW, int frameH, int dstCenterX, int dstCenterY)
{
	SDL_Surface* surface = IMG_Load(spritePath);
	if (surface == nullptr)
	{
		SDL_Log("Shooter Sprite load 실패 : %s", IMG_GetError());
		return false;
	}

	
	Uint32 key = SDL_MapRGB(surface->format, 147, 187, 236);
	SDL_SetColorKey(surface, SDL_TRUE, key);

	shooter.texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);


	if (shooter.texture == nullptr)
	{
		SDL_Log("Shooter texture 생성 실패: %s", SDL_GetError());
		return false;
	}
	SDL_SetTextureBlendMode(shooter.texture, SDL_BLENDMODE_BLEND);

	shooter.srcFrames[0].x = frame1X;
	shooter.srcFrames[0].y = frame1Y;
	shooter.srcFrames[0].w = frameW;
	shooter.srcFrames[0].h = frameH;

	shooter.srcFrames[1].x = frame2X;
	shooter.srcFrames[1].y = frame2Y;
	shooter.srcFrames[1].w = frameW;
	shooter.srcFrames[1].h = frameH;

	shooter.dstRect.w = frameW * 2;
	shooter.dstRect.h = frameH * 2;
	shooter.dstRect.x = dstCenterX - shooter.dstRect.w / 2;
	shooter.dstRect.y = dstCenterY - shooter.dstRect.h / 2;

	shooter.currentFrame = 0;
	shooter.isAnimating = false;
	shooter.frameTime = 0.05f;
	shooter.animTotalTime = 0.2f;
	shooter.animElapsed = 0.0f;

	return true;
}

void StartShooterAnim(Shooter& shooter)
{
	shooter.isAnimating = true;
	shooter.animElapsed = 0.0f;
	shooter.currentFrame = 0;
}

void UpdateShooter(Shooter& shooter, float dt)
{
	if (!shooter.isAnimating || shooter.texture == nullptr)
		return;

	shooter.animElapsed += dt;

	if (shooter.animElapsed >= shooter.animTotalTime)
	{
		shooter.isAnimating = false;
		shooter.currentFrame = 0;   
		return;
	}
	while (shooter.animElapsed >= shooter.frameTime)
	{
		shooter.animElapsed -= shooter.frameTime;

		shooter.currentFrame = 1 - shooter.currentFrame;
	}
}

void RenderShooter(SDL_Renderer* renderer, const Shooter& shooter)
{
	if (shooter.texture == nullptr)
		return;

	const SDL_Rect& src = shooter.srcFrames[shooter.currentFrame];
	SDL_RenderCopy(renderer, shooter.texture, &src, &shooter.dstRect);
}

void DestroyShooter(Shooter& shooter)
{
	if (shooter.texture)
	{
		SDL_DestroyTexture(shooter.texture);
		shooter.texture = nullptr;
	}
}
