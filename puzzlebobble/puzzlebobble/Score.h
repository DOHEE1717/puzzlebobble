#pragma once
#include "Hedder.h"
#include <string>



class Score
{
public:
	bool Init(SDL_Renderer* renderer, const char* fontPath, int fontSize, const SDL_Rect& areaIn);

	void Shutdown();

	void SetScore(int value, SDL_Renderer* renderer);

	void AddScore(int delta, SDL_Renderer* renderer);

	void Render(SDL_Renderer* renderer);

	int Get() const { return score; }


protected:

private:
	int score = 0;
	TTF_Font* font = nullptr;
	SDL_Texture* textText = nullptr;

	SDL_Rect area{};
	SDL_Rect textDst{};
	SDL_Color bgColor{ 0,0,0,100 };
	int padding = 6;
	SDL_Color color{ 255,255,255,255 };

	void UpdateTexture(SDL_Renderer* renderer);


};