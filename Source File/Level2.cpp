/* Start Header ****************************************************************
/*!
\file Level2.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par sharonjooai.lim@digipen.edu
\date January, 26, 2026
\brief Level 2 -- Medium difficulty.
 Same structure as Level 1 but with TWO mummies chasing the player.
 Loads its tile map from "Assets/level2.txt".
 All L2-specific state is prefixed with "l2_" to avoid collisions with Level1 globals.
Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************** */

#include "pch.h"

#include "leveleditor.hpp"
#include "GridUtils.h"
#include "Level1.h"
#include "Level2.h"
#include "gamestatemanager.h"
#include "GameStateList.h"
#include "Main.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdio>  // snprintf for HUD text -ths

// ===== ADDED: forward declaration so SpawnRandomPowerup() can be called
//              inside ResetLevel2() / Level2_Initialize() before its definition ===== -ths
static void SpawnRandomPowerup(); // -ths

// IsAreaClicked has no header declaration -- extern needed
extern bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height,
    float click_x, float click_y);

// ---- Level 2 local entities ----
// All prefixed l2_ so they don't conflict with Level1's globals.
static Entity l2_player; // Player entity (explorer.png)
static Entity l2_mummy;  // First mummy enemy
static Entity l2_mummy2; // Second mummy (Level 2 exclusive -- adds difficulty)
// ===== ADDED: one more different enemy -- Scorpion ===== -ths
static Entity l2_scorpion; // Scorpion enemy entity (scorpion.png) -ths
// ------------------------------------------------------- -ths
static Entity l2_exitPortal; // Exit goal; reaching it triggers GS_WIN
static Entity l2_coin;   // Legacy single coin entity
static AEGfxTexture* l2_DesertBlockTex = nullptr; // Wall tile texture
static AEGfxTexture* l2_FloorTex = nullptr;       // Floor tile texture
static bool l2_initialised = false; // Prevents double-initialization
static int  l2_coinCounter = 0;     // Total coins collected this session
static int  l2_turnCounter = 0;     // Player move count; controls mummy move frequency
static bool l2_playerMoved = false; // True when player made a valid move this frame
static float l2_gridStep = 50.0f;   // World units per grid cell (= GRID_TILE_SIZE)
static float l2_nextX = 0.0f;       // Unused pending-move X (kept for parity)
static float l2_nextY = 0.0f;       // Unused pending-move Y (kept for parity)

// ---- Overlay flags: set to true to show the respective full-screen overlay ----
static bool l2_paused = false;  // P key toggles; freezes game logic when true
static bool l2_showWin = false; // Shown when player reaches exit portal
static bool l2_showLose = false;// Shown when a mummy catches the player

// ---- Retry/Exit button positions for Win/Lose overlays ----
static const float kL2BtnRetryX = -200.0f;
static const float kL2BtnRetryY = -130.0f;
static const float kL2BtnExitX = 200.0f;
static const float kL2BtnExitY = -130.0f;
static const float kL2BtnW = 280.0f;
static const float kL2BtnH = 90.0f;

// ---- Powerup state for Level 2 ----
// Mirrors Level 1's gPower; all fields default to inactive.
static struct L2PowerState {
    bool speed = false; int speedTurns = 0; // Speed boost (turn-based)
    bool freeze = false; int freezeTurns = 0; // Enemy freeze (turn-based)
    bool invincible = false; int invTurns = 0; // Invincibility (turn-based)
    int invFrames = 0; // Invincibility (frame-based)
    // ===== ADDED: frame-based freeze so enemies freeze for real-time 3s ===== -ths
    int freezeFrames = 0; // decremented every Update frame (~180 frames @60 FPS) -ths
} l2Power;

// Returns true if the player is currently protected from any enemy contact.
static bool L2IsInvincibleNow() { return l2Power.invincible || (l2Power.invFrames > 0); }

// ----------------------------------------------------------------------------
// L2TickPowers
// Decrements all turn-based powerup counters by 1. Deactivates the powerup
// when its counter reaches zero. Call once per valid player move.
// ----------------------------------------------------------------------------
static void L2TickPowers()
{
    if (l2Power.speed && --l2Power.speedTurns <= 0) l2Power.speed = false;
    if (l2Power.freeze && --l2Power.freezeTurns <= 0) l2Power.freeze = false;
    if (l2Power.invincible && --l2Power.invTurns <= 0) l2Power.invincible = false;
}

