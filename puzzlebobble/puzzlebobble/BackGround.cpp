#include "BackGround.h"

static const char* BG_Path = "14029.png";

bool InitBackGround(Background& background, SDL_Renderer* renderer, int screenW, int screenH)
{
    SDL_Surface* surface = IMG_Load(BG_Path);

    if (surface == nullptr)
    {
        SDL_Log("backgrond load 실패 (BackGround): %s", IMG_GetError());
        return false;
    }
    Uint32 key = SDL_MapRGB(surface->format, 147, 187, 236);
    SDL_SetColorKey(surface, SDL_TRUE, key);

    background.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (background.texture == nullptr)
    {
        SDL_Log("Background SDL_texturesurface 실패", SDL_GetError());
        return false;
    }

    background.baseSrc.x = 322;
    background.baseSrc.y = 1087;
    background.baseSrc.w = 320;
    background.baseSrc.h = 224;

    background.playSrc.x = 686;
    background.playSrc.y = 1320;
    background.playSrc.w = 144;
    background.playSrc.h = 224;

    background.baseDst.x = 0;
    background.baseDst.y = 0;
    background.baseDst.w = screenW;
    background.baseDst.h = screenH;
    
    const int PLAYAREA_OFFSET_Y = 36;
    const int CONTROL_HEIGHT = 80;

    background.playArea.w = 256;
    background.playArea.h = screenH;
    background.playArea.x = (screenW - background.playArea.w) / 2;
    background.playArea.y = 32;

    background.controlSrc.x = 1;
    background.controlSrc.y = 10;
    background.controlSrc.w = 320;
    background.controlSrc.h = 224;
       
    background.controlDst.w = 300;
    background.controlDst.h = 600;
    background.controlDst.x = (screenW - background.controlDst.w) / 2;
    background.controlDst.y = screenH - background.controlDst.h*2;

    background.controlDst.y = background.playArea.y + background.playArea.h;
    if (background.controlDst.y + background.controlDst.h > screenH)
    {
        background.controlDst.y = screenH - background.controlDst.h;
    }

    return true;
}



void RenderBackGround(SDL_Renderer* renderer, const Background& bg)
{
    if (bg.texture == nullptr)
        return;
     
    SDL_RenderCopy(renderer, bg.texture, &bg.baseSrc, &bg.baseDst);

    SDL_RenderCopy(renderer, bg.texture, &bg.controlSrc, &bg.controlDst);
    SDL_RenderCopy(renderer, bg.texture, &bg.playSrc, &bg.playArea);


  
  
}

void DestroyBackGround(Background& bg)
{
    if (bg.texture)
    {
        SDL_DestroyTexture(bg.texture);
        bg.texture = nullptr;
    }
}
