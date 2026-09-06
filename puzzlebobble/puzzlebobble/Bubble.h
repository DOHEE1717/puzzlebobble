#pragma once
#include "Hedder.h"
#include<vector>

constexpr int SPECIAL_COLOR = 7;
extern SDL_Texture* gJokerTexture;

struct ShotBubble
{
    int color;
    float x, y;
    float vx, vy;
    bool isMoving;
    bool isJoker;
 
};

struct PlacedBubble
{
    int color;
    float x, y;

    int row;
    int col;
    bool isJoker;
};

struct ExplodingBubble
{
    int color;
    float x, y;
    float elapsed;
    int frame;
    bool finished;
};

constexpr int BUBBLE_EXPL_FRAMES = 4;

void InitShotBubble
(ShotBubble& shot,
    int startColor,
    float startX,
    float startY);

void FireShotBubble(ShotBubble& shot, 
    int color, 
    float angleShot, 
    float speed);

void UpdateShotBubble(ShotBubble& shot, 
    float deltaTime,
    int spriteRenderSize,
    const SDL_Rect& playArea);

void RenderShotBubble(SDL_Renderer* renderer, SDL_Texture* texture,
    const ShotBubble& shot,
    int spriteW, int spriteH);

void RenderPlacedBubbles(SDL_Renderer* renderer, SDL_Texture* texture,
    const std::vector<PlacedBubble>& bubbles,
    int spriteW, int spriteH);

int GetTopColumnIndex(float x,
        const SDL_Rect& playArea,
        int bubbleRenderSize,
        int maxCols);



void GetBubbleSpriteRect(int color, int spriteW, int spriteH, SDL_Rect& outSrc);

void GetBubbleExplodeRect(int color, int frameIndex, SDL_Rect& outSrc);