// ===== ADDED: frame tickers for per-frame immunity/freeze counters ===== -ths
static void L2TickInvFrames() { if (l2Power.invFrames > 0) --l2Power.invFrames; } // -ths
static void L2TickFreezeFrames() { if (l2Power.freezeFrames > 0) --l2Power.freezeFrames; } // -ths

// ----------------------------------------------------------------------------
// L2FindFreeSpawnCell
// Searches outward from (startRow, startCol) in expanding square rings to find
// the nearest empty (value == 0) cell at least 'minDist' Manhattan distance
// from (avoidRow, avoidCol).
// Writes the world-space center of the found cell to (outX, outY).
// Falls back to the start cell if no valid cell is found within maxRadius.
// Used by Level2_Initialize and ResetLevel2 to place all entities safely.
// ----------------------------------------------------------------------------
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
                if (abs(dr) != radius && abs(dc) != radius) continue; // outer ring only
                int r = startRow + dr;
                int c = startCol + dc;
                if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) continue;
                if (level[r][c] != 0) continue; // skip walls and special tiles
                if (avoidRow >= 0 && avoidCol >= 0)
                    if (abs(r - avoidRow) + abs(c - avoidCol) < minDist) continue; // too close
                GridToWorldCenter(r, c, outX, outY);
                std::cout << "L2 Spawn at grid (" << r << "," << c << ")\n";
                return;
            }
        }
    }
    // Fallback: use start cell even if not ideal
    GridToWorldCenter(startRow, startCol, outX, outY);
    std::cout << "L2 Spawn fallback at grid (" << startRow << "," << startCol << ")\n";
}

// ----------------------------------------------------------------------------
// L2LoadLevelTxt <-- THIS IS THE FUNCTION THAT READS LEVEL 2's FILE
// Opens "Assets/level2.txt" and fills the shared level[][] grid.
//
// File format: each cell written as <value>, rows separated by newlines.
// Tile values: 0=floor, 1=wall, 4=coin, etc. (same as Level 1).
//
// If the file cannot be opened, all cells are set to 0 (open map).
// ----------------------------------------------------------------------------
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

// ===== ADDED: Reachability helpers to keep scorpion out of sealed pockets ===== -ths
static bool L2IsCellInBounds(int r, int c) {                      // -ths
    return r >= 0 && r < GRID_ROWS && c >= 0 && c < GRID_COLS;      // -ths
}                                                                  // -ths

// Return true if 'target (tr,tc)' is reachable from 'start (sr,sc)' via 4-neighbour floor (value==0). -ths
static bool L2IsReachable(int sr, int sc, int tr, int tc) {        // -ths
    if (!L2IsCellInBounds(sr, sc) || !L2IsCellInBounds(tr, tc))      // -ths
        return false;                                                  // -ths
    if (level[sr][sc] != 0 || level[tr][tc] != 0)                    // -ths
        return false;                                                  // -ths

    static int vis[GRID_ROWS][GRID_COLS];                            // -ths
    for (int i = 0; i < GRID_ROWS; ++i) for (int j = 0; j < GRID_COLS; ++j)      // -ths
        vis[i][j] = 0;                                                 // -ths

    struct Node { int r, c; };                                        // -ths
    Node q[GRID_ROWS * GRID_COLS];                                     // -ths
    int qb = 0, qe = 0;                                                  // -ths
    q[qe++] = { sr, sc };                                              // -ths
    vis[sr][sc] = 1;                                                 // -ths
    static int dr[4] = { -1, 1, 0, 0 };                                // -ths
    static int dc[4] = { 0, 0,-1, 1 };                                // -ths

    while (qb < qe) {                                                // -ths
        Node cur = q[qb++];                                            // -ths
        if (cur.r == tr && cur.c == tc) return true;                   // -ths
        for (int k = 0; k < 4; ++k) {                                        // -ths
            int nr = cur.r + dr[k], nc = cur.c + dc[k];                  // -ths
            if (!L2IsCellInBounds(nr, nc)) continue;                      // -ths
            if (vis[nr][nc]) continue;                                   // -ths
            if (level[nr][nc] != 0) continue;                            // -ths
            vis[nr][nc] = 1;                                             // -ths
            q[qe++] = { nr,nc };                                           // -ths
        }                                                               // -ths
    }                                                                 // -ths
    return false;                                                     // -ths
}                                                                   // -ths

