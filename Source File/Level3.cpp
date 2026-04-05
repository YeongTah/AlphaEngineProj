/* Start Header ***************************************************************
\file       Level3.cpp
\coders     Sharon, Jasmine, Yeong Tah, San
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#include "pch.h"

#include "leveleditor.hpp"
#include "GridUtils.h"
#include "Level1.h"
#include "Level3.h"
#include "JumpScare.h"
#include "Effects.h"
#include "gamestatemanager.h"
#include "GameStateList.h"
#include "Main.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdio>  // snprintf for HUD text
#include <cstdlib>
#include "Confirmation.h"
#include "Debug.h"

// ======================= LEVEL 3 AUDIO HANDLES =======================
static AEAudio l3_sfxPlayerMove;
static AEAudio l3_sfxChest;
static AEAudio l3_sfxPowerup;
static AEAudio l3_sfxJumpscare;
static AEAudio l3_sfxExitDoor;
static AEAudio l3_sfxGameOver;
static AEAudio l3_sfxButton;

static AEAudioGroup l3AudioGroup;
// =====================================================================

// Forward declaration so SpawnRandomPowerup() can be called
// inside ResetLevel3() / Level3_Initialize() before its definition
static void SpawnRandomPowerup();
static void L3SpawnTreasureBox(); // forward decl for use in ResetLevel3 / Initialize
static int  L3RandInt(int mn, int mx); // forward decl for use in L3SpawnTreasureBox

// ========================== TREASURE BOX SYSTEM (LEVEL 3) ==========================
static Entity           l3_treasureBox;
static bool             l3_treasureBoxActive = false;
static AEGfxTexture* l3_treasureBoxTex = nullptr;

struct L3BoxMummy { float x, y, size; };
static L3BoxMummy l3_boxMummies[8];
static int        l3_boxMummyCount = 0;

static char l3_popupMsg[64] = "";
static int  l3_popupFrames = 0;
// ==================================================================================

// IsAreaClicked has no header declaration
extern bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height,
    s32 click_x, s32 click_y);

// ---- Level 3 local entities ----
static Entity l3_player;   // Player (explorer.png)
static Entity l3_mummy1;   // Mummy 1 -- spawns top-right
static Entity l3_mummy2;   // Mummy 2 -- spawns bottom-center
static Entity l3_mummy3;   // Mummy 3 -- spawns top-left (Level 3 exclusive)
static Entity l3_exitPortal; // Exit goal; reaching it triggers GS_WIN
static AEGfxTexture* l3_DoorOpenedTex = nullptr; // Opened exit portal (after collecting coin)
static Entity l3_coin;     // Legacy single coin entity
static AEGfxTexture* l3_DesertBlockTex = nullptr; // Wall tile texture (DesertBlock.png)
static AEGfxTexture* l3_FloorTex = nullptr;       // Floor tile texture (Floor.png)
static bool l3_initialised = false; // Prevents double-initialization within a session
static int  l3_coinCounter = 0;     // Total coins collected
static int  l3_turnCounter = 0;     // Player move count; controls mummy move frequency
static bool l3_playerMoved = false; // True when player made a valid move this frame
static float l3_gridStep = 50.0f;   // World units per grid cell (= GRID_TILE_SIZE)
static float l3_nextX = 0.0f;       // Unused pending-move X (kept for parity with L1/L2)
static float l3_nextY = 0.0f;       // Unused pending-move Y

// ---- Overlay flags ----
static bool l3_paused = false;  // P key toggles; freezes game logic when true
static bool l3_showWin = false; // Shown when player reaches exit portal
static bool l3_showLose = false;// Shown when any mummy catches the player

// ---- Powerup state for Level 3 (mirrors Level 1/2 structure) ----
static struct L3PowerState {
    bool speed = false; int speedTurns = 0;
    bool freeze = false; int freezeTurns = 0;
    bool invincible = false; int invTurns = 0;
    int invFrames = 0;
    int freezeFrames = 0; // counts down each Update frame
} l3Power;

// Returns true if the player is currently invincible (either turn- or frame-based).
static bool L3IsInvincibleNow() { return l3Power.invincible || (l3Power.invFrames > 0); }

// ----------------------------------------------------------------------------
// L3TickPowers
// Decrements all turn-based powerup counters by 1. Call once per player move.
// ----------------------------------------------------------------------------
static void L3TickPowers()
{
    if (l3Power.speed && --l3Power.speedTurns <= 0) l3Power.speed = false;
    if (l3Power.freeze && --l3Power.freezeTurns <= 0) l3Power.freeze = false;
    if (l3Power.invincible && --l3Power.invTurns <= 0) l3Power.invincible = false;
}

// Per-frame counters for invincibility & freeze
static void L3TickInvFrames() { if (l3Power.invFrames > 0) --l3Power.invFrames; }
static void L3TickFreezeFrames() { if (l3Power.freezeFrames > 0) --l3Power.freezeFrames; }

// ----------------------------------------------------------------------------
// L3FindFreeSpawnCell
// Searches outward from (startRow, startCol) in expanding rings for the nearest
// empty (value == 0) cell at least 'minDist' Manhattan distance from
// (avoidRow, avoidCol). Writes the world-space tile center to (outX, outY).
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
                return;
            }
        }
    }
    GridToWorldCenter(startRow, startCol, outX, outY);
}

// ----------------------------------------------------------------------------
/* L3LoadLevelTxt <-- THIS IS THE FUNCTION THAT READS LEVEL 3's FILE
   Opens "Assets/level3.txt" and fills the shared level[][] grid.

   File format: each cell written as <value>, rows separated by newlines.
   Tile values: 0=floor, 1=wall, 4=coin, etc. (same as Level 1 and 2).

   If the file cannot be opened, all cells are set to 0.
*/
// ----------------------------------------------------------------------------
static void L3LoadLevelTxt()
{
    const char* path = "Assets/level3.txt";
    std::ifstream is(path);
    if (!is.is_open())
    {
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
}

// ----------------------------------------------------------------------------
// PlayerTouchesMummy
// Returns true if the player's world position coincides with the mummy's
// position (within 1 world unit). Used for catch/lose detection.
// The <1.0f threshold accounts for floating-point imprecision on grid centers.
// ----------------------------------------------------------------------------
static bool PlayerTouchesMummy(const Entity& mummy, float playerX, float playerY)
{
    return fabsf(playerX - mummy.x) < 1.0f && fabsf(playerY - mummy.y) < 1.0f;
}

// ----------------------------------------------------------------------------
// L3SpawnTreasureBox
// ----------------------------------------------------------------------------
static void L3SpawnTreasureBox()
{
    for (int tries = 0; tries < 256; ++tries)
    {
        int r = L3RandInt(0, GRID_ROWS - 1);
        int c = L3RandInt(0, GRID_COLS - 1);
        if (level[r][c] != 0) continue;
        int pr, pc;
        WorldToGrid(l3_player.x, l3_player.y, pr, pc);
        if (abs(r - pr) + abs(c - pc) < 3) continue;
        int cr, cc;
        WorldToGrid(l3_coin.x, l3_coin.y, cr, cc);
        if (r == cr && c == cc) continue;
        int er, ec;
        WorldToGrid(l3_exitPortal.x, l3_exitPortal.y, er, ec);
        if (r == er && c == ec) continue;
        float wx, wy;
        GridToWorldCenter(r, c, wx, wy);
        l3_treasureBox.x = wx;
        l3_treasureBox.y = wy;
        l3_treasureBox.size = l3_gridStep * 0.9f;
        l3_treasureBoxActive = true;
        return;
    }
    // Fallback: no distance constraint
    for (int tries = 0; tries < 256; ++tries)
    {
        int r = L3RandInt(0, GRID_ROWS - 1);
        int c = L3RandInt(0, GRID_COLS - 1);
        if (level[r][c] != 0) continue;
        int cr, cc;
        WorldToGrid(l3_coin.x, l3_coin.y, cr, cc);
        if (r == cr && c == cc) continue;
        int er, ec;
        WorldToGrid(l3_exitPortal.x, l3_exitPortal.y, er, ec);
        if (r == er && c == ec) continue;
        float wx, wy;
        GridToWorldCenter(r, c, wx, wy);
        l3_treasureBox.x = wx;
        l3_treasureBox.y = wy;
        l3_treasureBox.size = l3_gridStep * 0.9f;
        l3_treasureBoxActive = true;
        return;
    }
    l3_treasureBoxActive = false;
}

// ----------------------------------------------------------------------------
// L3OpenTreasureBox
// 50% +1 coin, 50% spawn a chasing box mummy. Re-places the box after opening.
// ----------------------------------------------------------------------------
static void L3OpenTreasureBox()
{
    l3_treasureBoxActive = false; // chest disappears immediately on contact

    if (AEAudioIsValidAudio(l3_sfxChest))
        AEAudioPlay(l3_sfxChest, l3AudioGroup, 1.0f, 1.0f, 0);

    bool spawnMummy = (AERandFloat() >= 0.5f);
    if (!spawnMummy)
    {
        l3_coinCounter++;
        std::snprintf(l3_popupMsg, sizeof(l3_popupMsg), "Treasure: +1 Coin! (Total: %d)", l3_coinCounter);
        l3_popupFrames = 240;
    }
    else
    {
        JumpScare_Trigger();
        if (AEAudioIsValidAudio(l3_sfxJumpscare))
            AEAudioPlay(l3_sfxJumpscare, l3AudioGroup, 1.0f, 1.0f, 0);

        if (l3_boxMummyCount < (int)(sizeof(l3_boxMummies) / sizeof(l3_boxMummies[0])))
        {
            float sx, sy;
            int br, bc, pr, pc;
            WorldToGrid(l3_treasureBox.x, l3_treasureBox.y, br, bc);
            WorldToGrid(l3_player.x, l3_player.y, pr, pc);
            L3FindFreeSpawnCell(br, bc, sx, sy, pr, pc, 2);
            L3BoxMummy& m = l3_boxMummies[l3_boxMummyCount++];
            m.x = sx; m.y = sy; m.size = l3_gridStep;
            std::snprintf(l3_popupMsg, sizeof(l3_popupMsg), "Treasure: A Mummy appeared!");
            l3_popupFrames = 240;
        }
    }
    // Chest stays inactive -- no re-spawn. A new one is placed on next reset.
}

// ========== SAVE/LOAD FOR LEVEL 3 ==========
// --------------------------------------------------------------------
// SaveLevel3State
// Saves Level 3's current runtime state to the given file path.
// --------------------------------------------------------------------
static bool SaveLevel3State(const char* path)
{
    std::ofstream f(path);
    if (!f.is_open()) return false;

    // Player
    f << l3_player.x << ' ' << l3_player.y << '\n';

    // Counters
    f << l3_coinCounter << ' ' << l3_turnCounter << '\n';

    // Power state (turn‑based + frame‑based)
    f << (int)l3Power.speed << ' ' << l3Power.speedTurns << ' '
        << (int)l3Power.freeze << ' ' << l3Power.freezeTurns << ' '
        << (int)l3Power.invincible << ' ' << l3Power.invTurns << ' '
        << l3Power.invFrames << ' ' << l3Power.freezeFrames << '\n';

    // Box mummies (from treasure chest)
    f << l3_boxMummyCount << '\n';
    for (int i = 0; i < l3_boxMummyCount; ++i)
        f << l3_boxMummies[i].x << ' ' << l3_boxMummies[i].y << '\n';

    // Main mummies (three mummies)
    f << l3_mummy1.x << ' ' << l3_mummy1.y << '\n';
    f << l3_mummy2.x << ' ' << l3_mummy2.y << '\n';
    f << l3_mummy3.x << ' ' << l3_mummy3.y << '\n';

    // Treasure chest
    f << l3_treasureBox.x << ' ' << l3_treasureBox.y << ' '
        << (l3_treasureBoxActive ? 1 : 0) << '\n';

    // Exit portal
    f << l3_exitPortal.x << ' ' << l3_exitPortal.y << '\n';

    // Legacy coin entity
    f << l3_coin.x << ' ' << l3_coin.y << '\n';

    return true;
}

// --------------------------------------------------------------------
// LoadLevel3State
// Restores Level 3's runtime state from the given file path.
// --------------------------------------------------------------------
static bool LoadLevel3State(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;

    // Player
    f >> l3_player.x >> l3_player.y;

    // Counters
    f >> l3_coinCounter >> l3_turnCounter;

    // Power state
    int sp, fr, iv;
    f >> sp >> l3Power.speedTurns
        >> fr >> l3Power.freezeTurns
        >> iv >> l3Power.invTurns
        >> l3Power.invFrames >> l3Power.freezeFrames;
    l3Power.speed = (sp != 0);
    l3Power.freeze = (fr != 0);
    l3Power.invincible = (iv != 0);

    // Box mummies
    f >> l3_boxMummyCount;
    if (l3_boxMummyCount < 0) l3_boxMummyCount = 0;
    if (l3_boxMummyCount > (int)(sizeof(l3_boxMummies) / sizeof(l3_boxMummies[0])))
        l3_boxMummyCount = (int)(sizeof(l3_boxMummies) / sizeof(l3_boxMummies[0]));
    for (int i = 0; i < l3_boxMummyCount; ++i)
    {
        f >> l3_boxMummies[i].x >> l3_boxMummies[i].y;
        l3_boxMummies[i].size = l3_gridStep;
    }

    // Main mummies
    f >> l3_mummy1.x >> l3_mummy1.y;
    f >> l3_mummy2.x >> l3_mummy2.y;
    f >> l3_mummy3.x >> l3_mummy3.y;
    l3_mummy1.size = l3_gridStep;
    l3_mummy2.size = l3_gridStep;
    l3_mummy3.size = l3_gridStep;

    // Treasure chest
    int activeFlag;
    f >> l3_treasureBox.x >> l3_treasureBox.y >> activeFlag;
    l3_treasureBoxActive = (activeFlag != 0);
    l3_treasureBox.size = l3_gridStep * 0.9f;

    // Exit portal
    f >> l3_exitPortal.x >> l3_exitPortal.y;
    l3_exitPortal.size = 50.0f;

    // Legacy coin
    f >> l3_coin.x >> l3_coin.y;
    l3_coin.size = GRID_TILE_SIZE * 0.8f;

    return true;
}

// ----------------------------------------------------------------------------
// ResetLevel3
// Repositions all Level 3 entities without reloading textures or the tile map.
// Called when any mummy catches the player (in-level reset, not full reload).
//
// Spawn layout:
// - Player : center-left (col 4)
// - Mummy 1 : top-right, min 10 cells from player
// - Mummy 2 : bottom-center, min 10 cells from player
// - Mummy 3 : top-left, min 10 cells from player
// - Coin : grid center
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

    // Reset frame-based freeze; respawn power‑up
    l3Power.freezeFrames = 0;
    SpawnRandomPowerup();

    TrailParticle_Init(); // Re-initialize trail system

    // Reset treasure chest
    l3_boxMummyCount = 0;
    L3SpawnTreasureBox();
    l3_popupFrames = 0;
    l3_popupMsg[0] = '\0';
}

