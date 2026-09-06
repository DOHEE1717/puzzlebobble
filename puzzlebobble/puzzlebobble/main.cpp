#include "Hedder.h"
#include "Bubble.h"
#include "BackGround.h"
#include "Grid.h"
#include "Score.h"
#include "Shooter.h"
#include "AimNeedle.h"
#include <cstdio>
#include <cstring>

const int SCREEN_W = 640;
const int SCREEN_H = 480;

const float SHOT_SPEED = 800.f;

float angleDeg = 90.0f;
const float MIN_ANGLE = 8.0f;
const float MAX_ANGLE = 172.0f;

const int NUM_BUBBLE_COLORS = 8;
const int JOKER_CHANCE_PERCENT =5;
int GetRandomBubbleColor()
{
    int r = rand() % 100;

    if (r < JOKER_CHANCE_PERCENT)
        return SPECIAL_COLOR;

    return rand() % (NUM_BUBBLE_COLORS - 1);
}

const float SHOOTER_CENTER_X = SCREEN_W / 2.0f;
const float SHOOTER_CENTER_Y = 430.0f;

const int GRID_ROWS = 24;
const int GRID_COLS = 8;
const int GAMEOVER_ROW = 11;

int shooterColor = 0;
int nextColor = 0;

const int SPRITE_W = 16;
const int SPRITE_H = 16;
const int BUBBLE_RENDER_SIZE = SPRITE_W * 2;

const int MAX_COLS_TOP = 8;
SDL_Texture* gJokerTexture = nullptr;

Shooter gShooter;
AimNeedle gAim;
Score gScore;

int gBombSkillCount = 0;
int gLineSkillCount = 0;


enum class ItemType
{
    None,
    Bomb,
    Line
};

ItemType gHeldItem = ItemType::None;
ItemType gActiveShotItem = ItemType::None;


SDL_Texture* gBombTexture = nullptr;
SDL_Texture* gLineTexture = nullptr;
SDL_Texture* gRuleTableTexture = nullptr;

int gShotsCount = 0;
const int SHOTS_PER_DROP = 6;

int gBrickLines = 0;
int scoreMilestone = 0;

struct AimPath
{
    int pointCount = 0;
    SDL_FPoint points[32];
};

static const int DIGIT_W = 16;
static const int DIGIT_H = 16;
static const int DIGIT_Y = 2766;
static const int DIGIT0_X = 137;
static const int DIGIT_STEP_X = 17;

static void GetDigitRect(int digit, SDL_Rect& outSrc)
{
    if (digit < 0) digit = 0;
    if (digit > 9) digit = 9;

    outSrc.x = DIGIT0_X + digit * DIGIT_STEP_X;
    outSrc.y = DIGIT_Y;
    outSrc.w = DIGIT_W;
    outSrc.h = DIGIT_H;
}


static void RenderNumber(
    SDL_Renderer* renderer,
    SDL_Texture* atlas14029,  
    int value,
    int x, int y,            
    int scale = 2,
    bool alignRight = false,
    int spacing = 2           
)

{
    if (!renderer || !atlas14029) return;

    if (value < 0) value = 0;
     
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", value);

    int len = (int)std::strlen(buf);
    int digitW = DIGIT_W * scale;
    int digitH = DIGIT_H * scale;

    int totalW = len * digitW + (len - 1) * spacing;

    int startX = alignRight ? (x - totalW) : x;

    for (int i = 0; i < len; ++i)
    {
        int d = buf[i] - '0';

        SDL_Rect src;
        GetDigitRect(d, src);
         

        SDL_Rect dst;
        dst.x = startX + i * (digitW + spacing);
        dst.y = y;
        dst.w = digitW;
        dst.h = digitH;
          
       

        SDL_RenderCopy(renderer, atlas14029, &src, &dst);
    }
}



static float GetBubbleRadius()
{
    return BUBBLE_RENDER_SIZE * 0.5f;
}

enum class HitType
{
    None,
    Side,
    Top
};

static bool ComputeWallHit(
    const SDL_Rect& playArea,
    float sx, float sy,
    float dx, float dy,
    bool allowSide,
    bool allowTop,
    float& outT,
    HitType& outType,
    SDL_FPoint& outPos
)
{
    float radius = GetBubbleRadius();

    float left = (float)playArea.x + radius;
    float right = (float)(playArea.x + playArea.w) - radius;
    float top = (float)playArea.y + radius;
    float bottom = (float)(playArea.y + playArea.h) - radius;

    const float INF = 1e9f;
    outT = INF;
    outType = HitType::None;

    if (allowSide && fabsf(dx) > 1e-6f)
    {
        {
            float t = (left - sx) / dx;
            if (t > 0.0f)
            {
                float y = sy + dy * t;
                if (y >= top && y <= bottom && t < outT)
                {
                    outT = t;
                    outType = HitType::Side;
                    outPos.x = left;
                    outPos.y = y;
                }
            }
        }

        {
            float t = (right - sx) / dx;
            if (t > 0.0f)
            {
                float y = sy + dy * t;
                if (y >= top && y <= bottom && t < outT)
                {
                    outT = t;
                    outType = HitType::Side;
                    outPos.x = right;
                    outPos.y = y;
                }
            }
        }
    }

    if (allowTop && fabsf(dy) > 1e-6f)
    {
        float t = (top - sy) / dy;
        if (t > 0.0f)
        {
            float x = sx + dx * t;
            if (x >= left && x <= right && t < outT)
            {
                outT = t;
                outType = HitType::Top;
                outPos.x = x;
                outPos.y = top;
            }
        }
    }

    return (outType != HitType::None);
}