// Prefer the given startRow/startCol; ensure the chosen free tile is reachable from the player. -ths
static void L2FindReachableSpawnNear(int startRow, int startCol,   // -ths
    int avoidRow, int avoidCol,   // -ths
    int minDist,                  // -ths
    float& outX, float& outY,     // -ths
    int playerRow, int playerCol) // -ths
{                                                                   // -ths
  // 1) Try the regular near-start search first.                     // -ths
    float tx, ty;                                                     // -ths
    L2FindFreeSpawnCell(startRow, startCol, tx, ty, avoidRow, avoidCol, minDist); // -ths
    int tr, tc; WorldToGrid(tx, ty, tr, tc);                          // -ths
    if (L2IsCellInBounds(tr, tc) && level[tr][tc] == 0 &&                // -ths
        L2IsReachable(playerRow, playerCol, tr, tc)) {                // -ths
        outX = tx; outY = ty;                                           // -ths
        return;                                                         // -ths
    }                                                                 // -ths

    // 2) Scan a neighbourhood around (startRow,startCol) for a reachable free tile. // -ths
    int maxRadius = 10;                                               // -ths
    for (int radius = 0; radius <= maxRadius; ++radius) {                 // -ths
        for (int dr = -radius; dr <= radius; ++dr) {                    // -ths
            for (int dc = -radius; dc <= radius; ++dc) {                  // -ths
                if (abs(dr) != radius && abs(dc) != radius) continue;           // -ths
                int r = startRow + dr, c = startCol + dc;                   // -ths
                if (!L2IsCellInBounds(r, c)) continue;                       // -ths
                if (level[r][c] != 0) continue;                             // -ths
                // respect minDist from avoid cell (usually the player)      // -ths
                if (avoidRow >= 0 && avoidCol >= 0) {                       // -ths
                    if (abs(r - avoidRow) + abs(c - avoidCol) < minDist)      // -ths
                        continue;                                               // -ths
                }                                                           // -ths
                if (L2IsReachable(playerRow, playerCol, r, c)) {            // -ths
                    GridToWorldCenter(r, c, outX, outY);                      // -ths
                    return;                                                   // -ths
                }                                                           // -ths
            }                                                             // -ths
        }                                                               // -ths
    }                                                                 // -ths

    // 3) Fallback: pick the first reachable free tile anywhere.       // -ths
    for (int r = 0; r < GRID_ROWS; ++r)                                   // -ths
        for (int c = 0; c < GRID_COLS; ++c)                                 // -ths
            if (level[r][c] == 0 && L2IsReachable(playerRow, playerCol, r, c)) // -ths
            {
                GridToWorldCenter(r, c, outX, outY); return;
            }                 // -ths

// 4) Last resort: just return the player's tile (almost never triggered). // -ths
    GridToWorldCenter(playerRow, playerCol, outX, outY);              // -ths
}                                                                   // -ths

// ----------------------------------------------------------------------------
// ResetLevel2
// Repositions all Level 2 entities to safe spawn cells without reloading
// textures or the tile map. Called when a mummy catches the player.
//
// Spawn layout:
// - Player : center-left (col 4)
// - Mummy 1 : top-right corner, min 10 cells from player
// - Mummy 2 : bottom-center, min 10 cells from player
// - Scorpion : bottom-left, min 8 cells from player (new extra enemy)
// - Coin : grid center
// Also resets all counters and powerup state.
// ----------------------------------------------------------------------------
static void ResetLevel2()
{
    float px = 0.0f, py = 0.0f;
    // Player spawn: center-left area
    L2FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
    l2_player.x = px; l2_player.y = py;
    int playerRow, playerCol;
    WorldToGrid(l2_player.x, l2_player.y, playerRow, playerCol);
    // Mummy 1 spawn: top-right, at least 10 cells from player
    L2FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
    l2_mummy.x = px; l2_mummy.y = py;
    // Mummy 2 spawn: bottom-center, at least 10 cells from player (different angle of approach)
    L2FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
    l2_mummy2.x = px; l2_mummy2.y = py;

    // ===== REPLACED: Scorpion spawn with reachability-aware helper ===== -ths
    L2FindReachableSpawnNear(GRID_ROWS - 4, 2,      // preferred bottom-left area -ths
        playerRow, playerCol,  // avoid near player           -ths
        8,                     // min Manhattan distance      -ths
        px, py,                // out world coordinates       -ths
        playerRow, playerCol); // for reachability check      -ths
    l2_scorpion.x = px; l2_scorpion.y = py; l2_scorpion.size = 50.0f;              // -ths

    // Coin spawn: grid center
    L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
    l2_coin.x = px; l2_coin.y = py;
    l2_nextX = l2_player.x;
    l2_nextY = l2_player.y;
    l2_coinCounter = 0;
    l2_turnCounter = 0;
    l2_playerMoved = false;
    l2Power = {}; // clear all powerup state

    // ===== ADDED: reset frame-based freeze; respawn a power‑up ===== -ths
    l2Power.freezeFrames = 0; // -ths
    SpawnRandomPowerup();      // -ths
}

