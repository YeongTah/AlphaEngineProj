/* Start Header ************************************************************************/
/*!
\file   Level2.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  This file defines the function Load, Initialize, Update, Draw, Free, Unload
        to produce level 2 in the game, loading from Assets/level2.txt.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "leveleditor.hpp"
#include "pch.h"
#include "GridUtils.h"
#include "Level1.h"
#include "Level2.h"
#include "gamestatemanager.h"
#include "GameStateList.h"
#include "Main.h"
#include <iostream>
#include <fstream>
#include <cmath>

// IsAreaClicked has no header declaration - extern needed
extern bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height,
    float click_x, float click_y);

// ---- Level 2 local entities ----
static Entity l2_player;
static Entity l2_mummy;
static Entity l2_mummy2; // second mummy for level 2
static Entity l2_exitPortal;
static Entity l2_coin;
static AEGfxTexture* l2_DesertBlockTex = nullptr;
static AEGfxTexture* l2_FloorTex = nullptr;
static bool  l2_initialised = false;
static int   l2_coinCounter = 0;
static int   l2_turnCounter = 0;
static bool  l2_playerMoved = false;
static float l2_gridStep = 50.0f;
static float l2_nextX = 0.0f;
static float l2_nextY = 0.0f;

// ---- overlay flags ----
static bool l2_paused = false;
static bool l2_showWin = false;
static bool l2_showLose = false;

// ---- overlay button layout ----
static const float kL2BtnRetryX = -200.0f;
static const float kL2BtnRetryY = -130.0f;
static const float kL2BtnExitX = 200.0f;
static const float kL2BtnExitY = -130.0f;
static const float kL2BtnW = 280.0f;
static const float kL2BtnH = 90.0f;

// ---- powerup state ----
static struct L2PowerState {
    bool speed = false;      int speedTurns = 0;
    bool freeze = false;     int freezeTurns = 0;
    bool invincible = false; int invTurns = 0;
    int  invFrames = 0;
} l2Power;

static bool L2IsInvincibleNow() { return l2Power.invincible || (l2Power.invFrames > 0); }
static void L2TickPowers()
{
    if (l2Power.speed && --l2Power.speedTurns <= 0) l2Power.speed = false;
    if (l2Power.freeze && --l2Power.freezeTurns <= 0) l2Power.freeze = false;
    if (l2Power.invincible && --l2Power.invTurns <= 0) l2Power.invincible = false;
}


//----------------------------------------------------------------------------
// Finds nearest free (value==0) grid cell, avoiding a min distance from
// (avoidRow, avoidCol) using Manhattan distance.
//----------------------------------------------------------------------------
static void L2FindFreeSpawnCell(int startRow, int startCol, float& outX, float& outY,
    int avoidRow = -1, int avoidCol = -1, int minDist = 0, int maxRadius = 15)
{
    if (startRow < 0) startRow = 0;
    if (startRow >= GRID_ROWS) startRow = GRID_ROWS - 1;
    if (startCol < 0) startCol = 0;
    if (startCol >= GRID_COLS) startCol = GRID_COLS - 1;

    for (int radius = 0; radius <= maxRadius; ++radius)
    {
        for (int dr = -radius; dr <= radius; ++dr)
        {
            for (int dc = -radius; dc <= radius; ++dc)
            {
                if (abs(dr) != radius && abs(dc) != radius) continue;
                int r = startRow + dr;
                int c = startCol + dc;
                if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) continue;
                if (level[r][c] != 0) continue;
                if (avoidRow >= 0 && avoidCol >= 0)
                {
                    if (abs(r - avoidRow) + abs(c - avoidCol) < minDist) continue;
                }
                GridToWorldCenter(r, c, outX, outY);
                std::cout << "L2 Spawn at grid (" << r << "," << c << ")\n";
                return;
            }
        }
    }
    GridToWorldCenter(startRow, startCol, outX, outY);
    std::cout << "L2 Spawn fallback at grid (" << startRow << "," << startCol << ")\n";
}


//----------------------------------------------------------------------------
// Reads Assets/level2.txt into the shared level[][] grid.
//----------------------------------------------------------------------------
static void L2LoadLevelTxt()
{
    const char* path = "Assets/level2.txt";
    std::ifstream is(path);
    if (!is.is_open())
    {
        std::cout << "Level2: Could not open " << path << " - grid all zeros\n";
        for (int r = 0; r < GRID_ROWS; ++r)
            for (int c = 0; c < GRID_COLS; ++c)
                level[r][c] = 0;
        return;
    }
    int tile; char comma;
    for (int row = 0; row < GRID_ROWS; ++row)
        for (int col = 0; col < GRID_COLS; ++col)
            level[row][col] = (is >> tile >> comma) ? tile : 0;
    is.close();
    std::cout << "Level2: Loaded grid from " << path << "\n";
}


//----------------------------------------------------------------------------
// Resets all Level 2 positions to safe free cells.
//----------------------------------------------------------------------------
static void ResetLevel2()
{
    float px = 0.0f, py = 0.0f;

    L2FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
    l2_player.x = px; l2_player.y = py;

    int playerRow, playerCol;
    WorldToGrid(l2_player.x, l2_player.y, playerRow, playerCol);
    L2FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
    l2_mummy.x = px; l2_mummy.y = py;

    // Second mummy spawns from bottom-center, also at least 10 cells from player
    L2FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
    l2_mummy2.x = px; l2_mummy2.y = py;

    L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
    l2_coin.x = px; l2_coin.y = py;

    l2_nextX = l2_player.x;
    l2_nextY = l2_player.y;
    l2_coinCounter = 0;
    l2_turnCounter = 0;
    l2_playerMoved = false;
    l2Power = {};
}


//----------------------------------------------------------------------------
// Loads Level 2 resources and tile map
//----------------------------------------------------------------------------
void Level2_Load()
{
    std::cout << "Level2:Load\n";

    L2LoadLevelTxt();

    l2_player.pTex = AEGfxTextureLoad("Assets/explorer.png");
    l2_DesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    l2_FloorTex = AEGfxTextureLoad("Assets/Floor.png");
    l2_mummy.pTex = AEGfxTextureLoad("Assets/Enemy.png");
    l2_mummy2.pTex = AEGfxTextureLoad("Assets/Enemy.png");
    l2_coin.pTex = AEGfxTextureLoad("Assets/Coin.png");
    l2_exitPortal.pTex = AEGfxTextureLoad("Assets/Exit.png");
    pMesh = CreateSquareMesh();
}


//----------------------------------------------------------------------------
// Initialises Level 2 state and entity positions
//----------------------------------------------------------------------------
void Level2_Initialize()
{
    std::cout << "Level2:Initialize\n";

    l2Power = {};
    l2_paused = false; l2_showWin = false; l2_showLose = false;
    l2_initialised = false;

    if (!l2_initialised)
    {
        float px = 0.0f, py = 0.0f;
        l2_player.size = GRID_TILE_SIZE;
        l2_mummy.size = GRID_TILE_SIZE;
        l2_gridStep = GRID_TILE_SIZE;
        l2_initialised = true;

        // Player - center left free cell
        L2FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
        l2_player.x = px; l2_player.y = py;
        l2_player.size = 50.0f;
        l2_player.r = 0.0f; l2_player.g = 0.0f; l2_player.b = 1.0f;

        // Mummy 1 - top-right corner, at least 10 cells from player
        int playerRow, playerCol;
        WorldToGrid(l2_player.x, l2_player.y, playerRow, playerCol);
        L2FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
        l2_mummy.x = px; l2_mummy.y = py;
        l2_mummy.size = 50.0f;
        l2_mummy.r = 1.0f; l2_mummy.g = 0.0f; l2_mummy.b = 0.0f;

        // Mummy 2 - bottom-center area, at least 10 cells from player
        L2FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
        l2_mummy2.x = px; l2_mummy2.y = py;
        l2_mummy2.size = 50.0f;
        l2_mummy2.r = 1.0f; l2_mummy2.g = 0.0f; l2_mummy2.b = 0.0f;

        // Exit portal - right side
        L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        l2_exitPortal.x = px; l2_exitPortal.y = py;
        l2_exitPortal.size = 40.0f;
        l2_exitPortal.r = 1.0f; l2_exitPortal.g = 1.0f; l2_exitPortal.b = 0.0f;

        // Coin - center
        L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
        l2_coin.x = px; l2_coin.y = py;
        l2_coin.size = 30.0f;
        l2_coin.r = 1.0f; l2_coin.g = 0.5f; l2_coin.b = 0.0f;

        l2_nextX = l2_player.x;
        l2_nextY = l2_player.y;
        l2_coinCounter = 0;
        l2_turnCounter = 0;
        l2_playerMoved = false;
    }
}


//----------------------------------------------------------------------------
// Updates Level 2 game logic each frame
//----------------------------------------------------------------------------
void Level2_Update()
{
    // Back / Quit
    if (AEInputCheckReleased(AEVK_B) || AEInputCheckReleased(AEVK_ESCAPE)) { next = LEVELPAGE; return; }
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) { next = GS_QUIT;   return; }

    // Win / Lose overlay input
    if (l2_showLose || l2_showWin)
    {
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);
        float mx = (float)mxS, my = (float)myS;

        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            if (IsAreaClicked(kL2BtnRetryX, kL2BtnRetryY, kL2BtnW, kL2BtnH, mx, my))
            {
                next = GS_LEVEL2;
                l2_showLose = l2_showWin = false;
                return;
            }
            if (IsAreaClicked(kL2BtnExitX, kL2BtnExitY, kL2BtnW, kL2BtnH, mx, my))
            {
                next = MAINMENUSTATE;
                l2_showLose = l2_showWin = false;
                return;
            }
        }
        if (AEInputCheckReleased(AEVK_R)) { next = GS_LEVEL2;  l2_showLose = l2_showWin = false; return; }
        if (AEInputCheckReleased(AEVK_RETURN)) { next = LEVELPAGE;   l2_showLose = l2_showWin = false; return; }
        if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) { next = GS_QUIT; return; }
        return;
    }

    // Pause toggle
    if (AEInputCheckReleased(AEVK_P)) { l2_paused = !l2_paused; }
    if (l2_paused) return;

    // --- Player movement ---
    float testX = l2_player.x;
    float testY = l2_player.y;

    if (AEInputCheckTriggered(AEVK_W)) testY += l2_gridStep;
    else if (AEInputCheckTriggered(AEVK_S)) testY -= l2_gridStep;
    else if (AEInputCheckTriggered(AEVK_A)) testX -= l2_gridStep;
    else if (AEInputCheckTriggered(AEVK_D)) testX += l2_gridStep;

    if (testX != l2_player.x || testY != l2_player.y)
    {
        if (IsTileWalkable(testX, testY))
        {
            l2_player.x = testX;
            l2_player.y = testY;
            l2_playerMoved = true;
        }
    }

    if (l2_playerMoved)
    {
        l2_turnCounter++;

        // Tile coin collection (value 4)
        int r, c;
        WorldToGrid(l2_player.x, l2_player.y, r, c);
        if (level[r][c] == 4)
        {
            level[r][c] = 0;
            l2_coinCounter++;
            std::cout << "L2 Coin collected! Total: " << l2_coinCounter << "\n";
        }

        // Mummy 1 moves every 2 player turns
        if (l2_turnCounter % 2 == 0)
        {
            float diffX = l2_player.x - l2_mummy.x;
            float diffY = l2_player.y - l2_mummy.y;

            if (fabsf(diffX) > 1.0f)
            {
                float stepX = (diffX > 0) ? l2_gridStep : -l2_gridStep;
                if (canMove(l2_mummy.x + stepX, l2_mummy.y)) l2_mummy.x += stepX;
            }
            diffY = l2_player.y - l2_mummy.y;
            if (fabsf(diffY) > 1.0f)
            {
                float stepY = (diffY > 0) ? l2_gridStep : -l2_gridStep;
                if (canMove(l2_mummy.x, l2_mummy.y + stepY)) l2_mummy.y += stepY;
            }

            // Mummy 2 moves every 2 player turns (same pace, different position)
            float diff2X = l2_player.x - l2_mummy2.x;
            float diff2Y = l2_player.y - l2_mummy2.y;

            if (fabsf(diff2X) > 1.0f)
            {
                float stepX = (diff2X > 0) ? l2_gridStep : -l2_gridStep;
                if (canMove(l2_mummy2.x + stepX, l2_mummy2.y)) l2_mummy2.x += stepX;
            }
            diff2Y = l2_player.y - l2_mummy2.y;
            if (fabsf(diff2Y) > 1.0f)
            {
                float stepY = (diff2Y > 0) ? l2_gridStep : -l2_gridStep;
                if (canMove(l2_mummy2.x, l2_mummy2.y + stepY)) l2_mummy2.y += stepY;
            }
        }

        L2TickPowers();
        l2_playerMoved = false;
    }

    // --- Lose check - only after the player has actually moved at least once ---
    if (l2_turnCounter > 0 && !L2IsInvincibleNow() &&
        ((fabsf(l2_player.x - l2_mummy.x) < 1.0f && fabsf(l2_player.y - l2_mummy.y) < 1.0f) ||
            (fabsf(l2_player.x - l2_mummy2.x) < 1.0f && fabsf(l2_player.y - l2_mummy2.y) < 1.0f)))
    {
        ResetLevel2();
        printf("L2: Caught by a Mummy!\n");
        l2_showLose = true;
    }

    // --- Win check ---
    if (fabsf(l2_player.x - l2_exitPortal.x) < 1.0f &&
        fabsf(l2_player.y - l2_exitPortal.y) < 1.0f)
    {
        printf("L2: You Escaped!\n");
        next = GS_WIN;
    }

    // --- Legacy coin entity collect ---
    if (fabsf(l2_player.x - l2_coin.x) < 1.0f &&
        fabsf(l2_player.y - l2_coin.y) < 1.0f)
    {
        ++l2_coinCounter;
        printf("L2 Coin! Total: %d\n", l2_coinCounter);
        l2_coin.x = 2000.0f; l2_coin.y = 2000.0f;
    }
}


//----------------------------------------------------------------------------
// Renders Level 2 each frame
//----------------------------------------------------------------------------
void Level2_Draw()
{
    AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);

    // Overlays - skip game rendering entirely when active
    if (l2_showLose) { LosePage_Draw();  return; }
    if (l2_showWin) { WinPage_Draw();   return; }
    if (l2_paused) { PausePage_Draw(); return; }

    AEMtx33 transform, scale, trans;

    // Floor - draw on every walkable (value == 0) cell
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(l2_FloorTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            if (level[row][col] == 0)
            {
                float x, y;
                GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    // Walls
    AEGfxTextureSet(l2_DesertBlockTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            if (level[row][col] == 1)
            {
                float x, y;
                GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    // Player
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(l2_player.pTex, 0, 0);
    AEMtx33Scale(&scale, l2_player.size, l2_player.size);
    AEMtx33Trans(&trans, l2_player.x, l2_player.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Mummy 1
    AEGfxTextureSet(l2_mummy.pTex, 0, 0);
    AEMtx33Scale(&scale, l2_mummy.size, l2_mummy.size);
    AEMtx33Trans(&trans, l2_mummy.x, l2_mummy.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Mummy 2
    AEGfxTextureSet(l2_mummy2.pTex, 0, 0);
    AEMtx33Scale(&scale, l2_mummy2.size, l2_mummy2.size);
    AEMtx33Trans(&trans, l2_mummy2.x, l2_mummy2.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Coin
    if (l2_coin.x < 1000.0f)
    {
        AEGfxTextureSet(l2_coin.pTex, 0, 0);
        AEMtx33Scale(&scale, l2_coin.size, l2_coin.size);
        AEMtx33Trans(&trans, l2_coin.x, l2_coin.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Exit portal
    AEGfxTextureSet(l2_exitPortal.pTex, 0, 0);
    AEMtx33Scale(&scale, l2_exitPortal.size, l2_exitPortal.size);
    AEMtx33Trans(&trans, l2_exitPortal.x, l2_exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Reset render state after drawing
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
}
//----------------------------------------------------------------------------
void Level2_Free()
{
    std::cout << "Level2:Free\n";
}


//----------------------------------------------------------------------------
// Unloads all Level 2 resources
//----------------------------------------------------------------------------
void Level2_Unload()
{
    std::cout << "Level2:Unload\n";
    AEGfxTextureUnload(l2_player.pTex);
    AEGfxTextureUnload(l2_DesertBlockTex);
    AEGfxTextureUnload(l2_FloorTex);
    AEGfxTextureUnload(l2_mummy.pTex);
    AEGfxTextureUnload(l2_mummy2.pTex);
    AEGfxTextureUnload(l2_coin.pTex);
    AEGfxTextureUnload(l2_exitPortal.pTex);
    if (pMesh) { AEGfxMeshFree(pMesh); pMesh = nullptr; }
    l2_initialised = false;
}