// ===== Random power‑up data & helpers ==========================================
enum L3PowerupType { L3_PWR_IMMUNE = 0, L3_PWR_FREEZE = 1 };
static Entity l3_powerup;
static bool   l3_powerupActive = false;
static int    l3_powerupType = L3_PWR_IMMUNE;
static AEGfxTexture* l3_ImmuneTex = nullptr; // Assets/Immune.png
static AEGfxTexture* l3_FreezeTex = nullptr; // Assets/Freeze.png

static int L3RandInt(int mn, int mx)
{
    float t = AERandFloat(); // [0..1]
    int span = (mx - mn + 1);
    return mn + (int)(t * (float)span);
}

static void SpawnRandomPowerup()
{
    l3_powerupType = (AERandFloat() < 0.5f) ? L3_PWR_IMMUNE : L3_PWR_FREEZE;

    int pr, pc;
    WorldToGrid(l3_player.x, l3_player.y, pr, pc);
    const int NEAR_RADIUS = 5;

    struct Candidate { int r, c; };
    Candidate candidates[256];
    int candidateCount = 0;

    for (int radius = 1; radius <= NEAR_RADIUS; ++radius)
    {
        for (int dr = -radius; dr <= radius; ++dr)
        {
            for (int dc = -radius; dc <= radius; ++dc)
            {
                if (abs(dr) != radius && abs(dc) != radius) continue;

                int r = pr + dr;
                int c = pc + dc;
                if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) continue;
                if (level[r][c] != 0) continue;
                if (r == pr && c == pc) continue;

                candidates[candidateCount].r = r;
                candidates[candidateCount].c = c;
                candidateCount++;
            }
        }
    }

    if (candidateCount > 0)
    {
        int idx = L3RandInt(0, candidateCount - 1);
        int r = candidates[idx].r;
        int c = candidates[idx].c;
        float x, y;
        GridToWorldCenter(r, c, x, y);
        l3_powerup.x = x; l3_powerup.y = y;
        l3_powerup.size = 30.0f;
        l3_powerupActive = true;
        return;
    }

    // Fallback: random walkable cell anywhere (avoid player)
    for (int tries = 0; tries < 128; ++tries)
    {
        int r = L3RandInt(0, GRID_ROWS - 1);
        int c = L3RandInt(0, GRID_COLS - 1);
        if (level[r][c] == 0)
        {
            int pr2, pc2;
            WorldToGrid(l3_player.x, l3_player.y, pr2, pc2);
            if (r == pr2 && c == pc2) continue;

            float x, y;
            GridToWorldCenter(r, c, x, y);
            l3_powerup.x = x; l3_powerup.y = y;
            l3_powerup.size = 30.0f;
            l3_powerupActive = true;
            return;
        }
    }
    l3_powerupActive = false;
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
    // ================================================================
    // AUDIO LOAD FOR LEVEL 3
    // ================================================================
    l3AudioGroup = AEAudioCreateGroup();

    l3_sfxPlayerMove = AEAudioLoadSound("Assets/audio/player.wav");
    l3_sfxChest = AEAudioLoadSound("Assets/audio/chest.wav");
    l3_sfxPowerup = AEAudioLoadSound("Assets/audio/powerup.wav");
    l3_sfxJumpscare = AEAudioLoadSound("Assets/audio/jumpscare.wav");
    l3_sfxExitDoor = AEAudioLoadSound("Assets/audio/exit.wav");
    l3_sfxGameOver = AEAudioLoadSound("Assets/audio/gameover.wav");
    l3_sfxButton = AEAudioLoadSound("Assets/audio/button.wav");
    // ================================================================

    // Load Level 3's tile map from disk into the shared level[][] grid
    L3LoadLevelTxt();

    // Load entity textures
    l3_player.pTex = AEGfxTextureLoad("Assets/explorer.png");
    l3_DesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    l3_FloorTex = AEGfxTextureLoad("Assets/Floor.png");
    l3_mummy1.pTex = AEGfxTextureLoad("Assets/Enemy.png"); // all use same texture
    l3_mummy2.pTex = AEGfxTextureLoad("Assets/Enemy.png");
    l3_mummy3.pTex = AEGfxTextureLoad("Assets/Enemy.png");
    l3_coin.pTex = AEGfxTextureLoad("Assets/Coin.png");
    l3_exitPortal.pTex = AEGfxTextureLoad("Assets/DoorClosed.png");
    l3_DoorOpenedTex = AEGfxTextureLoad("Assets/DoorOpened.png");


    // Load power‑up textures
    l3_ImmuneTex = AEGfxTextureLoad("Assets/Immune.png");
    l3_FreezeTex = AEGfxTextureLoad("Assets/Freeze.png");

    l3_treasureBoxTex = AEGfxTextureLoad("Assets/TreasureChest.png");


    // Load jump scare texture
    JumpScare_Load();

    // Shared mesh for all sprites
    pMesh = CreateSquareMesh();
}

