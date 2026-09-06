#include "AimNeedle.h"
#include <cmath>


struct NeedleFrame
{
	int index;
	bool flip;
	
};

struct NeedleSpriteInfo
{
	int srcX;
	int srcY;
	SDL_RendererFlip flip;
};


static const int NEEDLE_FRAME_COUNT = 60;

static const NeedleSpriteInfo gNeedleSprites[NEEDLE_FRAME_COUNT] =
{
	{1,1545,SDL_FLIP_NONE}, //1

	{66,1545,SDL_FLIP_NONE}, //2

	{131,1545,SDL_FLIP_NONE}, //3

	{196,1545,SDL_FLIP_NONE}, //4

	{261,1545,SDL_FLIP_NONE}, //5

	{326,1545,SDL_FLIP_NONE}, //6

	{391,1545,SDL_FLIP_NONE}, //7

	{456,1545,SDL_FLIP_NONE}, //8

	{521,1545,SDL_FLIP_NONE}, //9

	{586,1545,SDL_FLIP_NONE}, //10

	{651,1545,SDL_FLIP_NONE}, //11

	{716,1545,SDL_FLIP_NONE}, //12

	{781,1545,SDL_FLIP_NONE}, //13

	{846,1545,SDL_FLIP_NONE}, //14

	{911,1545,SDL_FLIP_NONE}, //15

	{976,1545,SDL_FLIP_NONE}, //16

	{1,1610,SDL_FLIP_NONE}, //17 , 2-1

	{66,1610,SDL_FLIP_NONE}, //18

	{131,1610,SDL_FLIP_NONE}, //19

	{196,1610,SDL_FLIP_NONE}, //20

	{261,1610,SDL_FLIP_NONE}, //21

	{326,1610,SDL_FLIP_NONE}, //22 

	{391,1610,SDL_FLIP_NONE}, //23

	{456,1610,SDL_FLIP_NONE}, //24

	{521,1610,SDL_FLIP_NONE}, //25

	{586,1610,SDL_FLIP_NONE}, //26

	{651,1610,SDL_FLIP_NONE}, //27

	{716,1610,SDL_FLIP_NONE}, //28

	{781,1610,SDL_FLIP_NONE}, //29

	{846,1610,SDL_FLIP_NONE}, //30

	{911,1610,SDL_FLIP_NONE}, //31

	{976,1610,SDL_FLIP_NONE}, //32

	{1,1675,SDL_FLIP_NONE}, //33 , 3-1

	{66,1675,SDL_FLIP_NONE}, //34

	{131,1675,SDL_FLIP_NONE}, //35

	{196,1675,SDL_FLIP_NONE}, //36

	{261,1675,SDL_FLIP_NONE}, //37

	{326,1675,SDL_FLIP_NONE}, //38 

	{391,1675,SDL_FLIP_NONE}, //39

	{456,1675,SDL_FLIP_NONE}, //40

	{521,1675,SDL_FLIP_NONE}, //41

	{586,1675,SDL_FLIP_NONE}, //42

	{651,1675,SDL_FLIP_NONE}, //43

	{716,1675,SDL_FLIP_NONE}, //44

	{781,1675,SDL_FLIP_NONE}, //45

	{846,1675,SDL_FLIP_NONE}, //46

	{911,1675,SDL_FLIP_NONE}, //47

	{976,1675,SDL_FLIP_NONE}, //48

	{1,1740,SDL_FLIP_NONE}, //49 , 4-1

	{66,1740,SDL_FLIP_NONE}, //50

	{131,1740,SDL_FLIP_NONE}, //51

	{196,1740,SDL_FLIP_NONE}, //52

	{261,1740,SDL_FLIP_NONE}, //53

	{326,1740,SDL_FLIP_NONE}, //54 

	{391,1740,SDL_FLIP_NONE}, //55

	{456,1740,SDL_FLIP_NONE}, //56

	{521,1740,SDL_FLIP_NONE}, //57

	{586,1740,SDL_FLIP_NONE}, //58

	{651,1740,SDL_FLIP_NONE}, //59

	{716,1740,SDL_FLIP_NONE}, //60

	//{781,1740,SDL_FLIP_NONE}, //61

	//{846,1740,SDL_FLIP_NONE}, //62

	//{911,1740,SDL_FLIP_NONE}, //63

	//{976,1740,SDL_FLIP_NONE}, //64



};





