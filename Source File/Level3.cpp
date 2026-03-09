/* Start Header ************************************************************************/
/*!
\file   Level3.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  Level 3 -- Hard difficulty.
        Same structure as Level 2 but with THREE mummies chasing the player.
        Loads its tile map from "Assets/level3.txt".
        All L3-specific state is prefixed with "l3_" to avoid collisions.

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
static Entity l3_player;     // Player (explorer.png)
static Entity l3_mummy1;     // Mummy 1 -- spawns top-right
static Entity l3_mummy2;     // Mummy 2 -- spawns bottom-center
static Entity l3_mummy3;     // Mummy 3 -- spawns top-left (Level 3 exclusive)
static Entity l3_exitPortal; // Exit goal; reaching it triggers GS_WIN
static Entity l3_coin;       // Legacy single coin entity
static AEGfxTexture* l3_DesertBlockTex = nullptr; // Wall tile texture (DesertBlock.png)
static AEGfxTexture* l3_FloorTex = nullptr; // Floor tile texture (Floor.png)
static bool  l3_initialised = false; // Prevents double-initialization within a session
static int   l3_coinCounter = 0;     // Total coins collected
static int   l3_turnCounter = 0;     // Player move count; controls mummy move frequency
static bool  l3_playerMoved = false; // True when player made a valid move this frame
static float l3_gridStep = 50.0f; // World units per grid cell (= GRID_TILE_SIZE)
static float l3_nextX = 0.0f;  // Unused pending-move X (kept for parity with L1/L2)
static float l3_nextY = 0.0f;  // Unused pending-move Y

// ---- Overlay flags ----
static bool l3_paused = false; // P key toggles; freezes game logic when true
static bool l3_showWin = false; // Shown when player reaches exit portal
static bool l3_showLose = false; // Shown when any mummy catches the player

// ---- Retry/Exit button positions for Win/Lose overlays ----
static const float kL3BtnRetryX = -200.0f;
static const float kL3BtnRetryY = -130.0f;
static const float kL3BtnExitX = 200.0f;
static const float kL3BtnExitY = -130.0f;
static const float kL3BtnW = 280.0f;
static const float kL3BtnH = 90.0f;

// ---- Powerup state for Level 3 (mirrors Level 1/2 structure) ----
static struct L3PowerState {
    bool speed = false; int speedTurns = 0;
    bool freeze = false; int freezeTurns = 0;
    bool invincible = false; int invTurns = 0;
    int  invFrames = 0;
} l3Power;

// Returns true if the player is currently invincible (either turn- or frame-based).
static bool L3IsInvincibleNow() { return l3Power.invincible || (l3Power.invFrames > 0); }

// ----------------------------------------------------------------------------
// L3TickPowers
// Decrements all turn-based powerup counters by 1.  Call once per player move.
// ----------------------------------------------------------------------------
static void L3TickPowers()
{
    if (l3Power.speed && --l3Power.speedTurns <= 0) l3Power.speed = false;
    if (l3Power.freeze && --l3Power.freezeTurns <= 0) l3Power.freeze = false;
    if (l3Power.invincible && --l3Power.invTurns <= 0) l3Power.invincible = false;
}


// ----------------------------------------------------------------------------
// L3FindFreeSpawnCell
// Searches outward from (startRow, startCol) in expanding rings for the nearest
// empty (value == 0) cell at least 'minDist' Manhattan distance from
// (avoidRow, avoidCol).  Writes the world-space tile center to (outX, outY).
// Falls back to the start cell if no valid cell is found within maxRadius.
// ----------------------------------------------------------------------------
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
                if (abs(dr) != radius && abs(dc) != radius) continue; // outer ring only
                int r = startRow + dr;
                int c = startCol + dc;
                if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) continue;
                if (level[r][c] != 0) continue; // must be walkable
                if (avoidRow >= 0 && avoidCol >= 0)
                    if (abs(r - avoidRow) + abs(c - avoidCol) < minDist) continue;
                GridToWorldCenter(r, c, outX, outY);
                std::cout << "L3 Spawn at grid (" << r << "," << c << ")\n";
                return;
            }
        }
    }
    GridToWorldCenter(startRow, startCol, outX, outY);
    std::cout << "L3 Spawn fallback at grid (" << startRow << "," << startCol << ")\n";
}


// ----------------------------------------------------------------------------
// L3LoadLevelTxt  <-- THIS IS THE FUNCTION THAT READS LEVEL 3's FILE
// Opens "Assets/level3.txt" and fills the shared level[][] grid.
//
// File format: each cell written as <value>, rows separated by newlines.
// Tile values: 0=floor, 1=wall, 4=coin, etc. (same as Level 1 and 2).
//
// If the file cannot be opened, all cells are set to 0.
// ----------------------------------------------------------------------------
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


// ----------------------------------------------------------------------------
// MoveMummyTowardPlayer
// Moves a single mummy one step toward (playerX, playerY) using axis-priority
// greedy pathfinding: tries horizontal first, then vertical.
// canMove() validates each step against the tile grid.
// Extracted as a helper to avoid code duplication across 3 mummies.
// ----------------------------------------------------------------------------
static void MoveMummyTowardPlayer(Entity& mummy, float playerX, float playerY, float gridStep)
{
    float diffX = playerX - mummy.x;
    float diffY = playerY - mummy.y;

    // Horizontal step: close X gap first
    if (fabsf(diffX) > 1.0f)
    {
        float stepX = (diffX > 0) ? gridStep : -gridStep;
        if (canMove(mummy.x + stepX, mummy.y)) mummy.x += stepX;
    }
    // Vertical step: re-evaluate diffY after possible horizontal move
    diffY = playerY - mummy.y;
    if (fabsf(diffY) > 1.0f)
    {
        float stepY = (diffY > 0) ? gridStep : -gridStep;
        if (canMove(mummy.x, mummy.y + stepY)) mummy.y += stepY;
    }
}


// ----------------------------------------------------------------------------
// PlayerTouchesMummy
// Returns true if the player's world position coincides with the mummy's
// position (within 1 world unit).  Used for catch/lose detection.
// The <1.0f threshold accounts for floating-point imprecision on grid centers.
// ----------------------------------------------------------------------------
static bool PlayerTouchesMummy(const Entity& mummy, float playerX, float playerY)
{
    return fabsf(playerX - mummy.x) < 1.0f && fabsf(playerY - mummy.y) < 1.0f;
}


// ----------------------------------------------------------------------------
// ResetLevel3
// Repositions all Level 3 entities without reloading textures or the tile map.
// Called when any mummy catches the player (in-level reset, not full reload).
//
// Spawn layout:
//   - Player  : center-left (col 4)
//   - Mummy 1 : top-right,    min 10 cells from player
//   - Mummy 2 : bottom-center, min 10 cells from player
//   - Mummy 3 : top-left,     min 10 cells from player
//   - Coin    : grid center
// Also resets all counters and powerup state.
// ----------------------------------------------------------------------------
static void ResetLevel3()
{
    float px = 0.0f, py = 0.0f;

    L3FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
    l3_player.x = px; l3_player.y = py;

    int playerRow, playerCol;
    WorldToGrid(l3_player.x, l3_player.y, playerRow, playerCol);

    // Mummy 1 -- approaches from top-right
    L3FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
    l3_mummy1.x = px; l3_mummy1.y = py;

    // Mummy 2 -- approaches from bottom-center
    L3FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
    l3_mummy2.x = px; l3_mummy2.y = py;

    // Mummy 3 -- approaches from top-left (creates a triangular encirclement)
    L3FindFreeSpawnCell(2, 2, px, py, playerRow, playerCol, 10);
    l3_mummy3.x = px; l3_mummy3.y = py;

    L3FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
    l3_coin.x = px; l3_coin.y = py;

    l3_nextX = l3_player.x;
    l3_nextY = l3_player.y;
    l3_coinCounter = 0;
    l3_turnCounter = 0;
    l3_playerMoved = false;
    l3Power = {};
}


// ----------------------------------------------------------------------------
// Level3_Load
// Called once when entering Level 3.
// 1. Calls L3LoadLevelTxt() to populate level[][] from "Assets/level3.txt".
// 2. Loads textures for all entities (all 3 mummies share Enemy.png).
// 3. Creates the shared pMesh.
// ----------------------------------------------------------------------------
void Level3_Load()
{
    std::cout << "Level3:Load\n";

    // Load Level 3's tile map from disk into the shared level[][] grid
    L3LoadLevelTxt();

    // Load entity textures
    l3_player.pTex = AEGfxTextureLoad("Assets/explorer.png");
    l3_DesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    l3_FloorTex = AEGfxTextureLoad("Assets/Floor.png");
    l3_mummy1.pTex = AEGfxTextureLoad("Assets/Enemy.png"); // all 3 share the same texture
    l3_mummy2.pTex = AEGfxTextureLoad("Assets/Enemy.png");
    l3_mummy3.pTex = AEGfxTextureLoad("Assets/Enemy.png");
    l3_coin.pTex = AEGfxTextureLoad("Assets/Coin.png");
    l3_exitPortal.pTex = AEGfxTextureLoad("Assets/Exit.png");

    pMesh = CreateSquareMesh();
}


// ----------------------------------------------------------------------------
// Level3_Initialize
// Called once after Level3_Load and on every state re-entry.
// Resets overlays and powerups, then positions all entities:
//   - Player  : center-left free cell
//   - Mummy 1 : top-right,     min 10 cells from player
//   - Mummy 2 : bottom-center, min 10 cells from player
//   - Mummy 3 : top-left,      min 10 cells from player (third direction of threat)
//   - Exit    : center-right
//   - Coin    : grid center
// ----------------------------------------------------------------------------
void Level3_Initialize()
{
    std::cout << "Level3:Initialize\n";

    l3Power = {};
    l3_paused = false; l3_showWin = false; l3_showLose = false;
    l3_initialised = false; // Force full re-init every entry

    if (!l3_initialised)
    {
        float px = 0.0f, py = 0.0f;
        l3_player.size = GRID_TILE_SIZE;
        l3_mummy1.size = GRID_TILE_SIZE;
        l3_mummy2.size = GRID_TILE_SIZE;
        l3_mummy3.size = GRID_TILE_SIZE;
        l3_gridStep = GRID_TILE_SIZE;
        l3_initialised = true;

        // --- Player spawn ---
        L3FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
        l3_player.x = px; l3_player.y = py;
        l3_player.size = 50.0f;
        l3_player.r = 0.0f; l3_player.g = 0.0f; l3_player.b = 1.0f;

        int playerRow, playerCol;
        WorldToGrid(l3_player.x, l3_player.y, playerRow, playerCol);

        // --- Mummy 1: top-right corner, min 10 cells from player ---
        L3FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
        l3_mummy1.x = px; l3_mummy1.y = py;
        l3_mummy1.size = 50.0f;
        l3_mummy1.r = 1.0f; l3_mummy1.g = 0.0f; l3_mummy1.b = 0.0f;

        // --- Mummy 2: bottom-center, min 10 cells from player ---
        L3FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
        l3_mummy2.x = px; l3_mummy2.y = py;
        l3_mummy2.size = 50.0f;
        l3_mummy2.r = 1.0f; l3_mummy2.g = 0.0f; l3_mummy2.b = 0.0f;

        // --- Mummy 3: top-left corner, min 10 cells from player ---
        // Creates a triangular encirclement: top-right, bottom-center, top-left
        L3FindFreeSpawnCell(2, 2, px, py, playerRow, playerCol, 10);
        l3_mummy3.x = px; l3_mummy3.y = py;
        l3_mummy3.size = 50.0f;
        l3_mummy3.r = 1.0f; l3_mummy3.g = 0.0f; l3_mummy3.b = 0.0f;

        // --- Exit portal ---
        L3FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        l3_exitPortal.x = px; l3_exitPortal.y = py;
        l3_exitPortal.size = 40.0f;
        l3_exitPortal.r = 1.0f; l3_exitPortal.g = 1.0f; l3_exitPortal.b = 0.0f;

        // --- Coin ---
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


// ----------------------------------------------------------------------------
// Level3_Update
// Called every frame during the Level 3 game loop.
// Same structure as Level2_Update, but with three mummies.
// All 3 mummies move every 2nd player turn via MoveMummyTowardPlayer().
// Lose condition triggers if ANY of the 3 mummies occupies the player's cell.
// ----------------------------------------------------------------------------
void Level3_Update()
{
    // --- Navigation keys ---
    if (AEInputCheckReleased(AEVK_B) || AEInputCheckReleased(AEVK_ESCAPE)) { next = LEVELPAGE; return; }
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) { next = GS_QUIT;  return; }

    // --- Win / Lose overlay input ---
    if (l3_showLose || l3_showWin)
    {
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);
        float mx = (float)mxS, my = (float)myS;

        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            if (IsAreaClicked(kL3BtnRetryX, kL3BtnRetryY, kL3BtnW, kL3BtnH, mx, my))
            {
                next = GS_LEVEL3; l3_showLose = l3_showWin = false; return;
            }
            if (IsAreaClicked(kL3BtnExitX, kL3BtnExitY, kL3BtnW, kL3BtnH, mx, my))
            {
                next = MAINMENUSTATE; l3_showLose = l3_showWin = false; return;
            }
        }
        if (AEInputCheckReleased(AEVK_R)) { next = GS_LEVEL3;   l3_showLose = l3_showWin = false; return; }
        if (AEInputCheckReleased(AEVK_RETURN)) { next = LEVELPAGE;    l3_showLose = l3_showWin = false; return; }
        if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) { next = GS_QUIT; return; }
        return;
    }

    // --- Pause toggle ---
    if (AEInputCheckReleased(AEVK_P)) { l3_paused = !l3_paused; }
    if (l3_paused) return;

    // --- Player movement (WASD) ---
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

    // --- Per-turn logic ---
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

        // All 3 mummies advance every 2nd player turn
        // Using the shared MoveMummyTowardPlayer helper to avoid code duplication
        if (l3_turnCounter % 2 == 0)
        {
            MoveMummyTowardPlayer(l3_mummy1, l3_player.x, l3_player.y, l3_gridStep);
            MoveMummyTowardPlayer(l3_mummy2, l3_player.x, l3_player.y, l3_gridStep);
            MoveMummyTowardPlayer(l3_mummy3, l3_player.x, l3_player.y, l3_gridStep);
        }

        L3TickPowers();
        l3_playerMoved = false;
    }

    // --- Lose check: player caught by ANY of the 3 mummies ---
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

    // --- Legacy coin entity collect ---
    if (fabsf(l3_player.x - l3_coin.x) < 1.0f &&
        fabsf(l3_player.y - l3_coin.y) < 1.0f)
    {
        ++l3_coinCounter;
        printf("L3 Coin! Total: %d\n", l3_coinCounter);
        l3_coin.x = 2000.0f; l3_coin.y = 2000.0f; // "remove" by moving off-screen
    }
}


// ----------------------------------------------------------------------------
// Level3_Draw
// Called every frame to render Level 3.
// Rendering order (back to front):
//   1. Lose/Win/Pause overlay check -- delegate and return if active.
//   2. Floor tiles (value == 0).
//   3. Wall tiles  (value == 1).
//   4. Player, Mummy 1, Mummy 2, Mummy 3.
//   5. Coin entity (only while coin.x < 1000).
//   6. Exit portal.
// ----------------------------------------------------------------------------
void Level3_Draw()
{
    AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);

    if (l3_showLose) { LosePage_Draw();  return; }
    if (l3_showWin) { WinPage_Draw();   return; }
    if (l3_paused) { PausePage_Draw(); return; }

    AEMtx33 transform, scale, trans;

    // --- Floor tiles (value == 0) ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(l3_FloorTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
        for (int col = 0; col < GRID_COLS; col++)
            if (level[row][col] == 0)
            {
                float x, y; GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }

    // --- Wall tiles (value == 1) ---
    AEGfxTextureSet(l3_DesertBlockTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
        for (int col = 0; col < GRID_COLS; col++)
            if (level[row][col] == 1)
            {
                float x, y; GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }

    // --- Player (explorer.png) ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(l3_player.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_player.size, l3_player.size);
    AEMtx33Trans(&trans, l3_player.x, l3_player.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Mummy 1 ---
    AEGfxTextureSet(l3_mummy1.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_mummy1.size, l3_mummy1.size);
    AEMtx33Trans(&trans, l3_mummy1.x, l3_mummy1.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Mummy 2 ---
    AEGfxTextureSet(l3_mummy2.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_mummy2.size, l3_mummy2.size);
    AEMtx33Trans(&trans, l3_mummy2.x, l3_mummy2.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Mummy 3 ---
    AEGfxTextureSet(l3_mummy3.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_mummy3.size, l3_mummy3.size);
    AEMtx33Trans(&trans, l3_mummy3.x, l3_mummy3.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Coin (only while not collected) ---
    if (l3_coin.x < 1000.0f)
    {
        AEGfxTextureSet(l3_coin.pTex, 0, 0);
        AEMtx33Scale(&scale, l3_coin.size, l3_coin.size);
        AEMtx33Trans(&trans, l3_coin.x, l3_coin.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- Exit portal ---
    AEGfxTextureSet(l3_exitPortal.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_exitPortal.size, l3_exitPortal.size);
    AEMtx33Trans(&trans, l3_exitPortal.x, l3_exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Reset render state
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
}


// ----------------------------------------------------------------------------
// Level3_Free -- currently empty; no additional cleanup needed.
// ----------------------------------------------------------------------------
void Level3_Free()
{
    std::cout << "Level3:Free\n";
}


// ----------------------------------------------------------------------------
// Level3_Unload
// Unloads all Level 3 GPU textures and frees the shared mesh.
// Resets l3_initialised so Initialize runs fully on next entry.
// ----------------------------------------------------------------------------
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