// ===== ADDED: random power‑up data & helpers ========================================== -ths
enum L2PowerupType { L2_PWR_IMMUNE = 0, L2_PWR_FREEZE = 1 };        // -ths
static Entity l2_powerup;                                           // -ths
static bool   l2_powerupActive = false;                             // -ths
static int    l2_powerupType = L2_PWR_IMMUNE;                      // -ths
static AEGfxTexture* l2_ImmuneTex = nullptr; // Assets/Immune.png    // -ths
static AEGfxTexture* l2_FreezeTex = nullptr; // Assets/Freeze.png    // -ths

static int L2RandInt(int mn, int mx)                                // -ths
{
    float t = AERandFloat(); // [0..1]                                  // -ths
    int span = (mx - mn + 1);                                          // -ths
    return mn + (int)(t * (float)span);                                // -ths
}

static void SpawnRandomPowerup()                                    // -ths
{
    l2_powerupType = (AERandFloat() < 0.5f) ? L2_PWR_IMMUNE : L2_PWR_FREEZE; // -ths
    for (int tries = 0; tries < 128; ++tries)                                  // -ths
    {
        int r = L2RandInt(0, GRID_ROWS - 1);                                        // -ths
        int c = L2RandInt(0, GRID_COLS - 1);                                        // -ths
        if (level[r][c] == 0)                                                     // -ths
        {
            float x, y; GridToWorldCenter(r, c, x, y);                               // -ths
            l2_powerup.x = x; l2_powerup.y = y; l2_powerup.size = 30.0f;             // -ths
            l2_powerupActive = true;                                                 // -ths
            return;                                                                  // -ths
        }
    }
    l2_powerupActive = false; // fallback -ths
}

// ===== ADDED: Scorpion texture handle (loaded in Level2_Load) ===== -ths
static AEGfxTexture* l2_ScorpionTex = nullptr; // Assets/scorpion.png -ths

// ----------------------------------------------------------------------------
// Level2_Load
// Called once when entering Level 2.
// 1. Calls L2LoadLevelTxt() to populate level[][] from "Assets/level2.txt".
// 2. Loads all textures: player, wall, floor, both mummies, coin, exit portal.
// 3. Creates the shared pMesh.
// ----------------------------------------------------------------------------
void Level2_Load()
{
    std::cout << "Level2:Load\n";
    // Load Level 2's tile map from disk into the shared level[][] grid
    L2LoadLevelTxt();

    // Load entity textures
    l2_player.pTex = AEGfxTextureLoad("Assets/explorer.png"); // player sprite
    l2_DesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png"); // wall tile
    l2_FloorTex = AEGfxTextureLoad("Assets/Floor.png"); // floor tile
    l2_mummy.pTex = AEGfxTextureLoad("Assets/Enemy.png"); // mummy 1
    l2_mummy2.pTex = AEGfxTextureLoad("Assets/Enemy.png"); // mummy 2 (same texture)
    l2_coin.pTex = AEGfxTextureLoad("Assets/Coin.png"); // coin
    l2_exitPortal.pTex = AEGfxTextureLoad("Assets/DoorClosed.png"); // exit portal

    // ===== ADDED: load power‑up textures ===== -ths
    l2_ImmuneTex = AEGfxTextureLoad("Assets/Immune.png"); // -ths
    l2_FreezeTex = AEGfxTextureLoad("Assets/Freeze.png"); // -ths

    // ===== ADDED: load scorpion texture ===== -ths
    l2_ScorpionTex = AEGfxTextureLoad("Assets/scorpion.png"); // -ths

    pMesh = CreateSquareMesh(); // unit square mesh shared by all sprites
}