// ============================================================================
// DrawPauseButton - draws a gray rectangle and "PAUSE" text at top-right
// ============================================================================
static void DrawPauseButton()
{
    // Draw the button background rectangle
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
    const float textScale = 0.65f;
    const char* text = "PAUSE";

    // Get text dimensions in NDC
    float w, h;
    AEGfxGetPrintSize(fontId, text, textScale, &w, &h);

    // Convert button center to NDC
    float centerNDCX = 750.0f / 800.0f;   // 0.9375
    float centerNDCY = 420.0f / 450.0f;   // 0.93333...

    // Left X for centered text
    float leftX = centerNDCX - w * 0.5f;

    // Vertical centering: baseline = center + 20% of text height
    AEGfxPrint(fontId, text, leftX, centerNDCY + h * 0.2f, textScale, 1, 1, 1, 1);
}

// ----------------------------------------------------------------------------
// Level3_Initialize
// Called once after Level3_Load and on every state re-entry.
// Resets overlays and powerups, then positions all entities:
// - Player : center-left free cell
// - Mummy 1 : top-right, min 10 cells from player
// - Mummy 2 : bottom-center, min 10 cells from player
// - Mummy 3 : top-left, min 10 cells from player (third direction of threat)
// - Exit : center-right
// - Coin : grid center
// ----------------------------------------------------------------------------
void Level3_Initialize()
{

    // ================================================================
    // STOP ANY PREVIOUS AUDIO (safe + FMOD-friendly)
    // ================================================================
    AEAudioStopGroup(l3AudioGroup);    // stop leftover sounds
    // ================================================================

    // Reset powerup & overlay flags
    l3Power = {};
    l3_paused = false;
    l3_showWin = false;
    l3_showLose = false;
    l3_initialised = false; // Force full re-init every entry

    // Clear frame-based freeze
    l3Power.freezeFrames = 0;

    // Initialise jump scare
    JumpScare_Init();

    // Initialise particles
    TrailParticle_Init();

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
        l3_player.x = px;
        l3_player.y = py;
        l3_player.size = 50.0f;
        l3_player.r = 0.0f;
        l3_player.g = 0.0f;
        l3_player.b = 1.0f;

        int playerRow, playerCol;
        WorldToGrid(l3_player.x, l3_player.y, playerRow, playerCol);

        // --- Mummy 1 ---
        L3FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
        l3_mummy1.x = px;
        l3_mummy1.y = py;
        l3_mummy1.size = 50.0f;
        l3_mummy1.r = 1.0f;
        l3_mummy1.g = 0.0f;
        l3_mummy1.b = 0.0f;

        // --- Mummy 2 ---
        L3FindFreeSpawnCell(GRID_ROWS - 4, GRID_COLS / 2, px, py, playerRow, playerCol, 10);
        l3_mummy2.x = px;
        l3_mummy2.y = py;
        l3_mummy2.size = 50.0f;
        l3_mummy2.r = 1.0f;
        l3_mummy2.g = 0.0f;
        l3_mummy2.b = 0.0f;

        // --- Mummy 3 ---
        L3FindFreeSpawnCell(2, 2, px, py, playerRow, playerCol, 10);
        l3_mummy3.x = px;
        l3_mummy3.y = py;
        l3_mummy3.size = 50.0f;
        l3_mummy3.r = 1.0f;
        l3_mummy3.g = 0.0f;
        l3_mummy3.b = 0.0f;

        // --- Exit portal ---
        L3FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        l3_exitPortal.x = px;
        l3_exitPortal.y = py;
        l3_exitPortal.size = 50.0f;

        // --- Coin ---
        L3FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
        l3_coin.x = px;
        l3_coin.y = py;
        l3_coin.size = GRID_TILE_SIZE * 0.8f;
        l3_coin.r = 1.0f;
        l3_coin.g = 0.5f;
        l3_coin.b = 0.0f;

        // Spawn random power‑up
        SpawnRandomPowerup();

        // Spawn treasure box
        l3_boxMummyCount = 0;
        l3_treasureBoxActive = false;
        L3SpawnTreasureBox();

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
// All 3 mummies move every 2nd player turn via BFS pathfinding.
// Lose condition triggers if ANY of the 3 mummies occupies the player's cell.
// Jump Scare is also shown.
// ----------------------------------------------------------------------------
void Level3_Update()
{
    // --- Navigation keys ---
    if (AEInputCheckReleased(AEVK_B)) {
        next = LEVELPAGE;
        return;
    }
    if (AEInputCheckReleased(AEVK_ESCAPE) ||
        0 == AESysDoesWindowExist()) {
        next = GS_QUIT;
        return;
    }
    // --- Save (F5) / Load (F9) ---
    if (AEInputCheckReleased(AEVK_F5)) { SaveLevel3State("Assets/save3.txt"); }
    if (AEInputCheckReleased(AEVK_F9)) { LoadLevel3State("Assets/save3.txt"); }

    // --- Debug overlay toggle (F1) ---
    Debug_HandleToggle();

    // ===== PER-FRAME IMMUNITY/FREEZE TICKERS =====
    L3TickInvFrames();
    L3TickFreezeFrames();
    if (l3_popupFrames > 0) --l3_popupFrames;

    // ======================================================================
    // WIN / LOSE OVERLAY INPUT (corrected button coordinates)
    // ======================================================================
    if (l3_showLose || l3_showWin)
    {
        s32 mxS, myS;
        TransformScreentoWorld(mxS, myS);

        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            // Level Select button (0,60) 280x70
            if (IsAreaClicked(0.0f, 60.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l3_sfxButton))
                    AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
                l3_showLose = l3_showWin = false;
                next = LEVELPAGE;
                return;
            }
            // Restart button (0,-20) 280x70
            if (IsAreaClicked(0.0f, -20.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l3_sfxButton))
                    AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
                l3_showLose = l3_showWin = false;
                next = GS_LEVEL3;   // restart current level
                return;
            }
            // Quit button (0,-100) 280x70
            if (IsAreaClicked(0.0f, -100.0f, 280.0f, 70.0f, mxS, myS))
            {
                next = GS_QUIT;
                if (AEAudioIsValidAudio(l3_sfxButton))
                     AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
                Confirmation_Level(GS_LEVEL3, GS_QUIT, "Are you sure you want to quit?");
                next = CONFIRM;
                l3_showLose = l3_showWin = false;
                return;
            }
        }

        // --- Keyboard handling with confirmation ---
        if (AEInputCheckReleased(AEVK_R))
        {
            if (AEAudioIsValidAudio(l3_sfxButton))
                AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL3, GS_LEVEL3, "Are you sure you want to restart?");
            next = CONFIRM;
            l3_showLose = false;
            return;
        }
        if (AEInputCheckReleased(AEVK_RETURN) || AEInputCheckReleased(AEVK_B))
        {
            if (AEAudioIsValidAudio(l3_sfxButton))
                AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL3, LEVELPAGE, "Are you sure you want to go to Level Select?");
            next = CONFIRM;
            l3_showLose = false;
            return;
        }
        if (AEInputCheckReleased(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
            if (AEInputCheckReleased(AEVK_ESCAPE))
        {
            next = GS_QUIT;
            if (AEAudioIsValidAudio(l3_sfxButton))
                 AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL3, GS_QUIT, "Are you sure you want to quit?");
            next = CONFIRM;
            l3_showLose = l3_showWin = false;
            return;
            
        }
         if (0 == AESysDoesWindowExist())
             {
            next = GS_QUIT;
            return;
        }

        return; // overlay freeze
    }

    // =========================================================================
    // PAUSE BUTTON CLICK DETECTION (top-right corner)
    // =========================================================================
    {
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            if (IsAreaClicked(750.0f, 420.0f, 80.0f, 40.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l3_sfxButton))
                    AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
                l3_paused = !l3_paused;
                return;
            }
        }
    }
    // =========================================================================

    // --- Pause toggle ---
    if (AEInputCheckReleased(AEVK_P)) { l3_paused = !l3_paused; }

    if (l3_paused)
    {
        // --- Mouse click handling (updated coordinates) ---
        s32 mxS, myS;
        TransformScreentoWorld(mxS, myS);
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            // Resume (0, 120)
            if (IsAreaClicked(0.0f, 120.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l3_sfxButton))
                    AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
                l3_paused = false;
                return;
            }
            // Restart (0, 40)
            if (IsAreaClicked(0.0f, 40.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l3_sfxButton))
                    AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
                next = GS_RESTART;
                return;
            }
            // Level Select (0, -40)
            if (IsAreaClicked(0.0f, -40.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l3_sfxButton))
                    AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
                Confirmation_Level(GS_LEVEL3, LEVELPAGE, "Are you sure you want to go to Level Select?");
                next = CONFIRM;
                return;
            }
            // Quit (0, -120)
            if (IsAreaClicked(0.0f, -120.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l3_sfxButton))
                    AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
                Confirmation_Level(GS_LEVEL3, GS_QUIT, "Are you sure you want to quit?");
                next = CONFIRM;
                return;
            }
            // Main Menu (0, -200)
            if (IsAreaClicked(0.0f, -200.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l3_sfxButton))
                    AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
                Confirmation_Level(GS_LEVEL3, MAINMENUSTATE, "Are you sure you want to go to the Main Menu?");
                next = CONFIRM;
                return;
            }
        }

        // --- Keyboard shortcuts for the pause overlay ---
        if (AEInputCheckReleased(AEVK_P))
        {
            l3_paused = false;      // resume
            return;
        }
        if (AEInputCheckReleased(AEVK_R))
        {
            next = GS_RESTART;      // restart the level
            return;
        }
        if (AEInputCheckReleased(AEVK_B))
        {
            if (AEAudioIsValidAudio(l3_sfxButton))
                AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL3, LEVELPAGE, "Are you sure you want to go to Level Select?");
            next = CONFIRM;
            return;
        }
        if (AEInputCheckReleased(AEVK_M))
        {
            if (AEAudioIsValidAudio(l3_sfxButton))
                AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL3, MAINMENUSTATE, "Are you sure you want to go to the Main Menu?");
            next = CONFIRM;
            return;
        }
        if (AEInputCheckReleased(AEVK_ESCAPE))
        {
            if (AEAudioIsValidAudio(l3_sfxButton))
                AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL3, GS_QUIT, "Are you sure you want to quit?");
            next = CONFIRM;
            return;
        }

        return; // freeze game logic while paused
    }

    //                                                                --- PLAYER MOVEMENT START HERE ---

    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Update particle system before movement starts
    TrailParticle_Update(dt, l3_player.x, l3_player.y);


    float testX = l3_player.x;
    float testY = l3_player.y;

    bool attemptedMove = false;

    if (AEInputCheckTriggered(AEVK_W)) { testY += l3_gridStep; attemptedMove = true; }
    else if (AEInputCheckTriggered(AEVK_S)) { testY -= l3_gridStep; attemptedMove = true; }
    else if (AEInputCheckTriggered(AEVK_A)) { testX -= l3_gridStep; attemptedMove = true; }
    else if (AEInputCheckTriggered(AEVK_D)) { testX += l3_gridStep; attemptedMove = true; }

    // Player movement: must be walkable
    if (attemptedMove && IsTileWalkable(testX, testY))
    {
        l3_player.x = testX;
        l3_player.y = testY;
        l3_playerMoved = true;

        // PLAY MOVEMENT AUDIO
        if (AEAudioIsValidAudio(l3_sfxPlayerMove))
            AEAudioPlay(l3_sfxPlayerMove, l3AudioGroup, 1.0f, 1.0f, 0);

        // Particle appear only when player move
        TrailParticle_OnPlayerMoved(l3_player.x, l3_player.y);
    }

