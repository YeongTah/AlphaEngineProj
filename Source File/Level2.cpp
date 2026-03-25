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
#include "JumpScare.h"
#include "gamestatemanager.h"
#include "GameStateList.h"
#include "Main.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdio>  // snprintf for HUD text -ths

// ======================= LEVEL 2 AUDIO HANDLES ======================= // -ths

static AEAudio l2_sfxPlayerMove;  // -ths
static AEAudio l2_sfxChest;        // -ths
static AEAudio l2_sfxPowerup;      // -ths
static AEAudio l2_sfxJumpscare;    // -ths
static AEAudio l2_sfxExitDoor;     // -ths
static AEAudio l2_sfxGameOver;     // -ths
static AEAudio l2_sfxButton;       // -ths

static AEAudioGroup l2AudioGroup;  // -ths
// ===================================================================== // -ths

// ===== ADDED: forward declaration so SpawnRandomPowerup() can be called
//              inside ResetLevel2() / Level2_Initialize() before its definition ===== -ths
static void SpawnRandomPowerup(); // -ths
static void L2SpawnTreasureBox(); // forward decl for use in Reset/Initialize

// ========================== TREASURE BOX SYSTEM (Level 2) ==========================
// Identical behaviour to Level 1: player touches the chest -> 50% coin, 50% mummy.
// Box mummies are stored separately from the main enemies in l2_boxMummies[].
// All names are prefixed l2_ to avoid conflicts with Level 1 globals.
// -----------------------------------------------------------------------------------
static Entity        l2_treasureBox;
static bool          l2_treasureBoxActive = false;
static AEGfxTexture* l2_treasureBoxTex = nullptr; // Assets/TreasureChest.png

struct L2BoxMummy { float x, y, size; };
static L2BoxMummy l2_boxMummies[8];
static int        l2_boxMummyCount = 0;

static char l2_popupMsg[64] = "";
static int  l2_popupFrames = 0;

// IsAreaClicked has no header declaration -- extern needed
extern bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height,
    s32 click_x, s32 click_y);

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

    // Reset treasure box (one-time chest — re-spawn it for the new round)
    l2_boxMummyCount = 0;
    L2SpawnTreasureBox();

    // Clear any lingering popup
    l2_popupFrames = 0;
    l2_popupMsg[0] = '\0';
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

// ----------------------------------------------------------------------------
// L2SpawnTreasureBox
// Places the treasure box at a random free cell at least 3 Manhattan cells
// away from the player. Re-called after each box is opened.
// ----------------------------------------------------------------------------
static void L2SpawnTreasureBox()
{
    for (int tries = 0; tries < 256; ++tries)
    {
        int r = L2RandInt(0, GRID_ROWS - 1);
        int c = L2RandInt(0, GRID_COLS - 1);
        if (level[r][c] != 0) continue; // must be walkable

        int pr, pc;
        WorldToGrid(l2_player.x, l2_player.y, pr, pc);
        if (abs(r - pr) + abs(c - pc) < 3) continue; // too close to player

        float wx, wy;
        GridToWorldCenter(r, c, wx, wy);
        l2_treasureBox.x = wx;
        l2_treasureBox.y = wy;
        l2_treasureBox.size = l2_gridStep * 0.9f;
        l2_treasureBoxActive = true;
        printf("L2 TreasureBox spawned at grid (%d,%d) | world (%.1f,%.1f)\n", r, c, wx, wy);
        return;
    }
    // Fallback: no distance constraint
    for (int tries = 0; tries < 256; ++tries)
    {
        int r = L2RandInt(0, GRID_ROWS - 1);
        int c = L2RandInt(0, GRID_COLS - 1);
        if (level[r][c] != 0) continue;
        float wx, wy;
        GridToWorldCenter(r, c, wx, wy);
        l2_treasureBox.x = wx;
        l2_treasureBox.y = wy;
        l2_treasureBox.size = l2_gridStep * 0.9f;
        l2_treasureBoxActive = true;
        printf("L2 TreasureBox spawned (fallback) at grid (%d,%d)\n", r, c);
        return;
    }
    l2_treasureBoxActive = false;
}