// ----------------------------------------------------------------------------
// Level2_Initialize
// Called once after Level2_Load and on every state re-entry.
// Resets overlays and powerups, then positions all entities:
// - Player : center-left free cell
// - Mummy 1 : top-right, min 10 cells from player
// - Mummy 2 : bottom-center, min 10 cells from player
// - Scorpion : bottom-left, min 8 cells from player (new enemy)
// - Exit : center-right
// - Coin : grid center
// ----------------------------------------------------------------------------
void Level2_Initialize()
{
    std::cout << "Level2:Initialize\n";
    // Always reset powerup and overlay state on entry
    l2Power = {};
    l2_paused = false; l2_showWin = l2_showLose = false;
    l2_initialised = false; // Force full re-init every time

    // ===== ADDED: clear frame-based freeze ===== -ths
    l2Power.freezeFrames = 0; // -ths

    if (!l2_initialised)
    {
        float px = 0.0f, py = 0.0f;
        l2_player.size = GRID_TILE_SIZE;
        l2_mummy.size = GRID_TILE_SIZE;
        l2_gridStep = GRID_TILE_SIZE;
        l2_initialised = true;

        // --- Player spawn ---
        L2FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
        l2_player.x = px; l2_player.y = py;
        l2_player.size = 50.0f;
        l2_player.r = 0.0f; l2_player.g = 0.0f; l2_player.b = 1.0f;
        int playerRow, playerCol;
        WorldToGrid(l2_player.x, l2_player.y, playerRow, playerCol);

        // --- Mummy 1 spawn: top-right, min 10 cells from player ---
        L2FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
        l2_mummy.x = px; l2_mummy.y = py;
        l2_mummy.size = 50.0f;
        l2_mummy.r = 1.0f; l2_mummy.g = 0.0f; l2_mummy.b = 0.0f;

        // --- Mummy 2 spawn: bottom-center, min 10 cells from player ---
        // Starts from a different corner to approach the player from a different angle
        L2FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
        l2_mummy2.x = px; l2_mummy2.y = py;
        l2_mummy2.size = 50.0f;
        l2_mummy2.r = 1.0f; l2_mummy2.g = 0.0f; l2_mummy2.b = 0.0f;

        // ===== REPLACED: Scorpion spawn with reachability-aware helper ===== -ths
        L2FindReachableSpawnNear(GRID_ROWS - 4, 2,      // preferred bottom-left area -ths
            playerRow, playerCol,  // avoid near player           -ths
            8,                     // min Manhattan distance      -ths
            px, py,                // out world coordinates       -ths
            playerRow, playerCol); // for reachability check      -ths
        l2_scorpion.x = px; l2_scorpion.y = py;         // -ths
        l2_scorpion.size = 50.0f;                       // -ths
        l2_scorpion.pTex = l2_ScorpionTex;              // -ths

        // --- Exit portal spawn: right side ---
        L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        l2_exitPortal.x = px; l2_exitPortal.y = py;
        l2_exitPortal.size = 50.0f;
        l2_exitPortal.r = 1.0f; l2_exitPortal.g = 1.0f; l2_exitPortal.b = 0.0f;

        // --- Coin spawn: grid center ---
        L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
        l2_coin.x = px; l2_coin.y = py;
        l2_coin.size = 30.0f;
        l2_coin.r = 1.0f; l2_coin.g = 0.5f; l2_coin.b = 0.0f;

        // ===== ADDED: spawn a random power‑up ===== -ths
        SpawnRandomPowerup(); // -ths

        l2_nextX = l2_player.x;
        l2_nextY = l2_player.y;
        l2_coinCounter = 0;
        l2_turnCounter = 0;
        l2_playerMoved = false;
    }
}