//                                                                --- PLAYER MOVEMENT END HERE ---
// 
    // ======================================================================
    // POWER-UP PICKUP
    // ======================================================================
    if (l3_powerupActive &&
        fabsf(l3_player.x - l3_powerup.x) < 1.0f &&
        fabsf(l3_player.y - l3_powerup.y) < 1.0f)
    {
        // play powerup audio
        if (AEAudioIsValidAudio(l3_sfxPowerup))
            AEAudioPlay(l3_sfxPowerup, l3AudioGroup, 1.0f, 1.0f, 0);

        if (l3_powerupType == L3_PWR_IMMUNE)
            l3Power.invFrames = 300;        // 5s
        else
            l3Power.freezeFrames = 180;     // 3s

        l3_powerupActive = false;
        l3_powerup.x = l3_powerup.y = 2000.0f;
    }

    // ======================================================================
    // PER TURN LOGIC
    // ======================================================================
    if (l3_playerMoved)
    {
        l3_turnCounter++;

        // Coin tile collection
        int r, c;
        WorldToGrid(l3_player.x, l3_player.y, r, c);
        if (level[r][c] == 4) // coin counter
        {
            level[r][c] = 0;
            l3_coinCounter++;
            if (AEAudioIsValidAudio(l3_sfxButton))
                AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
        }

        // Mummy movement every 2nd turn, unless frozen
        if (l3_turnCounter % 2 == 0 && l3Power.freezeFrames <= 0)
        {
            // ---- BFS: find the next step on the shortest path from (startR,startC)
            // to (goalR,goalC), avoiding wall tiles (value == 1).
            // Writes the first-step grid coords into outR[0]/outC[0].
            // Returns false if already at goal or no path exists.
            auto L3BFSNextStep = [&](int startR, int startC, int goalR, int goalC,
                int outR[], int outC[]) -> bool
                {
                    if (startR == goalR && startC == goalC) return false;

                    bool visited[GRID_ROWS][GRID_COLS] = {};
                    int  parentR[GRID_ROWS][GRID_COLS];
                    int  parentC[GRID_ROWS][GRID_COLS];
                    for (int i = 0; i < GRID_ROWS; ++i)
                        for (int j = 0; j < GRID_COLS; ++j)
                        {
                            parentR[i][j] = -1; parentC[i][j] = -1;
                        }

                    int qR[GRID_ROWS * GRID_COLS] = {}, qC[GRID_ROWS * GRID_COLS] = {};
                    int head = 0, tail = 0;
                    visited[startR][startC] = true;
                    qR[tail] = startR; qC[tail] = startC; ++tail;

                    const int dr[] = { -1, 1, 0, 0 };
                    const int dc[] = { 0, 0,-1, 1 };
                    bool found = false;

                    while (head < tail && !found)
                    {
                        int cr = qR[head], cc = qC[head]; ++head;
                        for (int d = 0; d < 4; ++d)
                        {
                            int nr = cr + dr[d], nc = cc + dc[d];
                            if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS) continue;
                            if (visited[nr][nc]) continue;
                            if (level[nr][nc] == 1) continue;
                            visited[nr][nc] = true;
                            parentR[nr][nc] = cr;
                            parentC[nr][nc] = cc;
                            qR[tail] = nr; qC[tail] = nc; ++tail;
                            if (nr == goalR && nc == goalC) { found = true; break; }
                        }
                    }

                    if (!found) return false;

                    // Trace back to find the first step from start
                    int tr = goalR, tc = goalC;
                    while (parentR[tr][tc] != startR || parentC[tr][tc] != startC)
                    {
                        int pr2 = parentR[tr][tc], pc2 = parentC[tr][tc];
                        tr = pr2; tc = pc2;
                    }
                    outR[0] = tr; outC[0] = tc;
                    return true;
                };

            // Player grid position (BFS goal for all three mummies)
            int playerR, playerC;
            WorldToGrid(l3_player.x, l3_player.y, playerR, playerC);

            Entity* mummies[3] = { &l3_mummy1, &l3_mummy2, &l3_mummy3 };

            for (int i = 0; i < 3; ++i)
            {
                Entity& m = *mummies[i];
                int mR, mC, nR[1], nC[1];
                WorldToGrid(m.x, m.y, mR, mC);

                if (L3BFSNextStep(mR, mC, playerR, playerC, nR, nC))
                {
                    float nx, ny;
                    GridToWorldCenter(nR[0], nC[0], nx, ny);
                    // If the target is the player's cell and the player is invincible, skip moving.
                    int pR, pC;
                    WorldToGrid(l3_player.x, l3_player.y, pR, pC);
                    if (nR[0] == pR && nC[0] == pC && L3IsInvincibleNow())
                    {
                        // Do nothing
                    }
                    else
                    {
                        // Prevent two mummies from stacking on the same tile
                        bool blocked = false;
                        for (int j = 0; j < 3; ++j)
                            if (j != i && fabsf(mummies[j]->x - nx) < 1.0f && fabsf(mummies[j]->y - ny) < 1.0f)
                            {
                                blocked = true; break;
                            }
                        if (!blocked) { m.x = nx; m.y = ny; }
                    }
                }
            }
        }

        L3TickPowers();
        l3_playerMoved = false;

        // ====== TREASURE BOX TOUCH ======
        if (l3_treasureBoxActive &&
            fabsf(l3_player.x - l3_treasureBox.x) < 1.0f &&
            fabsf(l3_player.y - l3_treasureBox.y) < 1.0f)
        {
            L3OpenTreasureBox();
        }

        // ====== BOX MUMMY AI: BFS chase player every turn ======
        if (l3Power.freezeFrames <= 0)
        {
            int playerR2, playerC2;
            WorldToGrid(l3_player.x, l3_player.y, playerR2, playerC2);

            for (int i = 0; i < l3_boxMummyCount; ++i)
            {
                L3BoxMummy& bm = l3_boxMummies[i];

                int bmR, bmC;
                WorldToGrid(bm.x, bm.y, bmR, bmC);

                // BFS for this box mummy
                bool visited2[GRID_ROWS][GRID_COLS] = {};
                int  parentR2[GRID_ROWS][GRID_COLS], parentC2[GRID_ROWS][GRID_COLS];
                for (int a = 0; a < GRID_ROWS; ++a)
                    for (int b = 0; b < GRID_COLS; ++b)
                    {
                        parentR2[a][b] = -1; parentC2[a][b] = -1;
                    }

                int qR2[GRID_ROWS * GRID_COLS] = {}, qC2[GRID_ROWS * GRID_COLS] = {};
                int head2 = 0, tail2 = 0;
                visited2[bmR][bmC] = true;
                qR2[tail2] = bmR; qC2[tail2] = bmC; ++tail2;

                const int dr2[] = { -1, 1, 0, 0 };
                const int dc2[] = { 0, 0,-1, 1 };
                bool found2 = false;

                while (head2 < tail2 && !found2)
                {
                    int cr = qR2[head2], cc = qC2[head2]; ++head2;
                    for (int d = 0; d < 4; ++d)
                    {
                        int nr = cr + dr2[d], nc = cc + dc2[d];
                        if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS) continue;
                        if (visited2[nr][nc]) continue;
                        if (level[nr][nc] == 1) continue;
                        visited2[nr][nc] = true;
                        parentR2[nr][nc] = cr;
                        parentC2[nr][nc] = cc;
                        qR2[tail2] = nr; qC2[tail2] = nc; ++tail2;
                        if (nr == playerR2 && nc == playerC2) { found2 = true; break; }
                    }
                }

                if (!found2) continue;

                // Trace back to first step
                int tr = playerR2, tc = playerC2;
                while (parentR2[tr][tc] != bmR || parentC2[tr][tc] != bmC)
                {
                    int pr2 = parentR2[tr][tc], pc2 = parentC2[tr][tc];
                    tr = pr2; tc = pc2;
                }

                float nx, ny;
                GridToWorldCenter(tr, tc, nx, ny);

                // If the target is the player's cell and the player is invincible, skip moving.
                int pR, pC;
                WorldToGrid(l3_player.x, l3_player.y, pR, pC);
                if (tr == pR && tc == pC && L3IsInvincibleNow())
                {
                    // Do nothing
                }
                else
                {
                    // Block if occupied by a main mummy or another box mummy
                    bool blocked =
                        (fabsf(l3_mummy1.x - nx) < 1.0f && fabsf(l3_mummy1.y - ny) < 1.0f) ||
                        (fabsf(l3_mummy2.x - nx) < 1.0f && fabsf(l3_mummy2.y - ny) < 1.0f) ||
                        (fabsf(l3_mummy3.x - nx) < 1.0f && fabsf(l3_mummy3.y - ny) < 1.0f);
                    for (int j = 0; !blocked && j < l3_boxMummyCount; ++j)
                        if (j != i &&
                            fabsf(l3_boxMummies[j].x - nx) < 1.0f &&
                            fabsf(l3_boxMummies[j].y - ny) < 1.0f)
                            blocked = true;

                    if (!blocked) { bm.x = nx; bm.y = ny; }
                }
            }
        }
    }

    // ====== UPDATE JUMP SCARE ANIMATION =======
    JumpScare_Update(); // Before lose conditions

    // Ensure jumpscare is drawn before level resets
    static bool pendingGameOverReset = false;

    // ======================================================================
    // LOSE CONDITION (THREE MUMMIES + BOX MUMMIES)
    // ======================================================================
    if (l3_turnCounter > 0 &&
        !L3IsInvincibleNow() &&
        (PlayerTouchesMummy(l3_mummy1, l3_player.x, l3_player.y) ||
            PlayerTouchesMummy(l3_mummy2, l3_player.x, l3_player.y) ||
            PlayerTouchesMummy(l3_mummy3, l3_player.x, l3_player.y)))
    {
        if (!JumpScare_IsActive() && !pendingGameOverReset)
        {
            // Play jumpscare
            if (AEAudioIsValidAudio(l3_sfxJumpscare))
                AEAudioPlay(l3_sfxJumpscare, l3AudioGroup, 1.0f, 1.0f, 0);

            // Trigger jump scare immediately when caught
            JumpScare_Trigger();
            pendingGameOverReset = true; // Mark that we need to reset after jump scare
        }
    }

    // ====== BOX MUMMY CATCH ======
    if (l3_turnCounter > 0 && !L3IsInvincibleNow())
    {
        for (int i = 0; i < l3_boxMummyCount; ++i)
        {
            if (fabsf(l3_player.x - l3_boxMummies[i].x) < 1.0f &&
                fabsf(l3_player.y - l3_boxMummies[i].y) < 1.0f)
            {
                if (!JumpScare_IsActive() && !pendingGameOverReset)
                {
                    if (AEAudioIsValidAudio(l3_sfxJumpscare))
                        AEAudioPlay(l3_sfxJumpscare, l3AudioGroup, 1.0f, 1.0f, 0);
                    JumpScare_Trigger();
                    pendingGameOverReset = true;
                }
            }
        }
    }

    // ========= LEVEL RESET FOR LOSE CONDITION =========
    if (pendingGameOverReset && !JumpScare_IsActive())
    {
        // Play gameover
        if (AEAudioIsValidAudio(l3_sfxGameOver))
            AEAudioPlay(l3_sfxGameOver, l3AudioGroup, 1.0f, 1.0f, 0);

        ResetLevel3();
        pendingGameOverReset = false;
        l3_showLose = true;
        return; // skip remaining update logic this frame
    }

    // ======================================================================
    // WIN CONDITION (EXIT PORTAL)
    // ======================================================================
    if (fabsf(l3_player.x - l3_exitPortal.x) < 1.0f &&
        fabsf(l3_player.y - l3_exitPortal.y) < 1.0f)
    {
        if (l3_coinCounter >= 1)
        {
            // Exit audio
            if (AEAudioIsValidAudio(l3_sfxExitDoor))
                AEAudioPlay(l3_sfxExitDoor, l3AudioGroup, 1.0f, 1.0f, 0);


            gLastLevelPlayed = 3;   // so WinPage restarts Level 3

            next = GS_WIN;
        }
        else
        {
            // Player hasn't collected a coin yet -- show reminder
            std::snprintf(l3_popupMsg, sizeof(l3_popupMsg), "Collect a coin first!");
            l3_popupFrames = 120;
        }
    }

    // ---------------------------------------------------------------------
    // Legacy Coin entity
    // ---------------------------------------------------------------------
    if (fabsf(l3_player.x - l3_coin.x) < 1.0f &&
        fabsf(l3_player.y - l3_coin.y) < 1.0f)
    {
        ++l3_coinCounter;
        l3_coin.x = l3_coin.y = 2000.0f;
        if (AEAudioIsValidAudio(l3_sfxButton))
            AEAudioPlay(l3_sfxButton, l3AudioGroup, 1.0f, 1.0f, 0);
    }
}

