#pragma once
#include "Hedder.h"

struct  Background
{
	SDL_Texture* texture = nullptr;
	

	SDL_Rect baseSrc{};
	SDL_Rect playSrc{};

	SDL_Rect baseDst{};
	SDL_Rect playArea{};

	SDL_Rect controlSrc{};
	SDL_Rect controlDst{};


};

bool InitBackGround(Background& background, SDL_Renderer* renderer, int screenW, int screenH);

void RenderBackGround(SDL_Renderer* renderer, const Background& bg);

void DestroyBackGround(Background& bg);