// ----------------------------------------------------------------------------
// Level2_Update
// Called every frame during the Level 2 game loop.
// Logic is identical to Level1_Update with two differences:
// - Two mummies (l2_mummy and l2_mummy2) both chase the player.
// - Lose condition triggers if EITHER mummy occupies the player's cell.
//
// Order of operations each frame:
// 1. Back/Quit key handling.
// 2. Win/Lose overlay input (Retry / Exit buttons, R, ENTER, Q).
// 3. Pause toggle (P); returns early when paused.
// 4. WASD player movement validated by IsTileWalkable().
// 5. Per-turn logic (on playerMoved):
//    a. Tile coin collection (value 4).
//    b. Both mummies move every 2nd turn using axis-priority greedy chase.
//    c. L2TickPowers() -- decrement powerup durations.
// 6. Lose check: player shares cell with either mummy AND not invincible.
// 7. Win check: player reaches exit portal cell.
// 8. Legacy coin entity collect.
// ----------------------------------------------------------------------------
void Level2_Update()
{
    // --- Navigation keys ---
    if (AEInputCheckReleased(AEVK_B) ||
        AEInputCheckReleased(AEVK_ESCAPE)) {
        next = LEVELPAGE; return;
    }
    if (AEInputCheckReleased(AEVK_Q) ||
        0 == AESysDoesWindowExist()) {
        next = GS_QUIT; return;
    }

    // ===== ADDED: per-frame power timers (immunity & freeze) ===== -ths
    L2TickInvFrames();     // -ths
    L2TickFreezeFrames();  // -ths

    // --- Win / Lose overlay input ---
    if (l2_showLose ||
        l2_showWin)
    {
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);
        float mx = (float)mxS, my = (float)myS;
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            // "Retry" button: restart Level 2
            if (IsAreaClicked(kL2BtnRetryX, kL2BtnRetryY, kL2BtnW, kL2BtnH, mx, my))
            {
                next = GS_LEVEL2;
                l2_showLose = l2_showWin = false;
                return;
            }
            // "Exit" button: return to main menu
            if (IsAreaClicked(kL2BtnExitX, kL2BtnExitY, kL2BtnW, kL2BtnH, mx, my))
            {
                next = MAINMENUSTATE;
                l2_showLose = l2_showWin = false;
                return;
            }
        }
        if (AEInputCheckReleased(AEVK_R)) { next = GS_LEVEL2; l2_showLose = l2_showWin = false; return; }
        if (AEInputCheckReleased(AEVK_RETURN)) { next = LEVELPAGE; l2_showLose = l2_showWin = false; return; }
        if (AEInputCheckReleased(AEVK_Q) ||
            0 == AESysDoesWindowExist()) {
            next = GS_QUIT; return;
        }
        return; // freeze game while overlay is visible
    }

    // --- Pause toggle ---
    if (AEInputCheckReleased(AEVK_P)) { l2_paused = !l2_paused; }
    if (l2_paused) return;

    // --- Player movement (WASD) ---
    float testX = l2_player.x;
    float testY = l2_player.y;
    if (AEInputCheckTriggered(AEVK_W)) testY += l2_gridStep;
    else if (AEInputCheckTriggered(AEVK_S)) testY -= l2_gridStep;
    else if (AEInputCheckTriggered(AEVK_A)) testX -= l2_gridStep;
    else if (AEInputCheckTriggered(AEVK_D)) testX += l2_gridStep;

    // Validate candidate position against the tile grid
    if (testX != l2_player.x ||
        testY != l2_player.y)
    {
        if (IsTileWalkable(testX, testY))
        {
            l2_player.x = testX;
            l2_player.y = testY;
            l2_playerMoved = true;
        }
    }

    // ===== ADDED: power‑up pickup (pre‑turn) ===== -ths
    if (l2_powerupActive &&
        fabsf(l2_player.x - l2_powerup.x) < 1.0f &&
        fabsf(l2_player.y - l2_powerup.y) < 1.0f)
    {
        if (l2_powerupType == L2_PWR_IMMUNE)  l2Power.invFrames = 300; // ~5s -ths
        else                                   l2Power.freezeFrames = 180; // ~3s -ths
        l2_powerupActive = false; l2_powerup.x = l2_powerup.y = 2000.0f;   // off-screen -ths
    }

    // --- Per-turn logic ---
    if (l2_playerMoved)
    {
        l2_turnCounter++;

        // Tile coin collection: tile value 4 = COIN
        int r, c;
        WorldToGrid(l2_player.x, l2_player.y, r, c);
        if (level[r][c] == 4)
        {
            level[r][c] = 0; // remove tile so it can only be collected once
            l2_coinCounter++;
            std::cout << "L2 Coin collected! Total: " << l2_coinCounter << "\n";
        }

        // Both mummies move every 2nd player turn (axis-priority greedy chase)
        // ===== ADDED: skip movement while freezeFrames > 0 ===== -ths
        if (l2_turnCounter % 2 == 0 && l2Power.freezeFrames <= 0)
        {
            // --- Mummy 1 movement ---
            float diffX = l2_player.x - l2_mummy.x;
            float diffY = l2_player.y - l2_mummy.y;
            if (fabsf(diffX) > 1.0f) // try horizontal step first
            {
                float stepX = (diffX > 0) ? l2_gridStep : -l2_gridStep;
                if (canMove(l2_mummy.x + stepX, l2_mummy.y)) l2_mummy.x += stepX;
            }
            diffY = l2_player.y - l2_mummy.y; // re-evaluate after possible horizontal move
            if (fabsf(diffY) > 1.0f) // then try vertical step
            {
                float stepY = (diffY > 0) ? l2_gridStep : -l2_gridStep;
                if (canMove(l2_mummy.x, l2_mummy.y + stepY)) l2_mummy.y += stepY;
            }
            // --- Mummy 2 movement (same logic, different entity) ---
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

            // ===== ADDED: Scorpion movement (same axis-priority greedy chase) ===== -ths
            float sdX = l2_player.x - l2_scorpion.x;
            float sdY = l2_player.y - l2_scorpion.y;
            if (fabsf(sdX) > 1.0f)
            {
                float sx = (sdX > 0) ? l2_gridStep : -l2_gridStep;
                if (canMove(l2_scorpion.x + sx, l2_scorpion.y)) l2_scorpion.x += sx;
            }
            sdY = l2_player.y - l2_scorpion.y;
            if (fabsf(sdY) > 1.0f)
            {
                float sy = (sdY > 0) ? l2_gridStep : -l2_gridStep;
                if (canMove(l2_scorpion.x, l2_scorpion.y + sy)) l2_scorpion.y += sy;
            }
        }

        L2TickPowers();
        l2_playerMoved = false;
    }

    // --- Lose check: player caught by either mummy OR scorpion (new) ---
    // Only triggers after the player has moved (prevents false positive at spawn overlap)
    if (l2_turnCounter > 0 && !L2IsInvincibleNow() &&
        ((fabsf(l2_player.x - l2_mummy.x) < 1.0f && fabsf(l2_player.y - l2_mummy.y) < 1.0f) ||
            (fabsf(l2_player.x - l2_mummy2.x) < 1.0f && fabsf(l2_player.y - l2_mummy2.y) < 1.0f) ||
            (fabsf(l2_player.x - l2_scorpion.x) < 1.0f && fabsf(l2_player.y - l2_scorpion.y) < 1.0f))) // -ths
    {
        ResetLevel2();
        printf("L2: Caught by an Enemy!\n"); // generic line; scorpion included -ths
        l2_showLose = true;
    }

    // --- Win check: player reached the exit portal ---
    if (fabsf(l2_player.x - l2_exitPortal.x) < 1.0f &&
        fabsf(l2_player.y - l2_exitPortal.y) < 1.0f)
    {
        printf("L2: You Escaped!\n");
        next = GS_WIN; // Transition to the win page
    }

    // --- Legacy coin entity collect (moves coin off-screen when touched) ---
    if (fabsf(l2_player.x - l2_coin.x) < 1.0f &&
        fabsf(l2_player.y - l2_coin.y) < 1.0f)
    {
        ++l2_coinCounter;
        printf("L2 Coin! Total: %d\n", l2_coinCounter);
        l2_coin.x = 2000.0f; l2_coin.y = 2000.0f; // "remove" by moving off-screen
    }
}

