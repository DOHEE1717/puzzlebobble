#include "Score.h"

bool Score::Init(SDL_Renderer* renderer, const char* fontPath, int fontSize, const SDL_Rect& areaIn)
{
    area = areaIn;
    score = 0;

    font = TTF_OpenFont(fontPath, fontSize);
    if (!font)
    {
        SDL_Log("폰트 출력 실패 : %s", TTF_GetError());
        return false;

    }

    UpdateTexture(renderer);
    return true;
}

void Score::Shutdown()
{
    if (textText)
    {
        SDL_DestroyTexture(textText);
        textText = nullptr;
    }

    if (font)
    {
        TTF_CloseFont(font);
        font = nullptr;
    }

}

void Score::SetScore(int value, SDL_Renderer* renderer)
{
    score = value;
    UpdateTexture(renderer);
}

void Score::AddScore(int delta, SDL_Renderer* renderer)
{
    score += delta;
    UpdateTexture(renderer);
}

void Score::Render(SDL_Renderer* renderer)
{
    if (!textText)
        return;

    SDL_Rect bgRect;
    bgRect.x = textDst.x - padding;
    bgRect.y = textDst.y - padding;
    bgRect.w = textDst.w + padding * 2;
    bgRect.h = textDst.h + padding * 2;

    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_RenderCopy(renderer, textText, nullptr, &textDst);
}

void Score::UpdateTexture(SDL_Renderer* renderer)
{
    if (textText)
    {
        SDL_DestroyTexture(textText);
        textText = nullptr;
    }

    std::string text = std::to_string(score);

    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surf)
    {
        SDL_Log("TTF_RenderText_Blended 실패: %s", TTF_GetError());
        return;
    }

    textText = SDL_CreateTextureFromSurface(renderer, surf);

    textDst.w = surf->w;
    textDst.h = surf->h;

    // 중앙 정렬
    textDst.x = area.x + (area.w - textDst.w) / 2;
    textDst.y = area.y + (area.h - textDst.h) / 2;

    SDL_FreeSurface(surf);
}