static bool ComputeBubbleHit(
    const BubbleGrid& grid,
    float sx, float sy,
    float dx, float dy,
    float& outT,
    SDL_FPoint& outPos
)
{
    const auto& placed = grid.GetAll();
    float radius = GetBubbleRadius();
    float radius2 = radius * radius;

    const float INF = 1e9f;
    outT = INF;
    bool hit = false;

    for (const auto& b : placed)
    {
        float cx = b.x;
        float cy = b.y;

        float wx = cx - sx;
        float wy = cy - sy;

        float tProj = wx * dx + wy * dy;
        if (tProj < 0.0f)
            continue;

        float dist2Line = wx * wx + wy * wy - tProj * tProj;
        if (dist2Line > radius2)
            continue;

        float h = sqrtf(radius2 - dist2Line);
        float tHit = tProj - h;
        if (tHit < 0.0f)
            tHit = tProj;

        if (tHit > 0.0f && tHit < outT)
        {
            outT = tHit;
            outPos.x = sx + dx * tHit;
            outPos.y = sy + dy * tHit;
            hit = true;
        }
    }

    return hit;
}

static bool ComputeBubbleHitCell(const BubbleGrid& grid, float sx, float sy, float dx, float dy, float& outT, SDL_FPoint& outPos, int& outRow, int& outCol)
{
    const auto& placed = grid.GetAll();
    float radius = GetBubbleRadius();
    float radius2 = radius * radius;

    const float INF = 1e9f;
    outT = INF;
    bool hit = false;

    for (const auto& b : placed)
    {
        float cx = b.x;
        float cy = b.y;

        float wx = cx - sx;
        float wy = cy - sy;

        float tProj = wx * dx + wy * dy;
        if (tProj < 0.0f)
            continue;

        float dist2Line = wx * wx + wy * wy - tProj * tProj;
        if (dist2Line > radius2)
            continue;

        float h = sqrtf(radius2 - dist2Line);
        float tHit = tProj - h;
        if (tHit < 0.0f)
            tHit = tProj;

        if (tHit > 0.0f && tHit < outT)
        {
            outT = tHit;
            outPos.x = sx + dx * tHit;
            outPos.y = sy + dy * tHit;
                        
            outRow = b.row;
            outCol = b.col;

            hit = true;
        }
    }

    return hit;
}
static bool ComputeSkillFirstHitRow(
    const SDL_Rect& playArea,
    const BubbleGrid& grid,
    float startX, float startY,
    float angleDeg,
    int& outRow
)
{
    const float PI = 3.14159265f;

    float rad = (180.0f - angleDeg) * PI / 180.0f;
    float dx = cosf(rad);
    float dy = -sinf(rad);

    SDL_FPoint cur{ startX, startY };

    const int MAX_REFLECT = 30;
    const float INF = 1e9f;

    for (int n = 0; n < MAX_REFLECT; ++n)
    {
        float tWall, tBubble;
        HitType wallType;
        SDL_FPoint posWall, posBubble;

        int hitRow = -1, hitCol = -1;

        bool hasWall = ComputeWallHit(
            playArea,
            cur.x, cur.y,
            dx, dy,
            true,
            true,
            tWall, wallType, posWall);

        bool hasBubble = ComputeBubbleHitCell(
            grid,
            cur.x, cur.y,
            dx, dy,
            tBubble, posBubble,
            hitRow, hitCol);

        if (!hasWall)   tWall = INF;
        if (!hasBubble) tBubble = INF;

        if (tWall == INF && tBubble == INF)
            return false;

     
        if (tBubble < tWall)
        {
            outRow = hitRow;
            return (outRow >= 0);
        }

      
        if (wallType == HitType::Top)
        {
            outRow = 0;
            return true;
        }

       
        if (wallType == HitType::Side)
        {
            dx = -dx;
            cur = posWall;
            cur.x += dx * 0.1f;
            cur.y += dy * 0.1f;
            continue;
        }

        return false;
    }

    return false;
}

bool ComputeAimBouncePath(
    const SDL_Rect& playArea,
    const BubbleGrid& grid,
    float startX, float startY,
    float angleDeg,
    AimPath& outPath)
{
    outPath.pointCount = 0;

    outPath.points[outPath.pointCount++] = { startX, startY };

    const float PI = 3.14159265f;

    float rad = (180.0f - angleDeg) * PI / 180.0f;
    float dx = cosf(rad);
    float dy = -sinf(rad);

    SDL_FPoint cur = outPath.points[0];

    const int MAX_REFLECT = 30;

    for (int n = 0; n < MAX_REFLECT && outPath.pointCount < 32; ++n)
    {
        float tWall, tBubble;
        HitType wallType;
        SDL_FPoint posWall, posBubble;

        bool hasWall = ComputeWallHit(
            playArea,
            cur.x, cur.y,
            dx, dy,
            true,
            true,
            tWall, wallType, posWall);

        bool hasBubble = ComputeBubbleHit(
            grid,
            cur.x, cur.y,
            dx, dy,
            tBubble, posBubble);

        const float INF = 1e9f;
        if (!hasWall)   tWall = INF;
        if (!hasBubble) tBubble = INF;

        if (tWall == INF && tBubble == INF)
            return (outPath.pointCount >= 2);

        if (tBubble < tWall)
        {
            outPath.points[outPath.pointCount++] = posBubble;
            return true;
        }

        outPath.points[outPath.pointCount++] = posWall;

        if (wallType == HitType::Top)
        {
            return true;
        }

        if (wallType == HitType::Side)
        {
            dx = -dx;
            cur = posWall;

            cur.x += dx * 0.1f;
            cur.y += dy * 0.1f;
        }
    }

    return (outPath.pointCount >= 2);
}