// ----------------------------------------------------------------------------
// Level2_Draw
// Called every frame to render Level 2.
// Rendering order (back to front):
// 1. Overlay check: if lose/win/pause overlay active, delegate and return.
// 2. Floor tiles: all value==0 cells drawn with l2_FloorTex.
// 3. Wall tiles: all value==1 cells drawn with l2_DesertBlockTex.
// 4. Player, Mummy 1, Mummy 2, Scorpion (all using textures).
// 5. Coin entity (only drawn when coin.x < 1000).
// 6. Power-up icon (if active).
// 7. Exit portal texture.
// All sprites use the shared pMesh scaled by a TRS matrix.
// ----------------------------------------------------------------------------
void Level2_Draw()
{
    AEGfxSetBackgroundColor(0.22f, 0.14f, 0.09f);
    // Delegate rendering to overlay draw functions when active
    if (l2_showLose) { LosePage_Draw(); return; }
    if (l2_showWin) { WinPage_Draw();  return; }
    if (l2_paused) { PausePage_Draw(); return; }

    AEMtx33 transform, scale, trans;

    // --- Floor tiles (value == 0) ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(l2_FloorTex, 0, 0);
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
    AEGfxTextureSet(l2_DesertBlockTex, 0, 0);
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
    AEGfxTextureSet(l2_player.pTex, 0, 0);
    AEMtx33Scale(&scale, l2_player.size, l2_player.size);
    AEMtx33Trans(&trans, l2_player.x, l2_player.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Mummy 1 (Enemy.png) ---
    AEGfxTextureSet(l2_mummy.pTex, 0, 0);
    AEMtx33Scale(&scale, l2_mummy.size, l2_mummy.size);
    AEMtx33Trans(&trans, l2_mummy.x, l2_mummy.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Mummy 2 (Enemy.png) ---
    AEGfxTextureSet(l2_mummy2.pTex, 0, 0);
    AEMtx33Scale(&scale, l2_mummy2.size, l2_mummy2.size);
    AEMtx33Trans(&trans, l2_mummy2.x, l2_mummy2.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // ===== ADDED: Scorpion (scorpion.png) ===== -ths
    AEGfxTextureSet(l2_ScorpionTex, 0, 0);             // -ths
    AEMtx33Scale(&scale, l2_scorpion.size, l2_scorpion.size); // -ths
    AEMtx33Trans(&trans, l2_scorpion.x, l2_scorpion.y);       // -ths
    AEMtx33Concat(&transform, &trans, &scale);                // -ths
    AEGfxSetTransform(transform.m);                           // -ths
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);               // -ths

    // --- Coin (only rendered while not collected) ---
    if (l2_coin.x < 1000.0f)
    {
        AEGfxTextureSet(l2_coin.pTex, 0, 0);
        AEMtx33Scale(&scale, l2_coin.size, l2_coin.size);
        AEMtx33Trans(&trans, l2_coin.x, l2_coin.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // ===== ADDED: render power‑up if active ===== -ths
    if (l2_powerupActive)
    {
        AEGfxTextureSet((l2_powerupType == L2_PWR_IMMUNE) ? l2_ImmuneTex : l2_FreezeTex, 0, 0); // -ths
        AEMtx33Scale(&scale, l2_powerup.size, l2_powerup.size);  // -ths
        AEMtx33Trans(&trans, l2_powerup.x, l2_powerup.y);        // -ths
        AEMtx33Concat(&transform, &trans, &scale);               // -ths
        AEGfxSetTransform(transform.m);                          // -ths
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);              // -ths
    }

    // --- Exit portal (Exit.png) ---
    AEGfxTextureSet(l2_exitPortal.pTex, 0, 0);
    AEMtx33Scale(&scale, l2_exitPortal.size, l2_exitPortal.size);
    AEMtx33Trans(&trans, l2_exitPortal.x, l2_exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // ===== ADDED: HUD for active power-ups (top-left) ===== -ths
    if (l2Power.invFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "IMMUNE  %.1fs", l2Power.invFrames / 60.0f); // ~60 FPS -ths
        AEGfxPrint(fontId, buf, -0.95f, 0.90f, 0.8f, 0.90f, 0.90f, 0.20f, 1.0f);     // yellow-ish -ths
    }
    if (l2Power.freezeFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "FREEZE  %.1fs", l2Power.freezeFrames / 60.0f); // ~60 FPS -ths
        AEGfxPrint(fontId, buf, -0.95f, 0.82f, 0.8f, 0.60f, 0.85f, 1.00f, 1.0f);        // cyan-ish -ths
    }
    // ===== END ADDED HUD ===== -ths

    // Reset render state to clean defaults
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
}

// ----------------------------------------------------------------------------
// Level2_Free
// Called after the game loop exits Level 2. Currently empty.
// ----------------------------------------------------------------------------
void Level2_Free()
{
    std::cout << "Level2:Free\n";
}

// ----------------------------------------------------------------------------
// Level2_Unload
// Unloads all Level 2 GPU textures and frees the shared mesh.
// Resets l2_initialised so Initialize runs fully on next entry.
// ----------------------------------------------------------------------------
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

    // ===== ADDED: unload power‑up & scorpion textures ===== -ths
    AEGfxTextureUnload(l2_ImmuneTex);   // -ths
    AEGfxTextureUnload(l2_FreezeTex);   // -ths
    AEGfxTextureUnload(l2_ScorpionTex); // -ths

    if (pMesh) { AEGfxMeshFree(pMesh); pMesh = nullptr; }
    l2_initialised = false;
}