// ----------------------------------------------------------------------------
// L2OpenTreasureBox
// Called when the player steps onto the treasure box.
// 50% -> +1 coin.   50% -> spawns a new chasing box mummy near the chest.
// The chest disappears permanently after opening (no re-spawn).
// ----------------------------------------------------------------------------
static void L2OpenTreasureBox()
{
    l2_treasureBoxActive = false; // chest is gone for good -- no re-spawn

    if (AEAudioIsValidAudio(l2_sfxChest))
        AEAudioPlay(l2_sfxChest, l2AudioGroup, 1.0f, 1.0f, 0);

    bool spawnMummy = (AERandFloat() >= 0.5f);

    if (!spawnMummy)
    {
        l2_coinCounter++;
        printf("L2 Treasure Box: COIN! Total: %d\n", l2_coinCounter);
        std::snprintf(l2_popupMsg, sizeof(l2_popupMsg), "Treasure: +1 Coin! (Total: %d)", l2_coinCounter);
        l2_popupFrames = 180;
    }
    else
    {
        JumpScare_Trigger();
        if (AEAudioIsValidAudio(l2_sfxJumpscare))
            AEAudioPlay(l2_sfxJumpscare, l2AudioGroup, 1.0f, 1.0f, 0);

        if (l2_boxMummyCount < (int)(sizeof(l2_boxMummies) / sizeof(l2_boxMummies[0])))
        {
            float sx, sy;
            int br, bc;
            WorldToGrid(l2_treasureBox.x, l2_treasureBox.y, br, bc);
            int pr, pc;
            WorldToGrid(l2_player.x, l2_player.y, pr, pc);
            L2FindFreeSpawnCell(br, bc, sx, sy, pr, pc, 2);

            L2BoxMummy& bm = l2_boxMummies[l2_boxMummyCount++];
            bm.x = sx; bm.y = sy; bm.size = l2_gridStep;
            printf("L2 Treasure Box: MUMMY spawned at (%.0f,%.0f)!\n", sx, sy);
            std::snprintf(l2_popupMsg, sizeof(l2_popupMsg), "Treasure: A Mummy appeared!");
            l2_popupFrames = 180;
        }
    }
    // NOTE: L2SpawnTreasureBox() intentionally NOT called here -- chest appears once only.
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

    // ================================================================
    // AUDIO LOAD FOR LEVEL 2                                          -ths
    // ================================================================
    l2AudioGroup = AEAudioCreateGroup();                        // -ths

    l2_sfxPlayerMove = AEAudioLoadSound("Assets/audio/player.wav");   // -ths
    l2_sfxChest = AEAudioLoadSound("Assets/audio/chest.wav");    // -ths
    l2_sfxPowerup = AEAudioLoadSound("Assets/audio/powerup.wav");  // -ths
    l2_sfxJumpscare = AEAudioLoadSound("Assets/audio/jumpscare.wav");// -ths
    l2_sfxExitDoor = AEAudioLoadSound("Assets/audio/exit.wav");     // -ths
    l2_sfxGameOver = AEAudioLoadSound("Assets/audio/gameover.wav"); // -ths
    l2_sfxButton = AEAudioLoadSound("Assets/audio/button.wav");   // -ths

    // (NO main menu music here)                                       // -ths
    // ================================================================ // -ths


    // Load Level 2's tile map from disk into the shared level[][] grid
    L2LoadLevelTxt();

    // Load entity textures
    l2_player.pTex = AEGfxTextureLoad("Assets/explorer.png");          // player sprite
    l2_DesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");    // wall tile
    l2_FloorTex = AEGfxTextureLoad("Assets/Floor.png");                // floor tile
    l2_mummy.pTex = AEGfxTextureLoad("Assets/Enemy.png");              // mummy 1
    l2_mummy2.pTex = AEGfxTextureLoad("Assets/Enemy.png");             // mummy 2 (same texture)
    l2_coin.pTex = AEGfxTextureLoad("Assets/Coin.png");                // coin
    l2_exitPortal.pTex = AEGfxTextureLoad("Assets/DoorClosed.png");    // exit portal

    // ===== ADDED: load power‑up textures ===== -ths
    l2_ImmuneTex = AEGfxTextureLoad("Assets/Immune.png");              // -ths
    l2_FreezeTex = AEGfxTextureLoad("Assets/Freeze.png");              // -ths

    // ===== ADDED: load scorpion texture ===== -ths
    l2_ScorpionTex = AEGfxTextureLoad("Assets/Spider.png");          // -ths

    // Load treasure box texture
    l2_treasureBoxTex = AEGfxTextureLoad("Assets/TreasureChest.png");

    // Load jump scare texture
    JumpScare_Load();

    // Init treasure box state (actual spawn happens in Initialize)
    l2_boxMummyCount = 0;
    l2_treasureBoxActive = false;

    pMesh = CreateSquareMesh();                                        // unit square mesh shared by sprites
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

    // ================================================================
    // STOP ALL PREVIOUS AUDIO (safe, no FMOD crash)                 -ths
    // ================================================================
    AEAudioStopGroup(l2AudioGroup);
    // ================================================================ // -ths

    // Always reset powerup and overlay state on entry
    l2Power = {};
    l2_paused = false;
    l2_showWin = l2_showLose = false;
    l2_initialised = false; // Force full re-init every time

    // ===== ADDED: clear frame-based freeze ===== -ths
    l2Power.freezeFrames = 0; // -ths

    // Initialise jump scare
    JumpScare_Init();

    if (!l2_initialised)
    {
        float px = 0.0f, py = 0.0f;
        l2_player.size = GRID_TILE_SIZE;
        l2_mummy.size = GRID_TILE_SIZE;
        l2_gridStep = GRID_TILE_SIZE;
        l2_initialised = true;

        // --- Player spawn ---
        L2FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
        l2_player.x = px;
        l2_player.y = py;
        l2_player.size = 50.0f;
        l2_player.r = 0.0f;
        l2_player.g = 0.0f;
        l2_player.b = 1.0f;

        int playerRow, playerCol;
        WorldToGrid(l2_player.x, l2_player.y, playerRow, playerCol);

        // --- Mummy 1 spawn ---
        L2FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
        l2_mummy.x = px;
        l2_mummy.y = py;
        l2_mummy.size = 50.0f;
        l2_mummy.r = 1.0f;
        l2_mummy.g = 0.0f;
        l2_mummy.b = 0.0f;

        // --- Mummy 2 spawn ---
        L2FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
        l2_mummy2.x = px;
        l2_mummy2.y = py;
        l2_mummy2.size = 50.0f;
        l2_mummy2.r = 1.0f;
        l2_mummy2.g = 0.0f;
        l2_mummy2.b = 0.0f;

        // ===== Scorpion spawn (reachability-aware) ===== -ths
        L2FindReachableSpawnNear(
            GRID_ROWS - 4, 2,
            playerRow, playerCol,
            8,
            px, py,
            playerRow, playerCol
        ); // -ths

        l2_scorpion.x = px;
        l2_scorpion.y = py;  // -ths
        l2_scorpion.size = 50.0f;     // -ths
        l2_scorpion.pTex = l2_ScorpionTex;  // -ths

        // --- Exit portal spawn ---
        L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        l2_exitPortal.x = px;
        l2_exitPortal.y = py;
        l2_exitPortal.size = 50.0f;

        // --- Coin spawn ---
        L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
        l2_coin.x = px;
        l2_coin.y = py;
        l2_coin.size = 30.0f;
        l2_coin.r = 1.0f;
        l2_coin.g = 0.5f;
        l2_coin.b = 0.0f;

        // ===== Spawn random power‑up ===== -ths
        SpawnRandomPowerup(); // -ths

        // Spawn treasure box (one per round)
        l2_boxMummyCount = 0;
        L2SpawnTreasureBox();

        l2_nextX = l2_player.x;
        l2_nextY = l2_player.y;
        l2_coinCounter = 0;
        l2_turnCounter = 0;
        l2_playerMoved = false;
    }
}