// ----------------------------------------------------------------------------
// Level3_Draw
// Called every frame to render Level 3.
// Rendering order (back to front):
// 1. Lose/Win/Pause overlay check -- delegate and return if active.
// 2. Floor tiles (value == 0).
// 3. Wall tiles (value == 1).
// 4. Player, Mummy 1, Mummy 2, Mummy 3.
// 5. Coin entity (only while coin.x < 1000).
// 6. Exit portal.
// ----------------------------------------------------------------------------
void Level3_Draw()
{
    AEGfxSetBackgroundColor(0.22f, 0.14f, 0.09f);

    // ===== Reset dirty render state before overlays =====
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    // =====================================================

    // Delegate overlay pages
    if (l3_showLose) { LosePage_Draw(); return; }
    if (l3_showWin) { WinPage_Draw();  return; }
    if (l3_paused) { PausePage_Draw(); return; }

    AEMtx33 transform, scale, trans;

    // --- Floor tiles (value == 0 and value == 4) ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(l3_FloorTex, 0, 0);

    for (int row = 0; row < GRID_ROWS; row++)
        for (int col = 0; col < GRID_COLS; col++)
            if (level[row][col] == 0 || level[row][col] == 4)
            {
                float x, y;
                GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }

    // --- Coin tiles (value == 4) ---
    AEGfxTextureSet(l3_coin.pTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
        for (int col = 0; col < GRID_COLS; col++)
            if (level[row][col] == 4)
            {
                float x, y;
                GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, l3_coin.size, l3_coin.size);
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
                float x, y;
                GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }

    // --- Player ---
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

    // Render power‑up if active
    if (l3_powerupActive)
    {
        AEGfxTextureSet(
            (l3_powerupType == L3_PWR_IMMUNE) ? l3_ImmuneTex : l3_FreezeTex,
            0, 0
        );
        AEMtx33Scale(&scale, l3_powerup.size, l3_powerup.size);
        AEMtx33Trans(&trans, l3_powerup.x, l3_powerup.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- Exit portal ---
    AEGfxTextureSet((l3_coinCounter > 0) ? l3_DoorOpenedTex : l3_exitPortal.pTex, 0, 0);
    AEMtx33Scale(&scale, l3_exitPortal.size, l3_exitPortal.size);
    AEMtx33Trans(&trans, l3_exitPortal.x, l3_exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Treasure Box ---
    if (l3_treasureBoxActive)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(l3_treasureBoxTex, 0, 0);
        AEMtx33Scale(&scale, l3_treasureBox.size, l3_treasureBox.size);
        AEMtx33Trans(&trans, l3_treasureBox.x, l3_treasureBox.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- Box Mummies (red-tinted) ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 0.3f, 0.3f, 1.0f);
    AEGfxTextureSet(l3_mummy1.pTex, 0, 0);
    for (int i = 0; i < l3_boxMummyCount; ++i)
    {
        AEMtx33Scale(&scale, l3_boxMummies[i].size, l3_boxMummies[i].size);
        AEMtx33Trans(&trans, l3_boxMummies[i].x, l3_boxMummies[i].y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    // ===== HUD for active power-ups =====
    if (l3Power.invFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "IMMUNE  %.1fs", l3Power.invFrames / 60.0f);
        AEGfxPrint(fontId, buf, -0.95f, 0.74f, 1.0f, 0.90f, 0.90f, 0.20f, 1.0f);
    }
    if (l3Power.freezeFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "FREEZE  %.1fs", l3Power.freezeFrames / 60.0f);
        AEGfxPrint(fontId, buf, -0.95f, 0.82f, 1.0f, 0.60f, 0.85f, 1.00f, 1.0f);
    }

    // ====== Coin counter HUD ======
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Coins: %d", l3_coinCounter);
        AEGfxPrint(fontId, buf, -0.95f, 0.90f, 1.2f, 0.60f, 0.15f, 0.20f, 1.0f);
    }

    // ====== Hint: collect a coin before escaping (shown until first coin collected) ======
    if (l3_coinCounter == 0)
    {
        const char* hint = "Coins to escape: 0/1";
        float hw, hh;
        AEGfxGetPrintSize(fontId, hint, 1.2f, &hw, &hh);
        AEGfxPrint(fontId, hint, -hw * 0.5f, 0.90f, 1.2f, 0.60f, 0.15f, 0.20f, 1.0f);
    }

    // ====== Treasure box popup -- centered ======
    if (l3_popupFrames > 0)
    {
        float alpha = (l3_popupFrames < 60) ? l3_popupFrames / 60.0f : 1.0f;
        float popupScale = 0.85f;
        float pw, ph;
        AEGfxGetPrintSize(fontId, l3_popupMsg, popupScale, &pw, &ph);
        float centeredX = -pw * 0.5f;
        AEGfxPrint(fontId, l3_popupMsg, centeredX, 0.4f, popupScale, 1.0f, 1.0f, 0.4f, alpha);
    }

    // ===== Pause button (top-right) =====
    DrawPauseButton();

    // === Draw particles ===
    TrailParticle_Draw();

    // === Jump Scare ===
    JumpScare_Draw();

    // ---- Debug overlay: fill info struct and call shared draw function ----
    // Press F1 in-game to toggle on / off.
    if (Debug_IsActive())
    {
        DebugEntityInfo dbg;
        dbg.playerX = l3_player.x;
        dbg.playerY = l3_player.y;
        dbg.playerSize = l3_player.size;

        dbg.hasMummy1 = true;
        dbg.mummy1X = l3_mummy1.x;
        dbg.mummy1Y = l3_mummy1.y;
        dbg.mummy1Size = l3_mummy1.size;

        dbg.hasMummy2 = true;
        dbg.mummy2X = l3_mummy2.x;
        dbg.mummy2Y = l3_mummy2.y;
        dbg.mummy2Size = l3_mummy2.size;

        dbg.hasMummy3 = true;
        dbg.mummy3X = l3_mummy3.x;
        dbg.mummy3Y = l3_mummy3.y;
        dbg.mummy3Size = l3_mummy3.size;

        dbg.exitX = l3_exitPortal.x;
        dbg.exitY = l3_exitPortal.y;
        dbg.exitSize = l3_exitPortal.size;
        dbg.coinX = l3_coin.x;
        dbg.coinY = l3_coin.y;
        dbg.coinSize = l3_coin.size;

        dbg.powerupActive = l3_powerupActive;
        dbg.powerupX = l3_powerup.x;
        dbg.powerupY = l3_powerup.y;
        dbg.powerupSize = l3_powerup.size;

        dbg.treasureBoxActive = l3_treasureBoxActive;
        dbg.treasureBoxX = l3_treasureBox.x; dbg.treasureBoxY = l3_treasureBox.y;
        dbg.treasureBoxSize = l3_treasureBox.size;

        dbg.boxMummyCount = l3_boxMummyCount;
        for (int i = 0; i < l3_boxMummyCount; ++i)
        {
            dbg.boxMummyX[i] = l3_boxMummies[i].x;
            dbg.boxMummyY[i] = l3_boxMummies[i].y;
            dbg.boxMummySize[i] = l3_boxMummies[i].size;
        }

        dbg.coinCounter = l3_coinCounter;
        dbg.turnCounter = l3_turnCounter;
        dbg.invincibleActive = l3Power.invincible;
        dbg.invFrames = l3Power.invFrames;
        dbg.freezeActive = l3Power.freeze;
        dbg.freezeFrames = l3Power.freezeFrames;
        dbg.speedActive = l3Power.speed;
        dbg.speedTurns = l3Power.speedTurns;
        dbg.isPaused = l3_paused;
        dbg.isWin = l3_showWin;
        dbg.isLose = l3_showLose;
        dbg.popupFrames = l3_popupFrames;
        dbg.tileSize = GRID_TILE_SIZE;

        Debug_DrawOverlay(dbg);
    }

    // Reset render state for next frame
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
}

// ----------------------------------------------------------------------------
// Level3_Free -- currently empty; no additional cleanup needed.
// ----------------------------------------------------------------------------
void Level3_Free(){}

// ----------------------------------------------------------------------------
// Level3_Unload
// Unloads all Level 3 GPU textures and frees the shared mesh.
// Resets l3_initialised so Initialize runs fully on next entry.
// ----------------------------------------------------------------------------
void Level3_Unload()
{
    // ==================== AUDIO UNLOAD (LEVEL 3) =====================
    if (AEAudioIsValidAudio(l3_sfxPlayerMove))
        AEAudioUnloadAudio(l3_sfxPlayerMove);

    if (AEAudioIsValidAudio(l3_sfxChest))
        AEAudioUnloadAudio(l3_sfxChest);

    if (AEAudioIsValidAudio(l3_sfxPowerup))
        AEAudioUnloadAudio(l3_sfxPowerup);

    if (AEAudioIsValidAudio(l3_sfxJumpscare))
        AEAudioUnloadAudio(l3_sfxJumpscare);

    if (AEAudioIsValidAudio(l3_sfxExitDoor))
        AEAudioUnloadAudio(l3_sfxExitDoor);

    if (AEAudioIsValidAudio(l3_sfxGameOver))
        AEAudioUnloadAudio(l3_sfxGameOver);

    if (AEAudioIsValidAudio(l3_sfxButton))
        AEAudioUnloadAudio(l3_sfxButton);

    AEAudioUnloadAudioGroup(l3AudioGroup);
    // =================================================================

    // ========== ORIGINAL TEXTURE & MESH CLEANUP ==========
    AEGfxTextureUnload(l3_player.pTex);
    AEGfxTextureUnload(l3_DesertBlockTex);
    AEGfxTextureUnload(l3_FloorTex);
    AEGfxTextureUnload(l3_mummy1.pTex);
    AEGfxTextureUnload(l3_mummy2.pTex);
    AEGfxTextureUnload(l3_mummy3.pTex);
    AEGfxTextureUnload(l3_coin.pTex);
    AEGfxTextureUnload(l3_exitPortal.pTex);
    AEGfxTextureUnload(l3_DoorOpenedTex);

    // Unload power-up textures
    AEGfxTextureUnload(l3_ImmuneTex);
    AEGfxTextureUnload(l3_FreezeTex);

    if (l3_treasureBoxTex) { AEGfxTextureUnload(l3_treasureBoxTex); l3_treasureBoxTex = nullptr; }

    // Unload Jump Scare
    JumpScare_Unload();

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }

    l3_initialised = false;
}