#include "Bubble.h"
#include <cmath>

	static const int BUBBLE_SRC_LEFT_X = 1;
	static const int BUBBLE_SRC_RIGHT_X = 555;
	static const int BUBBLE_SRC_FIRST_Y = 1854;
	static const int BUBBLE_SRC_STEP_Y = 33;
	static const int BUBBLE_COLORS_PER_COLUMN = 4;

	static const int BUBBLE_EXPL_FRAME_W = 32;
	static const int BUBBLE_EXPL_FRAME_H = 32;

	static const int BUBBLE_EXPL_LEFT_FIRST_X = 270;
	static const int BUBBLE_EXPL_RIGHT_FIRST_X = 824;

	static const int BUBBLE_EXPL_FIRST_Y = 1846;
	static const int BUBBLE_EXPL_STEP_Y = 33;
	static const int BUBBLE_EXPL_STEP_X = 33;
	static const int BUBBLE_EXPL_COLORS_PER_COLUMN = 4;


int GetTopColumnIndex(float x, const SDL_Rect& playArea, int bubbleRenderSize, int maxCols)
{
	float radius = bubbleRenderSize * 0.5f;

	float firstCenterX = playArea.x + radius;
	float step = static_cast<float>(bubbleRenderSize);
	float fIndex = (x - firstCenterX) / step;
	int index = static_cast<int>(std::round(fIndex));

	if (index < 0)index = 0;
	if (index > maxCols - 1) index = maxCols - 1;
	return index;
}

void GetBubbleSpriteRect(int color, int spriteW, int spriteH, SDL_Rect& outSrc)
{
	int group = color / 4;
	int idx = color % 4;

	outSrc.x = (group == 0 ? BUBBLE_SRC_LEFT_X : BUBBLE_SRC_RIGHT_X);
	outSrc.y = BUBBLE_SRC_FIRST_Y + idx * BUBBLE_SRC_STEP_Y;
	outSrc.w = spriteW;
	outSrc.h = spriteH;
}

void GetBubbleExplodeRect(int color, int frameIndex, SDL_Rect& outSrc)
{
	if (frameIndex < 0) frameIndex = 0;
	if (frameIndex >= BUBBLE_EXPL_FRAMES)frameIndex = BUBBLE_EXPL_FRAMES - 1;

	int group = color / BUBBLE_EXPL_COLORS_PER_COLUMN;
	int idx = color % BUBBLE_COLORS_PER_COLUMN;

	int baseX = (group == 0 ? BUBBLE_EXPL_LEFT_FIRST_X : BUBBLE_EXPL_RIGHT_FIRST_X);
	int baseY = BUBBLE_EXPL_FIRST_Y + idx * BUBBLE_EXPL_STEP_Y;

	outSrc.x = baseX + frameIndex * BUBBLE_EXPL_STEP_X;
	outSrc.y = baseY;
	outSrc.w = BUBBLE_EXPL_FRAME_W;
	outSrc.h = BUBBLE_EXPL_FRAME_H;

}

void InitShotBubble(ShotBubble& shot, int startColor, float startX, float startY)
{
	shot.color = startColor;
	shot.x = startX;
	shot.y = startY;
	shot.vx = 0.0f;
	shot.vy = 0.0f;
	shot.isMoving = false;
}

void FireShotBubble(ShotBubble& shot, int startcolor, float angleShot, float speed)
{
	shot.color = startcolor;
	const float PI = 3.14159265f;
	float rad = (180.0f-angleShot) * PI / 180.f;

	shot.vx = std::cos(rad) * speed;
	shot.vy = -std::sin(rad) * speed;

	shot.isMoving = true;
}

void UpdateShotBubble(ShotBubble& shot, float deltaTime, int spriteRenderSize, const SDL_Rect& playArea)
{
    if (!shot.isMoving)
        return;

    shot.x += shot.vx * deltaTime;
    shot.y += shot.vy * deltaTime;

    float radius = spriteRenderSize * 0.5f;
    float leftWall = static_cast<float>(playArea.x) + radius;
    float rightWall = static_cast<float>(playArea.x + playArea.w);
    float topWall = static_cast<float>(playArea.y);

    if (shot.x - radius < leftWall)
    {
        shot.x = leftWall + radius;
        shot.vx = -shot.vx;
    }
    else if (shot.x + radius > rightWall)
    {
        shot.x = rightWall - radius;
        shot.vx = -shot.vx;
    }

    if (shot.y - radius < topWall)
    {
        shot.y = topWall + radius;
        shot.vx = 0.0f;
        shot.vy = 0.0f;
        shot.isMoving = false;
    }
}

void RenderShotBubble(SDL_Renderer* renderer, SDL_Texture* texture,
	const ShotBubble& shot,
	int spriteW, int spriteH)
{
	if (!shot.isMoving)
		return;

	SDL_Rect srcRect{};
	GetBubbleSpriteRect(shot.color, spriteW, spriteH, srcRect);

	SDL_Rect dstRect{};
	dstRect.w = spriteW * 2;
	dstRect.h = spriteH * 2;
	dstRect.x = static_cast<int>(shot.x - dstRect.w / 2);
	dstRect.y = static_cast<int>(shot.y - dstRect.h / 2);

	SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
}

void RenderPlacedBubbles(SDL_Renderer* renderer, SDL_Texture* texture,
	const std::vector<PlacedBubble>& bubbles,
	int spriteW, int spriteH)
{
	for (const PlacedBubble& b : bubbles)
	{
		SDL_Rect dstRect{};
		dstRect.w = spriteW * 2;
		dstRect.h = spriteH * 2;
		dstRect.x = static_cast<int>(b.x - dstRect.w / 2);
		dstRect.y = static_cast<int>(b.y - dstRect.h / 2);

		if (b.color == SPECIAL_COLOR && gJokerTexture)
		{
			SDL_RenderCopy(renderer, gJokerTexture, nullptr, &dstRect);
		}
		else
		{
			SDL_Rect srcRect{};
			GetBubbleSpriteRect(b.color, spriteW, spriteH, srcRect);
			SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
		}
	}
}