bool InitAimNeedle(
	AimNeedle& needle, 
	SDL_Renderer* renderer,
	const char* spritePath,
	int startX, int startY, 
	int frameW, int frameH,
	int framesPerRow, int totalFrames, 
	int centerX, int centerY)
{
	SDL_Surface* surface = IMG_Load(spritePath);
	if (!surface)
	{
		SDL_Log("aimneedle load 실패 ", IMG_GetError());
		return false;
	}

	Uint32 key = SDL_MapRGB(surface->format, 147, 187, 236);
	SDL_SetColorKey(surface, SDL_TRUE, key);

	needle.texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
		
	if (!needle.texture)
	{
		SDL_Log("AimNeedle texture 생성 실패: %s", SDL_GetError());
		return false;
	}

	SDL_SetTextureBlendMode(needle.texture, SDL_BLENDMODE_BLEND);

	needle.frameW = frameW;
	needle.frameH = frameH;
	needle.framesPerRow = framesPerRow;
	needle.totalFrames = NEEDLE_FRAME_COUNT;

	needle.startX = startX;
	needle.startY = startY;

	needle.centerX = static_cast<float>(centerX);
	needle.centerY = static_cast<float>(centerY);

	
	return true;
}

static int GetNeedleFrameIndex(const AimNeedle& needle, float angleDeg)
{
	if (angleDeg < 8.0f)angleDeg = 8.0f;
	if (angleDeg > 172.0f)angleDeg = 172.0f;

	float spriteAngle;

	if (angleDeg >= 90.0f)
	{
		spriteAngle = angleDeg;
	}

	else
	{
		spriteAngle = 180.0f - angleDeg;
	}

	const float SPRITE_MIN_ANGLE = 90.f;
	const float SPRITE_MAX_ANGLE = 172.0f;
	if (spriteAngle < SPRITE_MIN_ANGLE)spriteAngle = SPRITE_MIN_ANGLE;
	if (spriteAngle > SPRITE_MAX_ANGLE)spriteAngle = SPRITE_MAX_ANGLE;

	float tLinear = (spriteAngle - SPRITE_MIN_ANGLE) / (SPRITE_MAX_ANGLE - SPRITE_MIN_ANGLE);

	const float THRESHOLD = 0.5;
	const float CURVE = 0.8f;

	float t;
	
	if (tLinear <= THRESHOLD)
	{
		t = tLinear;
	}
	else
	{
		float u = (tLinear - THRESHOLD) / (1.0f - THRESHOLD);
		float u2 = powf(u, CURVE);
		t = THRESHOLD + u2 * (1.0f - THRESHOLD);
	}


	const int maxIndex = needle.totalFrames - 1;

	int frameIndex = static_cast<int>(std::round(t * maxIndex));


	if (frameIndex < 0) frameIndex = 0;
	if (frameIndex > maxIndex)frameIndex = maxIndex;

	return frameIndex;

}





void RenderAimNeedle(SDL_Renderer* renderer, const AimNeedle& needle, float angleDeg)
{
	if (!needle.texture)
		return;

	int frameIndex = GetNeedleFrameIndex(needle, angleDeg);

	if (frameIndex < 0)
	{
		frameIndex = 0;
	}
	if (frameIndex >= NEEDLE_FRAME_COUNT)
	{
		frameIndex = NEEDLE_FRAME_COUNT - 1;
	}
	const NeedleSpriteInfo& info = gNeedleSprites[frameIndex];

	SDL_RendererFlip flipFlag = info.flip;

	if (angleDeg < 90.0f)
	{
		flipFlag = (SDL_RendererFlip)(flipFlag | SDL_FLIP_HORIZONTAL);
	}

	
	SDL_Rect src;
	src.x = info.srcX;
	src.y = info.srcY;
	src.w = needle.frameW;
	src.h = needle.frameH;

	SDL_Rect dst;
	dst.w = needle.frameW*2.0;
	dst.h = needle.frameH*2.0;
	dst.x = static_cast<int>(needle.centerX - dst.w / 2);
	const int ARROW_OFFSET_Y = 62;
	dst.y = static_cast<int>(needle.centerY - dst.h + ARROW_OFFSET_Y);

	SDL_RenderCopyEx(
		renderer,
		needle.texture,
		&src,
		&dst,
		0.0,            
		nullptr,
		flipFlag  
	);

}

void DestroyAimNeedle(AimNeedle& needle)
{
	if (needle.texture)
	{
		SDL_DestroyTexture(needle.texture);
		needle.texture = nullptr;
	}
}