bool ComputeSkillHitBubble(
    const SDL_Rect& playArea,
    const BubbleGrid& grid,
    float startX, float startY,
    float angleDeg,
    int& outRow, int& outCol)
{
    const float PI = 3.14159265f;

    float rad = (180.0f - angleDeg) * PI / 180.0f;
    float dx = cosf(rad);
    float dy = -sinf(rad);

    SDL_FPoint cur{ startX, startY };

    const int MAX_REFLECT = 30;
    const float INF = 1e9f;

    for (int n = 0; n < MAX_REFLECT; ++n)
    {
        float tWall, tBubble;
        HitType wallType;
        SDL_FPoint posWall, posBubble;

        int hitRow = -1;
        int hitCol = -1;

        bool hasWall = ComputeWallHit(
            playArea,
            cur.x, cur.y,
            dx, dy,
            true,
            true,
            tWall, wallType, posWall);

        bool hasBubble = ComputeBubbleHitCell(
            grid,
            cur.x, cur.y,
            dx, dy,
            tBubble, posBubble,
            hitRow, hitCol);

        if (!hasWall)   tWall = INF;
        if (!hasBubble) tBubble = INF;

        if (tWall == INF && tBubble == INF)
            return false;

        if (tBubble < tWall)
        {
            outRow = hitRow;
            return (outRow >= 0);
        }

        if (wallType == HitType::Top)
        {
            outRow = 0;
            return true;
        }
        if (wallType == HitType::Side)
        {
            dx = -dx;
            cur = posWall;
            cur.x += dx * 0.1f;
            cur.y += dy * 0.1f;
            continue;
        }
        return false;
    }

    return false;
}

void RenderAimBounce(
    SDL_Renderer* renderer,
    const SDL_Rect& playArea,
    const BubbleGrid& grid,
    float startX, float startY,
    float angleDeg)
{
    AimPath path;
    if (!ComputeAimBouncePath(playArea, grid, startX, startY, angleDeg, path))
        return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);

    const float DOT_STEP = 12.0f;
    const float DOT_LENGTH = 8.0f;
    const int THICKNESS = 2;

    float totalLen = 0.0f;
    for (int i = 0; i < path.pointCount - 1; ++i)
    {
        SDL_FPoint a = path.points[i];
        SDL_FPoint b = path.points[i + 1];

        float vx = b.x - a.x;
        float vy = b.y - a.y;
        float len = sqrtf(vx * vx + vy * vy);
        totalLen += len;
    }
    if (totalLen <= 0.0f)
        return;

    float accumLen = 0.0f;

    for (int i = 0; i < path.pointCount - 1; ++i)
    {
        SDL_FPoint a = path.points[i];
        SDL_FPoint b = path.points[i + 1];

        float vx = b.x - a.x;
        float vy = b.y - a.y;
        float len = sqrtf(vx * vx + vy * vy);
        if (len < 1.0f)
        {
            accumLen += len;
            continue;
        }

        float ux = vx / len;
        float uy = vy / len;

        for (float s = 0.0f; s < len; s += DOT_STEP)
        {
            float e = s + DOT_LENGTH;
            if (e > len) e = len;

            SDL_FPoint p0{ a.x + ux * s, a.y + uy * s };
            SDL_FPoint p1{ a.x + ux * e, a.y + uy * e };

            float tColor = (accumLen + s) / totalLen;

            float r = fabsf(sinf(6.28318f * tColor + 0.0f));
            float g = fabsf(sinf(6.28318f * tColor + 2.094f));
            float b = fabsf(sinf(6.28318f * tColor + 4.188f));

            SDL_SetRenderDrawColor(
                renderer,
                (Uint8)(r * 255),
                (Uint8)(g * 255),
                (Uint8)(b * 255),
                220);

            float nx = -uy;
            float ny = ux;

            for (int k = -THICKNESS; k <= THICKNESS; ++k)
            {
                float ox = nx * (float)k;
                float oy = ny * (float)k;

                SDL_RenderDrawLine(
                    renderer,
                    (int)(p0.x + ox), (int)(p0.y + oy),
                    (int)(p1.x + ox), (int)(p1.y + oy));
            }
        }
        accumLen += len;
    }
}

void RenderSkillUI(SDL_Renderer* renderer, SDL_Texture* atlas14029, const SDL_Rect& area)
{
  

    const int scale = 2;
    const int iconSize = 16 * scale; 
    const int pad = 8;
    const int rowGap = 42;           
    const int numGap = 10;

    {
        SDL_Rect iconDst{ area.x + pad, area.y + pad, iconSize, iconSize };
        if (gBombTexture)
            SDL_RenderCopy(renderer, gBombTexture, nullptr, &iconDst);
                
        int numX = iconDst.x + iconDst.w + numGap;
        int numY = iconDst.y + (iconDst.h - DIGIT_H * scale) / 2;
        RenderNumber(renderer, atlas14029, gBombSkillCount, numX, numY, 2, false, 2);
    }
        
    {
        SDL_Rect iconDst{ area.x + pad, area.y + pad + rowGap, iconSize, iconSize };
        if (gLineTexture)
            SDL_RenderCopy(renderer, gLineTexture, nullptr, &iconDst);

        int numX = iconDst.x + iconDst.w + numGap;
        int numY = iconDst.y + (iconDst.h - DIGIT_H * scale) / 2;
        RenderNumber(renderer, atlas14029, gLineSkillCount, numX, numY, scale, false, 2);
    }
}

