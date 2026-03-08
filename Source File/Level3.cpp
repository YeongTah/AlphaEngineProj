/* Start Header ************************************************************************/
/*!
\file   Level3.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  This file defines the function Load, Initialize, Update, Draw, Free, Unload
        to produce level 3 in the game, loading from Assets/level3.txt.
        Level 3 has 3 mummies for increased difficulty.

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
#include "Level3.h"
#include "gamestatemanager.h"
#include "GameStateList.h"
#include "Main.h"
#include <iostream>
#include <fstream>
#include <cmath>

// IsAreaClicked has no header declaration
extern bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height,
    float click_x, float click_y);

// ---- Level 3 local entities ----
static Entity l3_player;
static Entity l3_mummy1;
static Entity l3_mummy2;
static Entity l3_mummy3; // 3 mummies for level 3
static Entity l3_exitPortal;
static Entity l3_coin;
static AEGfxTexture* l3_DesertBlockTex = nullptr;
static AEGfxTexture* l3_FloorTex = nullptr;
static bool  l3_initialised = false;
static int   l3_coinCounter = 0;
static int   l3_turnCounter = 0;
static bool  l3_playerMoved = false;
static float l3_gridStep = 50.0f;
static float l3_nextX = 0.0f;
static float l3_nextY = 0.0f;

// ---- overlay flags ----
static bool l3_paused = false;
static bool l3_showWin = false;
static bool l3_showLose = false;

// ---- overlay button layout ----
static const float kL3BtnRetryX = -200.0f;
static const float kL3BtnRetryY = -130.0f;
static const float kL3BtnExitX = 200.0f;
static const float kL3BtnExitY = -130.0f;
static const float kL3BtnW = 280.0f;
static const float kL3BtnH = 90.0f;

// ---- powerup state ----
static struct L3PowerState {
    bool speed = false;      int speedTurns = 0;
    bool freeze = false;     int freezeTurns = 0;
    bool invincible = false; int invTurns = 0;
    int  invFrames = 0;
} l3Power;

static bool L3IsInvincibleNow() { return l3Power.invincible || (l3Power.invFrames > 0); }
static void L3TickPowers()
{
    if (l3Power.speed && --l3Power.speedTurns <= 0) l3Power.speed = false;
    if (l3Power.freeze && --l3Power.freezeTurns <= 0) l3Power.freeze = false;
    if (l3Power.invincible && --l3Power.invTurns <= 0) l3Power.invincible = false;
}


//----------------------------------------------------------------------------
// Finds nearest free (value==0) grid cell, avoiding minDist from (avoidRow,avoidCol).
//----------------------------------------------------------------------------
static void L3FindFreeSpawnCell(int startRow, int startCol, float& outX, float& outY,
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
                std::cout << "L3 Spawn at grid (" << r << "," << c << ")\n";
                return;
            }
        }
    }
    GridToWorldCenter(startRow, startCol, outX, outY);
    std::cout << "L3 Spawn fallback at grid (" << startRow << "," << startCol << ")\n";
}


//----------------------------------------------------------------------------
// Reads Assets/level3.txt into the shared level[][] grid.
//----------------------------------------------------------------------------
static void L3LoadLevelTxt()
{
    const char* path = "Assets/level3.txt";
    std::ifstream is(path);
    if (!is.is_open())
    {
        std::cout << "Level3: Could not open " << path << " - grid all zeros\n";
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
    std::cout << "Level3: Loaded grid from " << path << "\n";
}


//----------------------------------------------------------------------------
// Helper: move one mummy one step toward the player each turn.
//----------------------------------------------------------------------------
static void MoveMummyTowardPlayer(Entity& mummy, float playerX, float playerY, float gridStep)
{
    float diffX = playerX - mummy.x;
    float diffY = playerY - mummy.y;

    if (fabsf(diffX) > 1.0f)
    {
        float stepX = (diffX > 0) ? gridStep : -gridStep;
        if (canMove(mummy.x + stepX, mummy.y)) mummy.x += stepX;
    }
    diffY = playerY - mummy.y;
    if (fabsf(diffY) > 1.0f)
    {
        float stepY = (diffY > 0) ? gridStep : -gridStep;
        if (canMove(mummy.x, mummy.y + stepY)) mummy.y += stepY;
    }
}


//----------------------------------------------------------------------------
// Helper: check if player touches this mummy.
//----------------------------------------------------------------------------
static bool PlayerTouchesMummy(const Entity& mummy, float playerX, float playerY)
{
    return fabsf(playerX - mummy.x) < 1.0f && fabsf(playerY - mummy.y) < 1.0f;
}


//----------------------------------------------------------------------------
// Resets all Level 3 positions to safe free cells.
//----------------------------------------------------------------------------
static void ResetLevel3()
{
    float px = 0.0f, py = 0.0f;

    // Player - center left
    L3FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
    l3_player.x = px; l3_player.y = py;

    int playerRow, playerCol;
    WorldToGrid(l3_player.x, l3_player.y, playerRow, playerCol);

    // Mummy 1 - top-right
    L3FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
    l3_mummy1.x = px; l3_mummy1.y = py;

    // Mummy 2 - bottom-center
    L3FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
    l3_mummy2.x = px; l3_mummy2.y = py;

    // Mummy 3 - top-left
    L3FindFreeSpawnCell(2, 2, px, py, playerRow, playerCol, 10);
    l3_mummy3.x = px; l3_mummy3.y = py;

    // Coin - center
    L3FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
    l3_coin.x = px; l3_coin.y = py;

    l3_nextX = l3_player.x;
    l3_nextY = l3_player.y;
    l3_coinCounter = 0;
    l3_turnCounter = 0;
    l3_playerMoved = false;
    l3Power = {};
}


//----------------------------------------------------------------------------
// Loads Level 3 resources and tile map
//----------------------------------------------------------------------------
void Level3_Load()
{
    std::cout << "Level3:Load\n";

    L3LoadLevelTxt();

    l3_player.pTex = AEGfxTextureLoad("Assets/explorer.png");
    l3_DesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    l3_FloorTex = AEGfxTextureLoad("Assets/Floor.png");
    l3_mummy1.pTex = AEGfxTextureLoad("Assets/Enemy.png");
    l3_mummy2.pTex = AEGfxTextureLoad("Assets/Enemy.png");
    l3_mummy3.pTex = AEGfxTextureLoad("Assets/Enemy.png");
    l3_coin.pTex = AEGfxTextureLoad("Assets/Coin.png");
    l3_exitPortal.pTex = AEGfxTextureLoad("Assets/Exit.png");
    pMesh = CreateSquareMesh();
}


//----------------------------------------------------------------------------
// Initialises Level 3 state and entity positions
//----------------------------------------------------------------------------
void Level3_Initialize()
{
    std::cout << "Level3:Initialize\n";

    l3Power = {};
    l3_paused = false; l3_showWin = false; l3_showLose = false;
    l3_initialised = false;

    if (!l3_initialised)
    {
        float px = 0.0f, py = 0.0f;
        l3_player.size = GRID_TILE_SIZE;
        l3_mummy1.size = GRID_TILE_SIZE;
        l3_mummy2.size = GRID_TILE_SIZE;
        l3_mummy3.size = GRID_TILE_SIZE;
        l3_gridStep = GRID_TILE_SIZE;
        l3_initialised = true;

        // Player - center left free cell
        L3FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
        l3_player.x = px; l3_player.y = py;
        l3_player.size = 50.0f;
        l3_player.r = 0.0f; l3_player.g = 0.0f; l3_player.b = 1.0f;

        int playerRow, playerCol;
        WorldToGrid(l3_player.x, l3_player.y, playerRow, playerCol);

        // Mummy 1 - top-right, at least 10 cells from player
        L3FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
        l3_mummy1.x = px; l3_mummy1.y = py;
        l3_mummy1.size = 50.0f;
        l3_mummy1.r = 1.0f; l3_mummy1.g = 0.0f; l3_mummy1.b = 0.0f;

        // Mummy 2 - bottom-center, at least 10 cells from player
        L3FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
        l3_mummy2.x = px; l3_mummy2.y = py;
        l3_mummy2.size = 50.0f;
        l3_mummy2.r = 1.0f; l3_mummy2.g = 0.0f; l3_mummy2.b = 0.0f;

        // Mummy 3 - top-left, at least 10 cells from player
        L3FindFreeSpawnCell(2, 2, px, py, playerRow, playerCol, 10);
        l3_mummy3.x = px; l3_mummy3.y = py;
        l3_mummy3.size = 50.0f;
        l3_mummy3.r = 1.0f; l3_mummy3.g = 0.0f; l3_mummy3.b = 0.0f;

        // Exit portal - right side
        L3FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        l3_exitPortal.x = px; l3_exitPortal.y = py;
        l3_exitPortal.size = 40.0f;
        l3_exitPortal.r = 1.0f; l3_exitPortal.g = 1.0f; l3_exitPortal.b = 0.0f;

        // Coin - center
        L3FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
        l3_coin.x = px; l3_coin.y = py;
        l3_coin.size = 30.0f;
        l3_coin.r = 1.0f; l3_coin.g = 0.5f; l3_coin.b = 0.0f;

        l3_nextX = l3_player.x;
        l3_nextY = l3_player.y;
        l3_coinCounter = 0;
        l3_turnCounter = 0;
        l3_playerMoved = false;
    }
}


//----------------------------------------------------------------------------
// Updates Level 3 game logic each frame
//----------------------------------------------------------------------------
void Level3_Update()
{
    // Back / Quit
    if (AEInputCheckReleased(AEVK_B) || AEInputCheckReleased(AEVK_ESCAPE)) { next = LEVELPAGE; return; }
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) { next = GS_QUIT;   return; }

    // Win / Lose overlay input
    if (l3_showLose || l3_showWin)
    {
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);
        float mx = (float)mxS, my = (float)myS;

        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            if (IsAreaClicked(kL3BtnRetryX, kL3BtnRetryY, kL3BtnW, kL3BtnH, mx, my))
            {
                next = GS_LEVEL3;
                l3_showLose = l3_showWin = false;
                return;
            }
            if (IsAreaClicked(kL3BtnExitX, kL3BtnExitY, kL3BtnW, kL3BtnH, mx, my))
            {
                next = MAINMENUSTATE;
                l3_showLose = l3_showWin = false;
                return;
            }
        }
        if (AEInputCheckReleased(AEVK_R)) { next = GS_LEVEL3;  l3_showLose = l3_showWin = false; return; }
        if (AEInputCheckReleased(AEVK_RETURN)) { next = LEVELPAGE;   l3_showLose = l3_showWin = false; return; }
        if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) { next = GS_QUIT; return; }
        return;
    }

    // Pause toggle
    if (AEInputCheckReleased(AEVK_P)) { l3_paused = !l3_paused; }
    if (l3_paused) return;

    // --- Player movement ---
    float testX = l3_player.x;
    float testY = l3_player.y;

    if (AEInputCheckTriggered(AEVK_W)) testY += l3_gridStep;
    else if (AEInputCheckTriggered(AEVK_S)) testY -= l3_gridStep;
    else if (AEInputCheckTriggered(AEVK_A)) testX -= l3_gridStep;
    else if (AEInputCheckTriggered(AEVK_D)) testX += l3_gridStep;

    if (testX != l3_player.x || testY != l3_player.y)
    {
        if (IsTileWalkable(testX, testY))
        {
            l3_player.x = testX;
            l3_player.y = testY;
            l3_playerMoved = true;
        }
    }

    if (l3_playerMoved)
    {
        l3_turnCounter++;

        // Tile coin collection (value 4)
        int r, c;
        WorldToGrid(l3_player.x, l3_player.y, r, c);
        if (level[r][c] == 4)
        {
            level[r][c] = 0;
            l3_coinCounter++;
            std::cout << "L3 Coin collected! Total: " << l3_coinCounter << "\n";
        }

        // All 3 mummies move every 2 player turns
        if (l3_turnCounter % 2 == 0)
        {
            MoveMummyTowardPlayer(l3_mummy1, l3_player.x, l3_player.y, l3_gridStep);
            MoveMummyTowardPlayer(l3_mummy2, l3_player.x, l3_player.y, l3_gridStep);
            MoveMummyTowardPlayer(l3_mummy3, l3_player.x, l3_player.y, l3_gridStep);
        }

        L3TickPowers();
        l3_playerMoved = false;
    }

    // --- Lose check - only after the player has actually moved at least once ---
    if (l3_turnCounter > 0 && !L3IsInvincibleNow() &&
        (PlayerTouchesMummy(l3_mummy1, l3_player.x, l3_player.y) ||
            PlayerTouchesMummy(l3_mummy2, l3_player.x, l3_player.y) ||
            PlayerTouchesMummy(l3_mummy3, l3_player.x, l3_player.y)))
    {
        ResetLevel3();
        printf("L3: Caught by a Mummy!\n");
        l3_showLose = true;
    }

    // --- Win check ---
    if (fabsf(l3_player.x - l3_exitPortal.x) < 1.0f &&
        fabsf(l3_player.y - l3_exitPortal.y) < 1.0f)
    {
        printf("L3: You Escaped!\n");
        next = GS_WIN;
    }

    // --- Coin entity collect ---
    if (fabsf(l3_player.x - l3_coin.x) < 1.0f &&
        fabsf(l3_player.y - l3_coin.y) < 1.0f)
    {
        ++l3_coinCounter;
        printf("L3 Coin! Total: %d\n", l3_coinCounter);
        l3_coin.x = 2000.0f; l3_coin.y = 2000.0f;
    }
}


//----------------------------------------------------------------------------
// Renders Level 3 each frame
//----------------------------------------------------------------------------
void Level3_Draw()
{
    AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);

    // Overlays - skip game rendering entirely when active
    if (l3_showLose) { LosePage_Draw();  return; }
    if (l3_showWin) { WinPage_Draw();   return; }
    if (l3_paused) { PausePage_Draw(); return; }

    AEMtx33 transform, scale, trans;

    // Floor - draw on every walkable (value == 0) cell
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(l3_FloorTex, 0, 0);
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
    AEGfxTextureSet(l3_DesertBlockTex, 0, 0);
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
    AEGfxTextureSet(l3_player.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_player.size, l3_player.size);
    AEMtx33Trans(&trans, l3_player.x, l3_player.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Mummy 1
    AEGfxTextureSet(l3_mummy1.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_mummy1.size, l3_mummy1.size);
    AEMtx33Trans(&trans, l3_mummy1.x, l3_mummy1.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Mummy 2
    AEGfxTextureSet(l3_mummy2.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_mummy2.size, l3_mummy2.size);
    AEMtx33Trans(&trans, l3_mummy2.x, l3_mummy2.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Mummy 3
    AEGfxTextureSet(l3_mummy3.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_mummy3.size, l3_mummy3.size);
    AEMtx33Trans(&trans, l3_mummy3.x, l3_mummy3.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Coin
    if (l3_coin.x < 1000.0f)
    {
        AEGfxTextureSet(l3_coin.pTex, 0, 0);
        AEMtx33Scale(&scale, l3_coin.size, l3_coin.size);
        AEMtx33Trans(&trans, l3_coin.x, l3_coin.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Exit portal
    AEGfxTextureSet(l3_exitPortal.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_exitPortal.size, l3_exitPortal.size);
    AEMtx33Trans(&trans, l3_exitPortal.x, l3_exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Reset render state after drawing
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
}


//----------------------------------------------------------------------------
// Cleans up dynamic resources
//----------------------------------------------------------------------------
void Level3_Free()
{
    std::cout << "Level3:Free\n";
}


//----------------------------------------------------------------------------
// Unloads all Level 3 resources
//----------------------------------------------------------------------------
void Level3_Unload()
{
    std::cout << "Level3:Unload\n";
    AEGfxTextureUnload(l3_player.pTex);
    AEGfxTextureUnload(l3_DesertBlockTex);
    AEGfxTextureUnload(l3_FloorTex);
    AEGfxTextureUnload(l3_mummy1.pTex);
    AEGfxTextureUnload(l3_mummy2.pTex);
    AEGfxTextureUnload(l3_mummy3.pTex);
    AEGfxTextureUnload(l3_coin.pTex);
    AEGfxTextureUnload(l3_exitPortal.pTex);
    if (pMesh) { AEGfxMeshFree(pMesh); pMesh = nullptr; }
    l3_initialised = false;
}