// ============================================================================
// DrawPauseButton - draws a gray rectangle and "PAUSE" text at top-right -ths
// ============================================================================

static void DrawPauseButton()
{
    // Draw the button background rectangle -ths
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.2f, 0.2f, 0.2f, 1.0f);  // dark gray

    AEMtx33 s, t, m;
    AEMtx33Scale(&s, 80.0f, 40.0f);       // button size
    AEMtx33Trans(&t, 750.0f, 420.0f);     // button center (world)
    AEMtx33Concat(&m, &t, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // ---- Center the "PAUSE" text inside the button ----
    const float textScale = 0.8f;
    const char* text = "PAUSE";

    // Get text dimensions in NDC
    float w, h;
    AEGfxGetPrintSize(fontId, text, textScale, &w, &h);

    // Convert button center to NDC
    float centerNDCX = 750.0f / 800.0f;   // 0.9375
    float centerNDCY = 420.0f / 450.0f;   // 0.93333...

    // Left X for centered text = centerNDCX - w/2
    float leftX = centerNDCX - w * 0.5f;

    // Print text centered over the button
    AEGfxPrint(fontId, text, leftX, centerNDCY, textScale, 1, 1, 1, 1);
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
// Jump Scare is also shown.
// 7. Win check: player reaches exit portal cell.
// 8. Legacy coin entity collect.
// ----------------------------------------------------------------------------
void Level2_Update()
{
    // --- Navigation keys ---
    if (AEInputCheckReleased(AEVK_B)) {
        next = LEVELPAGE; return;
    }
    if (AEInputCheckReleased(AEVK_ESCAPE) ||
        0 == AESysDoesWindowExist()) {
        next = GS_QUIT; return;
    }

    // ===== PER-FRAME POWER TIMERS ===== -ths
    L2TickInvFrames();     // -ths
    L2TickFreezeFrames();  // -ths

    // --- Win / Lose overlay input ---
    if (l2_showLose || l2_showWin)
    {
        s32 mxS, myS;
        TransformScreentoWorld(mxS, myS);

        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            // Retry
            if (IsAreaClicked(kL2BtnRetryX, kL2BtnRetryY, kL2BtnW, kL2BtnH, myS, myS))
            {
                // play button click -ths
                if (AEAudioIsValidAudio(l2_sfxButton))
                    AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0); // -ths

                next = GS_LEVEL2;
                l2_showLose = l2_showWin = false;
                return;
            }
            // Exit
            if (IsAreaClicked(kL2BtnExitX, kL2BtnExitY, kL2BtnW, kL2BtnH, mxS, myS))
            {
                // play button click -ths
                if (AEAudioIsValidAudio(l2_sfxButton))
                    AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0); // -ths

                next = MAINMENUSTATE;
                l2_showLose = l2_showWin = false;
                return;
            }
        }

        if (AEInputCheckReleased(AEVK_R))
        {
            next = GS_LEVEL2;
            l2_showLose = l2_showWin = false;
            return;
        }
        if (AEInputCheckReleased(AEVK_RETURN))
        {
            next = LEVELPAGE;
            l2_showLose = l2_showWin = false;
            return;
        }
        if (AEInputCheckReleased(AEVK_ESCAPE) ||
            0 == AESysDoesWindowExist())
        {
            next = GS_QUIT;
            return;
        }

        return;
    }

    // =========================================================================
    // ADDED: PAUSE BUTTON CLICK DETECTION (top-right corner) -ths
    // =========================================================================
    {

        // detect click inside pause rectangle -ths
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            s32 mxS, myS;
            TransformScreentoWorld(mxS, myS);

            if (IsAreaClicked(750.0f, 420.0f, 80.0f, 40.0f, mxS, myS)) // top-right pause button -ths
            {
                next = GS_PAUSE;   // go to PausePage -ths
                return;
            }
        }
    }
    // =========================================================================

    // --- Pause toggle ---
    if (AEInputCheckReleased(AEVK_P)) { l2_paused = !l2_paused; }
    if (l2_paused) return;

    // --- Player movement ---
    float testX = l2_player.x;
    float testY = l2_player.y;

    bool attemptedMove = false; // -ths

    if (AEInputCheckTriggered(AEVK_W)) { testY += l2_gridStep; attemptedMove = true; }
    else if (AEInputCheckTriggered(AEVK_S)) { testY -= l2_gridStep; attemptedMove = true; }
    else if (AEInputCheckTriggered(AEVK_A)) { testX -= l2_gridStep; attemptedMove = true; }
    else if (AEInputCheckTriggered(AEVK_D)) { testX += l2_gridStep; attemptedMove = true; }

    if (attemptedMove && IsTileWalkable(testX, testY))
    {
        l2_player.x = testX;
        l2_player.y = testY;
        l2_playerMoved = true;

        // ===== PLAY MOVEMENT AUDIO ===== -ths
        if (AEAudioIsValidAudio(l2_sfxPlayerMove))
            AEAudioPlay(l2_sfxPlayerMove, l2AudioGroup, 1.0f, 1.0f, 0); // -ths
    }

    // ===== POWER-UP PICKUP ===== -ths
    if (l2_powerupActive &&
        fabsf(l2_player.x - l2_powerup.x) < 1.0f &&
        fabsf(l2_player.y - l2_powerup.y) < 1.0f)
    {
        // play powerup audio -ths
        if (AEAudioIsValidAudio(l2_sfxPowerup))
            AEAudioPlay(l2_sfxPowerup, l2AudioGroup, 1.0f, 1.0f, 0); // -ths

        if (l2_powerupType == L2_PWR_IMMUNE)
            l2Power.invFrames = 300; // 5s
        else
            l2Power.freezeFrames = 180; // 3s

        l2_powerupActive = false;
        l2_powerup.x = l2_powerup.y = 2000.0f;
    }

    // Popup countdown
    if (l2_popupFrames > 0) --l2_popupFrames;

    // ===== TREASURE BOX TOUCH =====
    if (l2_treasureBoxActive &&
        fabsf(l2_player.x - l2_treasureBox.x) < 1.0f &&
        fabsf(l2_player.y - l2_treasureBox.y) < 1.0f)
    {
        L2OpenTreasureBox();
    }

    // --- Per-turn logic ---
    if (l2_playerMoved)
    {
        l2_turnCounter++;

        // Tile coin collection
        int r, c;
        WorldToGrid(l2_player.x, l2_player.y, r, c);
        if (level[r][c] == 4)
        {
            level[r][c] = 0;
            l2_coinCounter++;
            std::cout << "L2 Coin collected! Total: " << l2_coinCounter << "\n";
        }

        // Mummy & Scorpion movement (frozen if freezeFrames > 0)
        if (l2_turnCounter % 2 == 0 && l2Power.freezeFrames <= 0)
        {
            // Helper lambda: returns true if (nx, ny) is already occupied by another enemy
            // Enemies: l2_mummy, l2_mummy2, l2_scorpion
            auto enemyOccupies = [](float nx, float ny, float skipX, float skipY) -> bool {
                // Check each of the three enemies, skipping the one that is moving (skipX/skipY)
                float ex[] = { l2_mummy.x,  l2_mummy2.x,  l2_scorpion.x };
                float ey[] = { l2_mummy.y,  l2_mummy2.y,  l2_scorpion.y };
                for (int k = 0; k < 3; ++k)
                    if (!(fabsf(ex[k] - skipX) < 1.0f && fabsf(ey[k] - skipY) < 1.0f))
                        if (fabsf(ex[k] - nx) < 1.0f && fabsf(ey[k] - ny) < 1.0f)
                            return true;
                return false;
                };

            // ========== MUMMY 1 MOVEMENT ========== -ths
            float diffX = l2_player.x - l2_mummy.x;
            float diffY = l2_player.y - l2_mummy.y;

            // horizontal first
            if (fabsf(diffX) > 1.0f)
            {
                float stepX = (diffX > 0) ? l2_gridStep : -l2_gridStep;
                float nx = l2_mummy.x + stepX, ny = l2_mummy.y;
                if (!enemyOccupies(nx, ny, l2_mummy.x, l2_mummy.y) && canMove(nx, ny))
                    l2_mummy.x += stepX;
            }
            // vertical second
            diffY = l2_player.y - l2_mummy.y;
            if (fabsf(diffY) > 1.0f)
            {
                float stepY = (diffY > 0) ? l2_gridStep : -l2_gridStep;
                float nx = l2_mummy.x, ny = l2_mummy.y + stepY;
                if (!enemyOccupies(nx, ny, l2_mummy.x, l2_mummy.y) && canMove(nx, ny))
                    l2_mummy.y += stepY;
            }


            // ========== MUMMY 2 MOVEMENT ========== -ths
            float diff2X = l2_player.x - l2_mummy2.x;
            float diff2Y = l2_player.y - l2_mummy2.y;

            if (fabsf(diff2X) > 1.0f)
            {
                float stepX = (diff2X > 0) ? l2_gridStep : -l2_gridStep;
                float nx = l2_mummy2.x + stepX, ny = l2_mummy2.y;
                if (!enemyOccupies(nx, ny, l2_mummy2.x, l2_mummy2.y) && canMove(nx, ny))
                    l2_mummy2.x += stepX;
            }
            diff2Y = l2_player.y - l2_mummy2.y;
            if (fabsf(diff2Y) > 1.0f)
            {
                float stepY = (diff2Y > 0) ? l2_gridStep : -l2_gridStep;
                float nx = l2_mummy2.x, ny = l2_mummy2.y + stepY;
                if (!enemyOccupies(nx, ny, l2_mummy2.x, l2_mummy2.y) && canMove(nx, ny))
                    l2_mummy2.y += stepY;
            }


            // ========== SCORPION MOVEMENT ========== -ths
            float diffSX = l2_player.x - l2_scorpion.x;
            float diffSY = l2_player.y - l2_scorpion.y;

            // horizontal first
            if (fabsf(diffSX) > 1.0f)
            {
                float stepX = (diffSX > 0) ? l2_gridStep : -l2_gridStep;
                float nx = l2_scorpion.x + stepX, ny = l2_scorpion.y;
                if (!enemyOccupies(nx, ny, l2_scorpion.x, l2_scorpion.y) && canMove(nx, ny))
                    l2_scorpion.x += stepX;
            }
            // vertical second
            diffSY = l2_player.y - l2_scorpion.y;
            if (fabsf(diffSY) > 1.0f)
            {
                float stepY = (diffSY > 0) ? l2_gridStep : -l2_gridStep;
                float nx = l2_scorpion.x, ny = l2_scorpion.y + stepY;
                if (!enemyOccupies(nx, ny, l2_scorpion.x, l2_scorpion.y) && canMove(nx, ny))
                    l2_scorpion.y += stepY;
            }
        }

        L2TickPowers();
        l2_playerMoved = false;

        // ====== BOX MUMMY AI: chase player every 2nd turn ======
        if (l2_turnCounter % 2 == 0 && l2Power.freezeFrames <= 0)
        {
            for (int i = 0; i < l2_boxMummyCount; ++i)
            {
                L2BoxMummy& bm = l2_boxMummies[i];
                float dxB = l2_player.x - bm.x;
                float dyB = l2_player.y - bm.y;

                if (fabsf(dxB) > 1.0f)
                {
                    float stepX = (dxB > 0) ? l2_gridStep : -l2_gridStep;
                    float nx = bm.x + stepX, ny = bm.y;
                    bool blocked = (fabsf(l2_mummy.x - nx) < 1.0f && fabsf(l2_mummy.y - ny) < 1.0f) ||
                        (fabsf(l2_mummy2.x - nx) < 1.0f && fabsf(l2_mummy2.y - ny) < 1.0f) ||
                        (fabsf(l2_scorpion.x - nx) < 1.0f && fabsf(l2_scorpion.y - ny) < 1.0f);
                    for (int j = 0; !blocked && j < l2_boxMummyCount; ++j)
                        if (j != i && fabsf(l2_boxMummies[j].x - nx) < 1.0f && fabsf(l2_boxMummies[j].y - ny) < 1.0f)
                            blocked = true;
                    if (!blocked && canMove(nx, ny)) bm.x += stepX;
                }

                dyB = l2_player.y - bm.y;
                if (fabsf(dyB) > 1.0f)
                {
                    float stepY = (dyB > 0) ? l2_gridStep : -l2_gridStep;
                    float nx = bm.x, ny = bm.y + stepY;
                    bool blocked = (fabsf(l2_mummy.x - nx) < 1.0f && fabsf(l2_mummy.y - ny) < 1.0f) ||
                        (fabsf(l2_mummy2.x - nx) < 1.0f && fabsf(l2_mummy2.y - ny) < 1.0f) ||
                        (fabsf(l2_scorpion.x - nx) < 1.0f && fabsf(l2_scorpion.y - ny) < 1.0f);
                    for (int j = 0; !blocked && j < l2_boxMummyCount; ++j)
                        if (j != i && fabsf(l2_boxMummies[j].x - nx) < 1.0f && fabsf(l2_boxMummies[j].y - ny) < 1.0f)
                            blocked = true;
                    if (!blocked && canMove(nx, ny)) bm.y += stepY;
                }
            }
        }
    }

    // ====== UPDATE JUMP SCARE ANIMATION =======
    JumpScare_Update(); // Before lose conditions

    // Ensure jumpscare is drawn before level resets
    static bool pendingGameOverReset = false;

    // ===== LOSE CHECK ===== -ths
    bool l2_caughtByMain = l2_turnCounter > 0 && !L2IsInvincibleNow() &&
        ((fabsf(l2_player.x - l2_mummy.x) < 1.0f && fabsf(l2_player.y - l2_mummy.y) < 1.0f) ||
            (fabsf(l2_player.x - l2_mummy2.x) < 1.0f && fabsf(l2_player.y - l2_mummy2.y) < 1.0f) ||
            (fabsf(l2_player.x - l2_scorpion.x) < 1.0f && fabsf(l2_player.y - l2_scorpion.y) < 1.0f));

    bool l2_caughtByBox = false;
    if (l2_turnCounter > 0 && !L2IsInvincibleNow())
        for (int i = 0; i < l2_boxMummyCount; ++i)
            if (fabsf(l2_player.x - l2_boxMummies[i].x) < 1.0f &&
                fabsf(l2_player.y - l2_boxMummies[i].y) < 1.0f)
            {
                l2_caughtByBox = true; break;
            }

    if (l2_caughtByMain || l2_caughtByBox)
    {
        if (!JumpScare_IsActive() && !pendingGameOverReset)
        {
            if (AEAudioIsValidAudio(l2_sfxJumpscare))
                AEAudioPlay(l2_sfxJumpscare, l2AudioGroup, 1.0f, 1.0f, 0);

            JumpScare_Trigger();
            pendingGameOverReset = true;
            std::cout << "CAUGHT! Playing jump scare...\n";
        }
    }

    // ========= LEVEL RESET FOR LOSE CONDITION =========
    // All lose conditions and jump scare occurance is in
    // Reset level can commence
    if (pendingGameOverReset && !JumpScare_IsActive())
    {
        // play gameover -ths
        if (AEAudioIsValidAudio(l2_sfxGameOver))
            AEAudioPlay(l2_sfxGameOver, l2AudioGroup, 1.0f, 1.0f, 0); // -ths

        ResetLevel2();
        pendingGameOverReset = false;
        std::cout << "Jump scare finished, resetting level...\n";
        l2_showLose = true;
        return; // skip remaining update logic this frame
    }

    // ===== WIN (EXIT DOOR) ===== -ths
    if (fabsf(l2_player.x - l2_exitPortal.x) < 1.0f &&
        fabsf(l2_player.y - l2_exitPortal.y) < 1.0f)
    {
        printf("L2: You Escaped!\n");

        gLastLevelPlayed = 2;  // -ths (tell WinPage we came from Level 2)

        next = GS_WIN;
    }

    // Legacy coin entity collect
    if (fabsf(l2_player.x - l2_coin.x) < 1.0f &&
        fabsf(l2_player.y - l2_coin.y) < 1.0f)
    {
        ++l2_coinCounter;
        printf("L2 Coin! Total: %d\n", l2_coinCounter);
        l2_coin.x = l2_coin.y = 2000.0f;
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

    // --- Treasure Box ---
    if (l2_treasureBoxActive)
    {
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(l2_treasureBoxTex, 0, 0);
        AEMtx33Scale(&scale, l2_treasureBox.size, l2_treasureBox.size);
        AEMtx33Trans(&trans, l2_treasureBox.x, l2_treasureBox.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- Box Mummies (reddish tint to distinguish from main mummies) ---
    AEGfxSetColorToMultiply(1.0f, 0.3f, 0.3f, 1.0f);
    AEGfxTextureSet(l2_mummy.pTex, 0, 0);
    for (int i = 0; i < l2_boxMummyCount; ++i)
    {
        AEMtx33Scale(&scale, l2_boxMummies[i].size, l2_boxMummies[i].size);
        AEMtx33Trans(&trans, l2_boxMummies[i].x, l2_boxMummies[i].y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // reset tint

    // --- Jump Scare --- 
    JumpScare_Draw();

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

    // Coin counter HUD
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Coins: %d", l2_coinCounter);
        AEGfxPrint(fontId, buf, -0.95f, 0.74f, 0.8f, 1.00f, 0.85f, 0.10f, 1.0f);
    }

    // Treasure box popup message
    if (l2_popupFrames > 0)
    {
        float alpha = (l2_popupFrames < 60) ? l2_popupFrames / 60.0f : 1.0f;
        AEGfxPrint(fontId, l2_popupMsg, -0.35f, 0.0f, 0.85f, 1.0f, 1.0f, 0.4f, alpha);
    }

    // ===== ADDED: Pause button (top-right) ===== -ths
    DrawPauseButton(); // -ths

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

    // ==================== AUDIO UNLOAD (LEVEL 2) ===================== // -ths
    if (AEAudioIsValidAudio(l2_sfxPlayerMove))
        AEAudioUnloadAudio(l2_sfxPlayerMove);     // -ths

    if (AEAudioIsValidAudio(l2_sfxChest))
        AEAudioUnloadAudio(l2_sfxChest);          // -ths

    if (AEAudioIsValidAudio(l2_sfxPowerup))
        AEAudioUnloadAudio(l2_sfxPowerup);        // -ths

    if (AEAudioIsValidAudio(l2_sfxJumpscare))
        AEAudioUnloadAudio(l2_sfxJumpscare);      // -ths

    if (AEAudioIsValidAudio(l2_sfxExitDoor))
        AEAudioUnloadAudio(l2_sfxExitDoor);       // -ths

    if (AEAudioIsValidAudio(l2_sfxGameOver))
        AEAudioUnloadAudio(l2_sfxGameOver);       // -ths

    AEAudioUnloadAudioGroup(l2AudioGroup);      // -ths
    // ================================================================= // -ths


    // ========== ORIGINAL TEXTURE & MESH CLEANUP ==========
    AEGfxTextureUnload(l2_player.pTex);
    AEGfxTextureUnload(l2_DesertBlockTex);
    AEGfxTextureUnload(l2_FloorTex);
    AEGfxTextureUnload(l2_mummy.pTex);
    AEGfxTextureUnload(l2_mummy2.pTex);
    AEGfxTextureUnload(l2_coin.pTex);
    AEGfxTextureUnload(l2_exitPortal.pTex);

    // ===== ADDED: unload power-up & scorpion textures ===== -ths
    AEGfxTextureUnload(l2_ImmuneTex);   // -ths
    AEGfxTextureUnload(l2_FreezeTex);   // -ths
    AEGfxTextureUnload(l2_ScorpionTex); // -ths

    // Unload treasure box texture
    if (l2_treasureBoxTex) { AEGfxTextureUnload(l2_treasureBoxTex); l2_treasureBoxTex = nullptr; }

    // ===== Unload Jump Scare =====
    JumpScare_Unload();

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }

    l2_initialised = false;
}