int main(int argc, char* argv[])
{
    srand((unsigned int)time(NULL));

    shooterColor = GetRandomBubbleColor();
    nextColor = GetRandomBubbleColor();

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("SDL_Init 실패: %s", SDL_GetError());
        return -1;
    }

    if (TTF_Init() == -1)
    {
        SDL_Log("TTF_Init 실패: %s", TTF_GetError());
        SDL_Quit();
        return -1;
    }

    int imgFlags = IMG_INIT_PNG;
    int initted = IMG_Init(imgFlags);
    SDL_Log("IMG_Init returned flags = 0x%x", initted);

    if ((initted & imgFlags) == 0)
    {
        SDL_Log("IMG_Init PNG 지원 없음 / 실패 : %s", IMG_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "PUZZLE BOBBLE SDL",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_W,
        SCREEN_H,
        0);
    if (window == nullptr)
    {
        SDL_Log("SDL_CreateWindow 실패: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        SDL_Log("SDL_CreateRenderer 실패: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    Background bg{};
    if (!InitBackGround(bg, renderer, SCREEN_W, SCREEN_H))
    {
        SDL_Log("배경 초기화 실패");
    }

    {
        SDL_Rect scoreArea;
        scoreArea.x = 16;
        scoreArea.y = 16;
        scoreArea.w = bg.playArea.x - 32;
        scoreArea.h = 40;

        if (!gScore.Init(renderer,
            "D:/kiu/puzzlebobble/assets/fonts/Galmuri14.ttf",
            20,
            scoreArea))
        {
            SDL_Log("Score Init 실패: %s", TTF_GetError());
        }
    }
      

    char* basePath = SDL_GetBasePath();
    if (!basePath)
    {
        SDL_Log("SDL_GetBasePath 실패: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Log("SDL_GetBasePath: %s", basePath);

    std::string imagePath = std::string(basePath) + "14029.png";
    SDL_free(basePath);
    SDL_Log("현재 이미지 로드 경로 : %s", imagePath.c_str());

    SDL_Surface* bubbleSurface = IMG_Load(imagePath.c_str());
    if (bubbleSurface == nullptr)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "PNG 로드 실패",
            IMG_GetError(),
            window);
        SDL_Log("IMG_Load 실패 : %s", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    Uint32 colorkey = SDL_MapRGB(bubbleSurface->format, 147, 187, 236);
    SDL_SetColorKey(bubbleSurface, SDL_TRUE, colorkey);

    SDL_Texture* bubbleTexture = SDL_CreateTextureFromSurface(renderer, bubbleSurface);
    SDL_FreeSurface(bubbleSurface);

    if (bubbleTexture == nullptr)
    {
        SDL_Log("SDL_CreateTextureFromSurface 실패: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    {
        SDL_Surface* jokerSurf = IMG_Load("D:/kiu/puzzlebobble/puzzlebobble/x64/Debug/jokerbubble.png");
        if (!jokerSurf)
        {
            SDL_Log("조커 버블 PNG 로드 실패: %s", IMG_GetError());
            gJokerTexture = nullptr;
        }
        else
        {
            gJokerTexture = SDL_CreateTextureFromSurface(renderer, jokerSurf);
            SDL_FreeSurface(jokerSurf);
            if (!gJokerTexture)
            {
                SDL_Log("조커 버블 텍스처 생성 실패: %s", SDL_GetError());
            }
        }
    }

   
    {
        SDL_Surface* bombSurf = IMG_Load("D:/kiu/puzzlebobble/puzzlebobble/x64/Debug/Boombubble.png");
        if (!bombSurf)
        {
            SDL_Log("폭탄 스킬 PNG 로드 실패: %s", IMG_GetError());
            gBombTexture = nullptr;
        }
        else
        {
            gBombTexture = SDL_CreateTextureFromSurface(renderer, bombSurf);
            SDL_FreeSurface(bombSurf);

            if (!gBombTexture)
            {
                SDL_Log("폭탄 텍스처 생성 실패: %s", SDL_GetError());
            }
            else
            {
                SDL_SetTextureBlendMode(gBombTexture, SDL_BLENDMODE_BLEND);
            }
        }
    }

    {
        SDL_Surface* lineSurf = IMG_Load("D:/kiu/puzzlebobble/puzzlebobble/x64/Debug/LineBubble.png");
        if (!lineSurf)
        {
            SDL_Log("라인 스킬 PNG 로드 실패: %s", IMG_GetError());
            gLineTexture = nullptr;
        }
        else
        {
            gLineTexture = SDL_CreateTextureFromSurface(renderer, lineSurf);
            SDL_FreeSurface(lineSurf);

            if (!gLineTexture)
            {
                SDL_Log("라인 텍스처 생성 실패: %s", SDL_GetError());
            }
            else
            {
                SDL_SetTextureBlendMode(gLineTexture, SDL_BLENDMODE_BLEND);
            }
        }
    }
    
    {
        SDL_Surface* tableSurf =
            IMG_Load("D:/kiu/puzzlebobble/puzzlebobble/x64/Debug/table.png");

        if (!tableSurf)
        {
            SDL_Log("table.png 로드 실패: %s", IMG_GetError());
            gRuleTableTexture = nullptr;
        }
        else
        {
            gRuleTableTexture = SDL_CreateTextureFromSurface(renderer, tableSurf);
            SDL_FreeSurface(tableSurf);

            if (!gRuleTableTexture)
            {
                SDL_Log("table.png 텍스처 생성 실패: %s", SDL_GetError());
            }
        }
    }

    SDL_SetTextureBlendMode(bubbleTexture, SDL_BLENDMODE_BLEND);

   

    const int FIRST_BUBBLE_X = 12;
    const int FIRST_BUBBLE_Y = 1188;

    const int BUBBLE_GAP = 11;
    const int BUBBLE_STEP_Y = SPRITE_H + BUBBLE_GAP;

    const float LOCAL_SHOOTER_CENTER_X = SCREEN_W / 2.0f;
    const float LOCAL_SHOOTER_CENTER_Y = 440.0f;

    ShotBubble shot{};
    InitShotBubble(shot, shooterColor, LOCAL_SHOOTER_CENTER_X, LOCAL_SHOOTER_CENTER_Y - 20);

    const int MAX_ROW = GRID_ROWS;
    BubbleGrid grid(bg.playArea, BUBBLE_RENDER_SIZE, MAX_ROW, MAX_COLS_TOP);

    Uint32 prevTicks = SDL_GetTicks();
    bool running = true;

    auto DoGameOver = [&](const char* msg)
        {
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_INFORMATION,
                "GameOver",
                msg,
                window
            );
            running = false;
        };

    const int SHOOTER_FRAME_W = 64;
    const int SHOOTER_FRAME_H = 40;

    InitShooter(
        gShooter,
        renderer,
        imagePath.c_str(),
        1, 1805,
        66, 1805,
        SHOOTER_FRAME_W,
        SHOOTER_FRAME_H,
        (int)LOCAL_SHOOTER_CENTER_X,
        (int)LOCAL_SHOOTER_CENTER_Y);

    InitAimNeedle(
        gAim,
        renderer,
        imagePath.c_str(),
        1, 1545,
        64, 64,
        16, 64,
        LOCAL_SHOOTER_CENTER_X, LOCAL_SHOOTER_CENTER_Y - 10);

    std::vector<ExplodingBubble> explodingBubbles;

    struct FallingBubble
    {
        int color;
        float x, y;
        float vy;
    };

    std::vector<FallingBubble> fallingBubbles;

    while (running)
    {
        Uint32 currentTicks = SDL_GetTicks();
        float deltaTime = (currentTicks - prevTicks) / 1000.0f;
        prevTicks = currentTicks;

        bool collidedWithBubble = false;
        int  hitRow = -1;
        int  hitCol = -1;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = false;
                }
                else if (event.key.keysym.sym == SDLK_LEFT)
                {
                    angleDeg -= 1.0f;
                    if (angleDeg < MIN_ANGLE)
                        angleDeg = MIN_ANGLE;
                    StartShooterAnim(gShooter);
                }
                else if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    angleDeg += 1.0f;
                    if (angleDeg > MAX_ANGLE)
                        angleDeg = MAX_ANGLE;
                    StartShooterAnim(gShooter);
                }
                else if (event.key.keysym.sym == SDLK_SPACE)
                {

                    if (!shot.isMoving)
                    {
                        ItemType firedItem = ItemType::None;

                        if (gHeldItem == ItemType::Bomb && gBombSkillCount > 0)
                        {
                            firedItem = ItemType::Bomb;
                            gHeldItem = ItemType::None;
                            --gBombSkillCount;
                        }
                        else if (gHeldItem == ItemType::Line && gLineSkillCount > 0)
                        {
                            firedItem = ItemType::Line;
                            gHeldItem = ItemType::None;
                            --gLineSkillCount;
                        }
                        gActiveShotItem = firedItem;

                        InitShotBubble(
                            shot,
                            shooterColor,
                            LOCAL_SHOOTER_CENTER_X,
                            LOCAL_SHOOTER_CENTER_Y - 20.0f);

                        FireShotBubble(
                            shot,
                            shooterColor,
                            angleDeg,
                            SHOT_SPEED);

                        if (gActiveShotItem == ItemType::None)
                        {
                            shooterColor = nextColor;
                            nextColor = GetRandomBubbleColor();
                        }
                        gShotsCount++;
                    }
                }
                          
                else if (event.key.keysym.sym == SDLK_z)
                {
                  
                    if (!shot.isMoving && gBombSkillCount > 0)
                    {
                        gHeldItem = ItemType::Bomb;
                    }
                }
                else if (event.key.keysym.sym == SDLK_x)
                {
                    
                    if (!shot.isMoving && gLineSkillCount > 0)
                    {
                        gHeldItem = ItemType::Line;
                    }
                }
            }
        }

        bool wasMoving = shot.isMoving;

        SDL_Rect shotArea = bg.playArea;
        shotArea.y += gBrickLines * BUBBLE_RENDER_SIZE;
        shotArea.h -= gBrickLines * BUBBLE_RENDER_SIZE;

        UpdateShooter(gShooter, deltaTime);

        if (shot.isMoving)
        {
            float remaining = deltaTime;
            const float MAX_FRAME = 0.02f;

            while (remaining > 0.0f && shot.isMoving)
            {
                float step = (remaining > MAX_FRAME ? MAX_FRAME : remaining);
                remaining -= step;

                float oldX = shot.x;
                float oldY = shot.y;

                UpdateShotBubble(shot, step, BUBBLE_RENDER_SIZE, shotArea);

                const float COLLISION_RADIUS = BUBBLE_RENDER_SIZE * 0.5f;
                float collideDist = COLLISION_RADIUS * 2.0f;
                float collideDist2 = collideDist * collideDist;

                const auto& placed = grid.GetAll();
                for (const auto& p : placed)
                {
                    float vx = shot.x - oldX;
                    float vy = shot.y - oldY;

                    float segLen2 = vx * vx + vy * vy;
                    if (segLen2 <= 0.000001f)
                        continue;

                    float tx = p.x - oldX;
                    float ty = p.y - oldY;

                    float t = (tx * vx + ty * vy) / segLen2;
                    if (t < 0.0f) t = 0.0f;
                    if (t > 1.0f) t = 1.0f;

                    float closestX = oldX + vx * t;
                    float closestY = oldY + vy * t;

                    float dx = closestX - p.x;
                    float dy = closestY - p.y;
                    float dist2 = dx * dx + dy * dy;

                    if (dist2 <= collideDist2)
                    {
                        float dist = std::sqrt(dist2);
                        if (dist > 0.0001f)
                        {
                            float nx = dx / dist;
                            float ny = dy / dist;

                            shot.x = p.x + nx * collideDist;
                            shot.y = p.y + ny * collideDist;
                        }
                        else
                        {
                            shot.x = oldX;
                            shot.y = oldY;
                        }

                        shot.isMoving = false;

                        collidedWithBubble = true;
                        hitRow = p.row;
                        hitCol = p.col;

                        break;
                    }
                }
            }
        }

        const float EXP_FRAME_TIME = 0.10f;

        for (auto& eb : explodingBubbles)
        {
            if (eb.finished)
            {
                continue;
            }

            eb.elapsed += deltaTime;

            int newFrame = (int)(eb.elapsed / EXP_FRAME_TIME);
            if (newFrame >= 4)
            {
                eb.finished = true;
            }
            else
            {
                eb.frame = newFrame;
            }
        }

        explodingBubbles.erase(
            std::remove_if(explodingBubbles.begin(),
                explodingBubbles.end(),
                [](const ExplodingBubble& e) {return e.finished; }),
            explodingBubbles.end());

        const float GRAVITY = 400.0f;

        for (auto& fb : fallingBubbles)
        {
            fb.vy += GRAVITY * deltaTime;
            fb.y += fb.vy * deltaTime;
        }

        fallingBubbles.erase(
            std::remove_if(fallingBubbles.begin(), fallingBubbles.end(),
                [](const FallingBubble& f) { return f.y - BUBBLE_RENDER_SIZE > SCREEN_H; }),
            fallingBubbles.end());

        if (wasMoving && !shot.isMoving)
        {
            int impactRow = -1;
            int impactCol = -1;

            if (collidedWithBubble)
            {
                impactRow = hitRow;
                impactCol = hitCol;
            }

            else
            {
                int r = -1, c = -1;
                if (grid.WorldToGrid(shot.x, shot.y, r, c))
                {
                    impactRow = r;
                    impactCol = c;
                }
                else
                {
                    impactRow = 0;
                    impactCol = MAX_COLS_TOP / 2;
                }
            }

            if (gActiveShotItem == ItemType::Bomb)
            {
                int removedCount = 0;
                if (impactRow >= 0 && impactCol >= 0)
                {
                    removedCount = grid.RemoveRadius(impactRow, impactCol, 1);
                }

                if (removedCount > 0)
                {
                    int totalScoreGain = 0;

                    const auto& removedCluster = grid.GetLastRemovedCluster();
                    for (const auto& b : removedCluster)
                    {
                        ExplodingBubble eb;
                        eb.color = b.color;
                        eb.x = b.x;
                        eb.y = b.y;
                        eb.elapsed = 0.0f;
                        eb.frame = 0;
                        eb.finished = false;
                        explodingBubbles.push_back(eb);
                    }

                    totalScoreGain += (int)removedCluster.size() * 10;

                    int floatingCount = grid.RemoveFloatingBubbles();
                    if (floatingCount > 0)
                    {
                        const auto& floats = grid.GetLastRemovedFloating();
                        for (const auto& b : floats)
                        {
                            FallingBubble fb;
                            fb.color = b.color;
                            fb.x = b.x;
                            fb.y = b.y;
                            fb.vy = 0.0f;
                            fallingBubbles.push_back(fb);
                        }
                        totalScoreGain += floatingCount * 20;
                    }

                    if (totalScoreGain > 0)
                        gScore.AddScore(totalScoreGain, renderer);

                }
                gActiveShotItem = ItemType::None;
            }

            else if (gActiveShotItem == ItemType::Line)
            {
                int sRow = impactRow;
                if (sRow < 0) sRow = 0;

                int removedCount = grid.RemoveRowAll(sRow);
                if (removedCount > 0)
                {
                    int totalScoreGain = 0;

                    const auto& removedCluster = grid.GetLastRemovedCluster();
                    for (const auto& b : removedCluster)
                    {
                        ExplodingBubble eb;
                        eb.color = b.color;
                        eb.x = b.x;
                        eb.y = b.y;
                        eb.elapsed = 0.0f;
                        eb.frame = 0;
                        eb.finished = false;
                        explodingBubbles.push_back(eb);
                    }

                    totalScoreGain += (int)removedCluster.size() * 10;

                    int floatingCount = grid.RemoveFloatingBubbles();
                    if (floatingCount > 0)
                    {
                        const auto& floats = grid.GetLastRemovedFloating();
                        for (const auto& b : floats)
                        {
                            FallingBubble fb;
                            fb.color = b.color;
                            fb.x = b.x;
                            fb.y = b.y;
                            fb.vy = 0.0f;
                            fallingBubbles.push_back(fb);
                        }
                        totalScoreGain += floatingCount * 20;

                    }
                    if (totalScoreGain > 0)
                        gScore.AddScore(totalScoreGain, renderer);
                }
                gActiveShotItem = ItemType::None;
            }

            else
            {
                int row, col;

                if (!grid.FindNearestEmptyCell(shot.x, shot.y, row, col))
                {
                    SDL_ShowSimpleMessageBox(
                        SDL_MESSAGEBOX_INFORMATION,
                        "GameOver",
                        "더 이상 붙일 수 있는 칸이 없습니다.",
                        window);
                    running = false;
                    continue;
                }
                grid.AddBubble(row, col, shot.color);

                const int MIN_MATCH = 3;
                int removedCount = grid.RemoveCluster(row, col, MIN_MATCH);
                int totalScoreGain = 0;

                bool specialPopped = false;

                if (grid.HasBubbleRow(GAMEOVER_ROW))
                {
                    DoGameOver("패배했습니다.");
                    continue;
                }

                if (removedCount > 0)
                {
                    const auto& removedCluster = grid.GetLastRemovedCluster();
                    for (const auto& b : removedCluster)
                    {
                        ExplodingBubble eb;
                        eb.color = b.color;
                        eb.x = b.x;
                        eb.y = b.y;
                        eb.elapsed = 0.0f;
                        eb.frame = 0;
                        eb.finished = false;
                        explodingBubbles.push_back(eb);

                        if (b.color == SPECIAL_COLOR)
                        {
                            specialPopped = true;
                        }

                    }

                    totalScoreGain += removedCount * 10;

                    int floatingCount = grid.RemoveFloatingBubbles();
                    if (floatingCount > 0)
                    {
                        const auto& floats = grid.GetLastRemovedFloating();
                        for (const auto& b : floats)
                        {
                            FallingBubble fb;
                            fb.color = b.color;
                            fb.x = b.x;
                            fb.y = b.y;
                            fb.vy = 0.0f;
                            fallingBubbles.push_back(fb);
                        }
                        totalScoreGain += floatingCount * 20;
                    }
                }

                if (specialPopped)
                {
                    int r = rand() % 2;

                    if (r == 0)
                    {
                        ++gBombSkillCount;
                    }
                    else
                    {
                        ++gLineSkillCount;
                    }
                }

                if (totalScoreGain > 0)
                    gScore.AddScore(totalScoreGain, renderer);

            }

            int brickLinesBeforeShot = gBrickLines;

            if (gShotsCount >= SHOTS_PER_DROP)
            {
                gShotsCount = 0;

                gBrickLines++;

                int brickHeight = 32;
                int maxLines = bg.playArea.h / brickHeight;
                if (gBrickLines > maxLines)
                    gBrickLines = maxLines;

                grid.SetRowOffset(gBrickLines);

                if (grid.HasBubbleRow(GAMEOVER_ROW))
                {
                    DoGameOver("패배했습니다. 구슬이 제한선에 도달했습니다.");
                    continue;
                }
            }

            int currentScore = gScore.Get();
            int milestone = currentScore / 100;

            if (milestone > scoreMilestone)
            {
                scoreMilestone = milestone;

                if (brickLinesBeforeShot > 0)
                {
                    int targetLines = brickLinesBeforeShot - 1;
                    if (targetLines < 0) targetLines = 0;

                    if (gBrickLines > targetLines)
                    {
                        gBrickLines = targetLines;
                        grid.SetRowOffset(gBrickLines);
                        if (grid.HasBubbleRow(GAMEOVER_ROW))
                        {
                            DoGameOver("패배했습니다. 구슬이 제한선에 도달했습니다.");
                            continue;
                        }
                    }
                }

                else
                {
                    if (gBrickLines > 0)
                    {
                        gBrickLines--;
                        grid.SetRowOffset(gBrickLines);
                        if (grid.HasBubbleRow(GAMEOVER_ROW))
                        {
                            DoGameOver("패배했습니다. 구슬이 제한선에 도달했습니다.");
                            continue;
                        }
                    }
                }

            }

        }
  
           
 

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        RenderBackGround(renderer, bg);
        if (gRuleTableTexture)
        {
            SDL_Rect tableDst;
                        
            const int RIGHT_START_Y = 50;
            const int RIGHT_PAD_L = 45;
            const int RIGHT_PAD_R = 24;
            const int RIGHT_PAD_B = 24;
            const float SCALE = 0.95f;

            int baseX = bg.playArea.x + bg.playArea.w + RIGHT_PAD_L;
            int baseY = RIGHT_START_Y;

            int maxW = SCREEN_W - baseX - RIGHT_PAD_R;
            int maxH = SCREEN_H - baseY - RIGHT_PAD_B;

            tableDst.w = (int)(maxW * SCALE);
            tableDst.h = (int)(maxH * SCALE);

            tableDst.x = baseX + (maxW - tableDst.w) / 2;
            tableDst.y = baseY + (maxH - tableDst.h) / 2;

            SDL_RenderCopy(renderer, gRuleTableTexture, nullptr, &tableDst);
        }

        {
            const int BRICK_SRC_X = 322;
            const int BRICK_SRC_Y = 10;
            const int BRICK_SRC_W = 320;
            const int BRICK_SRC_H = 32;

            SDL_Rect brickSrc;
            brickSrc.x = BRICK_SRC_X;
            brickSrc.y = BRICK_SRC_Y;
            brickSrc.w = BRICK_SRC_W;
            brickSrc.h = BRICK_SRC_H;

            const int BRICK_DST_H = 32;

            for (int i = 0; i < gBrickLines; ++i)
            {
                SDL_Rect brickDst;
                brickDst.w = bg.playArea.w;
                brickDst.h = BRICK_DST_H;
                brickDst.x = bg.playArea.x;
                brickDst.y = bg.playArea.y + i * BRICK_DST_H;

                SDL_RenderCopy(renderer, bubbleTexture, &brickSrc, &brickDst);
            }
        }

        if (!shot.isMoving)
        {
            SDL_Rect aimArea = bg.playArea;
            aimArea.y += gBrickLines * BUBBLE_RENDER_SIZE;
            aimArea.h -= gBrickLines * BUBBLE_RENDER_SIZE;

            RenderAimBounce(
                renderer,
                aimArea,
                grid,
                LOCAL_SHOOTER_CENTER_X,
                LOCAL_SHOOTER_CENTER_Y - 20.0f,
                angleDeg);
        }

        RenderShooter(renderer, gShooter);
        RenderAimNeedle(renderer, gAim, angleDeg);

        if (!shot.isMoving)
        {
            SDL_Rect curDst;
            curDst.w = SPRITE_W * 2;
            curDst.h = SPRITE_H * 2;
            curDst.x = static_cast<int>(LOCAL_SHOOTER_CENTER_X - curDst.w / 2);
            curDst.y = static_cast<int>(LOCAL_SHOOTER_CENTER_Y - curDst.h / 2 - 20);

           
            if (gHeldItem == ItemType::Bomb && gBombTexture)
            {
                SDL_RenderCopy(renderer, gBombTexture, nullptr, &curDst);
            }
            else if (gHeldItem == ItemType::Line && gLineTexture)
            {
                SDL_RenderCopy(renderer, gLineTexture, nullptr, &curDst);
            }
            else
            {
                if (shooterColor == SPECIAL_COLOR && gJokerTexture)
                {
                    SDL_RenderCopy(renderer, gJokerTexture, nullptr, &curDst);
                }
                else
                {
                    SDL_Rect curSrc;
                    GetBubbleSpriteRect(shooterColor, SPRITE_W, SPRITE_H, curSrc);
                    SDL_RenderCopy(renderer, bubbleTexture, &curSrc, &curDst);
                }
            }
        }

        RenderPlacedBubbles(renderer, bubbleTexture, grid.GetAll(), SPRITE_W, SPRITE_H);

        for (const auto& fb : fallingBubbles)
        {
            SDL_Rect src;
            GetBubbleSpriteRect(fb.color, SPRITE_W, SPRITE_H, src);

            SDL_Rect dst;
            dst.w = SPRITE_W * 2;
            dst.h = SPRITE_H * 2;
            dst.x = (int)(fb.x - dst.w / 2);
            dst.y = (int)(fb.y - dst.h / 2);

            if (fb.color == SPECIAL_COLOR && gJokerTexture)
            {
                SDL_RenderCopy(renderer, bubbleTexture, &src, &dst);
            }
            else
            {
                SDL_Rect src2;
                GetBubbleSpriteRect(fb.color, SPRITE_W, SPRITE_H, src2);
                SDL_RenderCopy(renderer, bubbleTexture, &src2, &dst);
            }
        }

        for (const auto& eb : explodingBubbles)
        {
            if (eb.finished)
                continue;

            int explColor = eb.color;
            if (explColor == SPECIAL_COLOR)
            {
                explColor = 0;
            }

            SDL_Rect src;
            GetBubbleExplodeRect(eb.color, eb.frame, src);

            SDL_Rect dst;
            dst.w = SPRITE_W * 2;
            dst.h = SPRITE_H * 2;
            dst.x = (int)(eb.x - dst.w / 2);
            dst.y = (int)(eb.y - dst.h / 2);

            SDL_RenderCopy(renderer, bubbleTexture, &src, &dst);
        }

        {
            SDL_Rect nextDst;
            nextDst.w = SPRITE_W * 2;
            nextDst.h = SPRITE_H * 2;
            nextDst.x = static_cast<int>(LOCAL_SHOOTER_CENTER_X - nextDst.w - 40);
            nextDst.y = static_cast<int>(LOCAL_SHOOTER_CENTER_Y - nextDst.h / 2 + 10);

            if (nextColor == SPECIAL_COLOR && gJokerTexture)
            {
                SDL_RenderCopy(renderer, gJokerTexture, nullptr, &nextDst);
            }
            else
            {
                SDL_Rect nextSrc;
                GetBubbleSpriteRect(nextColor, SPRITE_W, SPRITE_H, nextSrc);
                SDL_RenderCopy(renderer, bubbleTexture, &nextSrc, &nextDst);
            }
        }

        if (shot.isMoving)
        {
            SDL_Rect dst;
            dst.w = SPRITE_W * 2;
            dst.h = SPRITE_H * 2;
            dst.x = (int)(shot.x - dst.w / 2);
            dst.y = (int)(shot.y - dst.h / 2);
                        
            if (gActiveShotItem == ItemType::Bomb && gBombTexture)
            {
                SDL_RenderCopy(renderer, gBombTexture, nullptr, &dst);
            }
            else if (gActiveShotItem == ItemType::Line && gLineTexture)
            {
                SDL_RenderCopy(renderer, gLineTexture, nullptr, &dst);
            }
            else if (shot.color == SPECIAL_COLOR && gJokerTexture)
            {
                SDL_RenderCopy(renderer, gJokerTexture, nullptr, &dst);
            }
            else
            {
                RenderShotBubble(renderer, bubbleTexture, shot, SPRITE_W, SPRITE_H);
            }
        }
                
        {
            const int UI_TOP_Y = 120;
            const int UI_GAP_Y = 16;

            SDL_Rect scoreArea;
            scoreArea.x = 16;
            scoreArea.y = UI_TOP_Y;
            scoreArea.w = bg.playArea.x - 32;
            scoreArea.h = 40;

            int scoreX = scoreArea.x + 8;
            int scoreY = scoreArea.y + 8;
                       
            RenderNumber(renderer, bubbleTexture, gScore.Get(), scoreX, scoreY, 2, false, 2);
                   
            SDL_Rect skillArea;
            skillArea.x = 16;
            skillArea.y = scoreArea.y + scoreArea.h + UI_GAP_Y;
            skillArea.w = bg.playArea.x - 32;
            skillArea.h = 50;

            RenderSkillUI(renderer, bubbleTexture, skillArea);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    gScore.Shutdown();
      

    DestroyAimNeedle(gAim);
    DestroyShooter(gShooter);
    DestroyBackGround(bg);

    if (gJokerTexture)
    {
        SDL_DestroyTexture(gJokerTexture);
        gJokerTexture = nullptr;
    }

    
    if (gBombTexture)
    {
        SDL_DestroyTexture(gBombTexture);
        gBombTexture = nullptr;
    }
    if (gLineTexture)
    {
        SDL_DestroyTexture(gLineTexture);
        gLineTexture = nullptr;
    }

    SDL_DestroyTexture(bubbleTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();
    return 0;
}
