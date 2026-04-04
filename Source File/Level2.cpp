
#include "pch.h"

#include "leveleditor.hpp"
#include "GridUtils.h"
#include "Level1.h"
#include "Level2.h"
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

// ======================= LEVEL 2 AUDIO HANDLES =======================
static AEAudio l2_sfxPlayerMove;
static AEAudio l2_sfxChest;
static AEAudio l2_sfxPowerup;
static AEAudio l2_sfxJumpscare;
static AEAudio l2_sfxExitDoor;
static AEAudio l2_sfxGameOver;
static AEAudio l2_sfxButton;

static AEAudioGroup l2AudioGroup;
// =====================================================================

// Forward declaration so SpawnRandomPowerup() can be called
// inside ResetLevel2() / Level2_Initialize() before its definition
static void SpawnRandomPowerup();
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
static Entity l2_scorpion; // Scorpion enemy entity (scorpion.png)
static Entity l2_exitPortal; // Exit goal; reaching it triggers GS_WIN
static Entity l2_coin;   // Legacy single coin entity
static AEGfxTexture* l2_DesertBlockTex = nullptr; // Wall tile texture
static AEGfxTexture* l2_FloorTex = nullptr;       // Floor tile texture
static AEGfxTexture* l2_DoorOpenedTex = nullptr; // Opened exit portal texture (DoorOpened.png)
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
    int freezeFrames = 0; // decremented every Update frame (~180 frames @60 FPS)
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

// Frame tickers for per-frame immunity/freeze counters
static void L2TickInvFrames() { if (l2Power.invFrames > 0) --l2Power.invFrames; }
static void L2TickFreezeFrames() { if (l2Power.freezeFrames > 0) --l2Power.freezeFrames; }

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

// ===== Reachability helpers to keep scorpion out of sealed pockets =====
static bool L2IsCellInBounds(int r, int c) {
    return r >= 0 && r < GRID_ROWS && c >= 0 && c < GRID_COLS;
}

// Return true if 'target (tr,tc)' is reachable from 'start (sr,sc)' via 4-neighbour floor (value==0).
static bool L2IsReachable(int sr, int sc, int tr, int tc) {
    if (!L2IsCellInBounds(sr, sc) || !L2IsCellInBounds(tr, tc))
        return false;
    if (level[sr][sc] != 0 || level[tr][tc] != 0)
        return false;

    static int vis[GRID_ROWS][GRID_COLS];
    for (int i = 0; i < GRID_ROWS; ++i) for (int j = 0; j < GRID_COLS; ++j)
        vis[i][j] = 0;

    struct Node { int r, c; };
    Node q[GRID_ROWS * GRID_COLS];
    int qb = 0, qe = 0;
    q[qe++] = { sr, sc };
    vis[sr][sc] = 1;
    static int dr[4] = { -1, 1, 0, 0 };
    static int dc[4] = { 0, 0,-1, 1 };

    while (qb < qe) {
        Node cur = q[qb++];
        if (cur.r == tr && cur.c == tc) return true;
        for (int k = 0; k < 4; ++k) {
            int nr = cur.r + dr[k], nc = cur.c + dc[k];
            if (!L2IsCellInBounds(nr, nc)) continue;
            if (vis[nr][nc]) continue;
            if (level[nr][nc] != 0) continue;
            vis[nr][nc] = 1;
            q[qe++] = { nr,nc };
        }
    }
    return false;
}

// Prefer the given startRow/startCol; ensure the chosen free tile is reachable from the player.
static void L2FindReachableSpawnNear(int startRow, int startCol,
    int avoidRow, int avoidCol,
    int minDist,
    float& outX, float& outY,
    int playerRow, int playerCol)
{
    // 1) Try the regular near-start search first.
    float tx, ty;
    L2FindFreeSpawnCell(startRow, startCol, tx, ty, avoidRow, avoidCol, minDist);
    int tr, tc; WorldToGrid(tx, ty, tr, tc);
    if (L2IsCellInBounds(tr, tc) && level[tr][tc] == 0 &&
        L2IsReachable(playerRow, playerCol, tr, tc)) {
        outX = tx; outY = ty;
        return;
    }

    // 2) Scan a neighbourhood around (startRow,startCol) for a reachable free tile.
    int maxRadius = 10;
    for (int radius = 0; radius <= maxRadius; ++radius) {
        for (int dr = -radius; dr <= radius; ++dr) {
            for (int dc = -radius; dc <= radius; ++dc) {
                if (abs(dr) != radius && abs(dc) != radius) continue;
                int r = startRow + dr, c = startCol + dc;
                if (!L2IsCellInBounds(r, c)) continue;
                if (level[r][c] != 0) continue;
                // respect minDist from avoid cell (usually the player)
                if (avoidRow >= 0 && avoidCol >= 0) {
                    if (abs(r - avoidRow) + abs(c - avoidCol) < minDist)
                        continue;
                }
                if (L2IsReachable(playerRow, playerCol, r, c)) {
                    GridToWorldCenter(r, c, outX, outY);
                    return;
                }
            }
        }
    }

    // 3) Fallback: pick the first reachable free tile anywhere.
    for (int r = 0; r < GRID_ROWS; ++r)
        for (int c = 0; c < GRID_COLS; ++c)
            if (level[r][c] == 0 && L2IsReachable(playerRow, playerCol, r, c))
            {
                GridToWorldCenter(r, c, outX, outY); return;
            }

    // 4) Last resort: just return the player's tile (almost never triggered).
    GridToWorldCenter(playerRow, playerCol, outX, outY);
}

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

    // Scorpion spawn with reachability-aware helper
    L2FindReachableSpawnNear(GRID_ROWS - 4, 2,      // preferred bottom-left area
        playerRow, playerCol,  // avoid near player
        8,                     // min Manhattan distance
        px, py,                // out world coordinates
        playerRow, playerCol); // for reachability check
    l2_scorpion.x = px; l2_scorpion.y = py; l2_scorpion.size = 50.0f;

    // Coin spawn: grid center
    L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
    l2_coin.x = px; l2_coin.y = py;
    l2_nextX = l2_player.x;
    l2_nextY = l2_player.y;
    l2_coinCounter = 0;
    l2_turnCounter = 0;
    l2_playerMoved = false;
    l2Power = {}; // clear all powerup state

    // Reset frame-based freeze; respawn a power‑up
    l2Power.freezeFrames = 0;
    SpawnRandomPowerup();

    // Reset treasure box (one-time chest — re-spawn it for the new round)
    l2_boxMummyCount = 0;
    L2SpawnTreasureBox();

    TrailParticle_Init(); // Re-initialize trail system

    // Clear any lingering popup
    l2_popupFrames = 0;
    l2_popupMsg[0] = '\0';
}

// ===== Random power‑up data & helpers ==========================================
enum L2PowerupType { L2_PWR_IMMUNE = 0, L2_PWR_FREEZE = 1 };
static Entity l2_powerup;
static bool   l2_powerupActive = false;
static int    l2_powerupType = L2_PWR_IMMUNE;
static AEGfxTexture* l2_ImmuneTex = nullptr; // Assets/Immune.png
static AEGfxTexture* l2_FreezeTex = nullptr; // Assets/Freeze.png

static int L2RandInt(int mn, int mx)
{
    float t = AERandFloat(); // [0..1]
    int span = (mx - mn + 1);
    return mn + (int)(t * (float)span);
}

static void SpawnRandomPowerup()
{
    l2_powerupType = (AERandFloat() < 0.5f) ? L2_PWR_IMMUNE : L2_PWR_FREEZE;

    int pr, pc;
    WorldToGrid(l2_player.x, l2_player.y, pr, pc);
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
        int idx = L2RandInt(0, candidateCount - 1);
        int r = candidates[idx].r;
        int c = candidates[idx].c;
        float x, y;
        GridToWorldCenter(r, c, x, y);
        l2_powerup.x = x; l2_powerup.y = y;
        l2_powerup.size = 30.0f;
        l2_powerupActive = true;
        return;
    }

    // Fallback: random walkable cell anywhere (avoid player)
    for (int tries = 0; tries < 128; ++tries)
    {
        int r = L2RandInt(0, GRID_ROWS - 1);
        int c = L2RandInt(0, GRID_COLS - 1);
        if (level[r][c] == 0)
        {
            int pr2, pc2;
            WorldToGrid(l2_player.x, l2_player.y, pr2, pc2);
            if (r == pr2 && c == pc2) continue;

            float x, y;
            GridToWorldCenter(r, c, x, y);
            l2_powerup.x = x; l2_powerup.y = y;
            l2_powerup.size = 30.0f;
            l2_powerupActive = true;
            return;
        }
    }
    l2_powerupActive = false;
}

// ----------------------------------------------------------------------------
// L2SpawnTreasureBox
// Places the treasure box at a random free cell at least 3 Manhattan cells
// away from the player. Re-called after each box is opened.
// ----------------------------------------------------------------------------
static void L2SpawnTreasureBox()
{
    int pr, pc;
    WorldToGrid(l2_player.x, l2_player.y, pr, pc);

    for (int tries = 0; tries < 256; ++tries)
    {
        int r = L2RandInt(0, GRID_ROWS - 1);
        int c = L2RandInt(0, GRID_COLS - 1);
        if (level[r][c] != 0) continue;                          
        if (abs(r - pr) + abs(c - pc) < 3) continue;             
        if (!L2IsReachable(pr, pc, r, c)) continue;             
        int cr, cc;
        WorldToGrid(l2_coin.x, l2_coin.y, cr, cc);
        if (r == cr && c == cc) continue;
        float wx, wy;
        GridToWorldCenter(r, c, wx, wy);
        l2_treasureBox.x = wx;
        l2_treasureBox.y = wy;
        l2_treasureBox.size = l2_gridStep * 0.9f;
        l2_treasureBoxActive = true;
        printf("L2 TreasureBox spawned at grid (%d,%d) | world (%.1f,%.1f)\n", r, c, wx, wy);
        return;
    }
    // Fallback: drop distance constraint but keep reachability
    for (int tries = 0; tries < 256; ++tries)
    {
        int r = L2RandInt(0, GRID_ROWS - 1);
        int c = L2RandInt(0, GRID_COLS - 1);
        if (level[r][c] != 0) continue;
        if (!L2IsReachable(pr, pc, r, c)) continue;    
        int cr, cc;
        WorldToGrid(l2_coin.x, l2_coin.y, cr, cc);
        if (r == cr && c == cc) continue;
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
        l2_popupFrames = 240;
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
            l2_popupFrames = 240;
        }
    }
    // NOTE: L2SpawnTreasureBox() intentionally NOT called here -- chest appears once only.
}

// Scorpion texture handle (loaded in Level2_Load)
static AEGfxTexture* l2_ScorpionTex = nullptr; // Assets/scorpion.png

// ========== SAVE/LOAD FOR LEVEL 2 ==========
// --------------------------------------------------------------------
// SaveLevel2State
// Saves Level 2's current runtime state to the given file path.
// --------------------------------------------------------------------
static bool SaveLevel2State(const char* path)
{
    std::ofstream f(path);
    if (!f.is_open()) return false;

    // Player
    f << l2_player.x << ' ' << l2_player.y << '\n';

    // Counters
    f << l2_coinCounter << ' ' << l2_turnCounter << '\n';

    // Power state (turn‑based + frame‑based)
    f << (int)l2Power.speed << ' ' << l2Power.speedTurns << ' '
        << (int)l2Power.freeze << ' ' << l2Power.freezeTurns << ' '
        << (int)l2Power.invincible << ' ' << l2Power.invTurns << ' '
        << l2Power.invFrames << ' ' << l2Power.freezeFrames << '\n';

    // Box mummies (from treasure chest)
    f << l2_boxMummyCount << '\n';
    for (int i = 0; i < l2_boxMummyCount; ++i)
        f << l2_boxMummies[i].x << ' ' << l2_boxMummies[i].y << '\n';

    // Main enemies: two mummies + scorpion
    f << l2_mummy.x << ' ' << l2_mummy.y << '\n';
    f << l2_mummy2.x << ' ' << l2_mummy2.y << '\n';
    f << l2_scorpion.x << ' ' << l2_scorpion.y << '\n';

    // Treasure chest
    f << l2_treasureBox.x << ' ' << l2_treasureBox.y << ' '
        << (l2_treasureBoxActive ? 1 : 0) << '\n';

    // Exit portal
    f << l2_exitPortal.x << ' ' << l2_exitPortal.y << '\n';

    // Legacy coin entity (if still used)
    f << l2_coin.x << ' ' << l2_coin.y << '\n';

    return true;
}

// --------------------------------------------------------------------
// LoadLevel2State
// Restores Level 2's runtime state from the given file path.
// --------------------------------------------------------------------
static bool LoadLevel2State(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;

    // Player
    f >> l2_player.x >> l2_player.y;

    // Counters
    f >> l2_coinCounter >> l2_turnCounter;

    // Power state
    int sp, fr, iv;
    f >> sp >> l2Power.speedTurns
        >> fr >> l2Power.freezeTurns
        >> iv >> l2Power.invTurns
        >> l2Power.invFrames >> l2Power.freezeFrames;
    l2Power.speed = (sp != 0);
    l2Power.freeze = (fr != 0);
    l2Power.invincible = (iv != 0);

    // Box mummies
    f >> l2_boxMummyCount;
    if (l2_boxMummyCount < 0) l2_boxMummyCount = 0;
    if (l2_boxMummyCount > (int)(sizeof(l2_boxMummies) / sizeof(l2_boxMummies[0])))
        l2_boxMummyCount = (int)(sizeof(l2_boxMummies) / sizeof(l2_boxMummies[0]));
    for (int i = 0; i < l2_boxMummyCount; ++i)
    {
        f >> l2_boxMummies[i].x >> l2_boxMummies[i].y;
        l2_boxMummies[i].size = l2_gridStep;
    }

    // Main enemies
    f >> l2_mummy.x >> l2_mummy.y;
    f >> l2_mummy2.x >> l2_mummy2.y;
    f >> l2_scorpion.x >> l2_scorpion.y;
    l2_mummy.size = l2_gridStep;
    l2_mummy2.size = l2_gridStep;
    l2_scorpion.size = l2_gridStep;

    // Treasure chest
    int activeFlag;
    f >> l2_treasureBox.x >> l2_treasureBox.y >> activeFlag;
    l2_treasureBoxActive = (activeFlag != 0);
    l2_treasureBox.size = l2_gridStep * 0.9f;

    // Exit portal
    f >> l2_exitPortal.x >> l2_exitPortal.y;
    l2_exitPortal.size = 50.0f;

    // Legacy coin
    f >> l2_coin.x >> l2_coin.y;
    l2_coin.size = GRID_TILE_SIZE * 0.8f;

    return true;
}
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
    // AUDIO LOAD FOR LEVEL 2
    // ================================================================
    l2AudioGroup = AEAudioCreateGroup();

    l2_sfxPlayerMove = AEAudioLoadSound("Assets/audio/player.wav");
    l2_sfxChest = AEAudioLoadSound("Assets/audio/chest.wav");
    l2_sfxPowerup = AEAudioLoadSound("Assets/audio/powerup.wav");
    l2_sfxJumpscare = AEAudioLoadSound("Assets/audio/jumpscare.wav");
    l2_sfxExitDoor = AEAudioLoadSound("Assets/audio/exit.wav");
    l2_sfxGameOver = AEAudioLoadSound("Assets/audio/gameover.wav");
    l2_sfxButton = AEAudioLoadSound("Assets/audio/button.wav");
    // ================================================================

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
	l2_DoorOpenedTex = AEGfxTextureLoad("Assets/DoorOpened.png"); // opened exit portal (after collecting coin)
 
    // Load power‑up textures
    l2_ImmuneTex = AEGfxTextureLoad("Assets/Immune.png");
    l2_FreezeTex = AEGfxTextureLoad("Assets/Freeze.png");

    // Load scorpion texture
    l2_ScorpionTex = AEGfxTextureLoad("Assets/Spider.png");

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

    // ================================================================
    // STOP ALL PREVIOUS AUDIO (safe, no FMOD crash)
    // ================================================================
    AEAudioStopGroup(l2AudioGroup);
    // ================================================================

    // Always reset powerup and overlay state on entry
    l2Power = {};
    l2_paused = false;
    l2_showWin = l2_showLose = false;
    l2_initialised = false; // Force full re-init every time

    // Clear frame-based freeze
    l2Power.freezeFrames = 0;

    // Initialise jump scare
    JumpScare_Init();

    // Initialise particles
    TrailParticle_Init();

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

        // Scorpion spawn (reachability-aware)
        L2FindReachableSpawnNear(
            GRID_ROWS - 4, 2,
            playerRow, playerCol,
            8,
            px, py,
            playerRow, playerCol
        );

        l2_scorpion.x = px;
        l2_scorpion.y = py;
        l2_scorpion.size = 50.0f;
        l2_scorpion.pTex = l2_ScorpionTex;

        // --- Exit portal spawn ---
        L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        l2_exitPortal.x = px;
        l2_exitPortal.y = py;
        l2_exitPortal.size = 50.0f;

        // --- Coin spawn ---
        L2FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
        l2_coin.x = px;
        l2_coin.y = py;
        l2_coin.size = GRID_TILE_SIZE * 0.8f;
        l2_coin.r = 1.0f;
        l2_coin.g = 0.5f;
        l2_coin.b = 0.0f;

        // Spawn random power‑up
        SpawnRandomPowerup();

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
//    b. Both mummies move every 2nd turn using BFS pathfinding.
//    c. L2TickPowers() -- decrement powerup durations.
// 6. Lose check: player shares cell with either mummy AND not invincible.
//    Jump Scare is also shown.
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
    // --- Save (F5) / Load (F9) ---
    if (AEInputCheckReleased(AEVK_F5)) { if (SaveLevel2State("Assets/save2.txt")) std::cout << "Saved (Assets/save2.txt)\n"; }
    if (AEInputCheckReleased(AEVK_F9)) { if (LoadLevel2State("Assets/save2.txt")) std::cout << "Loaded (Assets/save2.txt)\n"; }

    // --- Debug overlay toggle (F1) ---
    Debug_HandleToggle();

    // ===== PER-FRAME POWER TIMERS =====
    L2TickInvFrames();
    L2TickFreezeFrames();

    // --- Win / Lose overlay input ---
    if (l2_showLose || l2_showWin)
    {
        s32 mxS, myS;
        TransformScreentoWorld(mxS, myS);

        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            // Level Select button (0,60) 280x70
            if (IsAreaClicked(0.0f, 60.0f, 280.0f, 70.0f, mxS, myS))
            {
                next = LEVELPAGE;
                l2_showLose = l2_showWin = false;
                return;
            }
            // Restart button (0,-20) 280x70
            if (IsAreaClicked(0.0f, -20.0f, 280.0f, 70.0f, mxS, myS))
            {
                next = GS_LEVEL2;
                l2_showLose = l2_showWin = false;
                return;
            }
            // Quit button (0,-100) 280x70
            if (IsAreaClicked(0.0f, -100.0f, 280.0f, 70.0f, mxS, myS))
            {
                next = GS_QUIT;
                l2_showLose = l2_showWin = false;
                return;
            }
        }

        // Keyboard handling with confirmation
        if (AEInputCheckReleased(AEVK_R))
        {
            if (AEAudioIsValidAudio(l2_sfxButton))
                AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL2, GS_LEVEL2, "Are you sure you want to restart?");
            next = CONFIRM;
            l2_showLose = false;
            return;
        }
        if (AEInputCheckReleased(AEVK_RETURN) || AEInputCheckReleased(AEVK_B))
        {
            if (AEAudioIsValidAudio(l2_sfxButton))
                AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL2, LEVELPAGE, "Are you sure you want to go to Level Select?");
            next = CONFIRM;
            l2_showLose = false;
            return;
        }
        if (AEInputCheckReleased(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
        {
            next = GS_QUIT;
            return;
        }
        return;
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
                if (AEAudioIsValidAudio(l2_sfxButton))
                    AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
                l2_paused = !l2_paused;
                return;
            }
        }
    }
    // =========================================================================

    // --- Pause toggle ---
    if (AEInputCheckReleased(AEVK_P)) { l2_paused = !l2_paused; }

    if (l2_paused)
    {
        // --- Mouse click handling (updated coordinates) ---
        s32 mxS, myS;
        TransformScreentoWorld(mxS, myS);
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            // Resume (0, 120)
            if (IsAreaClicked(0.0f, 120.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l2_sfxButton))
                    AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
                l2_paused = false;
                return;
            }
            // Restart (0, 40)
            if (IsAreaClicked(0.0f, 40.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l2_sfxButton))
                    AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
                next = GS_RESTART;
                return;
            }
            // Level Select (0, -40)
            if (IsAreaClicked(0.0f, -40.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l2_sfxButton))
                    AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
                Confirmation_Level(GS_LEVEL2, LEVELPAGE, "Are you sure you want to go to Level Select?");
                next = CONFIRM;
                return;
            }
            // Quit (0, -120)
            if (IsAreaClicked(0.0f, -120.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l2_sfxButton))
                    AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
                Confirmation_Level(GS_LEVEL2, GS_QUIT, "Are you sure you want to quit?");
                next = CONFIRM;
                return;
            }
            // Main Menu (0, -200)
            if (IsAreaClicked(0.0f, -200.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(l2_sfxButton))
                    AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
                Confirmation_Level(GS_LEVEL2, MAINMENUSTATE, "Are you sure you want to go to the Main Menu?");
                next = CONFIRM;
                return;
            }
        }

        // --- Keyboard shortcuts for the pause overlay ---
        if (AEInputCheckReleased(AEVK_P))
        {
            l2_paused = false;      // resume
            return;
        }
        if (AEInputCheckReleased(AEVK_R))
        {
            next = GS_RESTART;      // restart the level
            return;
        }
        if (AEInputCheckReleased(AEVK_B))
        {
            if (AEAudioIsValidAudio(l2_sfxButton))
                AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL2, LEVELPAGE, "Are you sure you want to go to Level Select?");
            next = CONFIRM;
            return;
        }
        if (AEInputCheckReleased(AEVK_M))
        {
            if (AEAudioIsValidAudio(l2_sfxButton))
                AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL2, MAINMENUSTATE, "Are you sure you want to go to the Main Menu?");
            next = CONFIRM;
            return;
        }
        if (AEInputCheckReleased(AEVK_ESCAPE))
        {
            if (AEAudioIsValidAudio(l2_sfxButton))
                AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL2, GS_QUIT, "Are you sure you want to quit?");
            next = CONFIRM;
            return;
        }

        return; // freeze game logic while paused
    }

    //                                                                --- PLAYER MOVEMENT START HERE ---

    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Update particle system before movement starts
    TrailParticle_Update(dt, l2_player.x, l2_player.y);

    // --- Player movement ---
    float testX = l2_player.x;
    float testY = l2_player.y;

    bool attemptedMove = false;

    if (AEInputCheckTriggered(AEVK_W)) { testY += l2_gridStep; attemptedMove = true; }
    else if (AEInputCheckTriggered(AEVK_S)) { testY -= l2_gridStep; attemptedMove = true; }
    else if (AEInputCheckTriggered(AEVK_A)) { testX -= l2_gridStep; attemptedMove = true; }
    else if (AEInputCheckTriggered(AEVK_D)) { testX += l2_gridStep; attemptedMove = true; }

    if (attemptedMove && IsTileWalkable(testX, testY))
    {
        l2_player.x = testX;
        l2_player.y = testY;
        l2_playerMoved = true;

        // PLAY MOVEMENT AUDIO
        if (AEAudioIsValidAudio(l2_sfxPlayerMove))
            AEAudioPlay(l2_sfxPlayerMove, l2AudioGroup, 1.0f, 1.0f, 0);

        // Particle appear only when player move
        TrailParticle_OnPlayerMoved(l2_player.x, l2_player.y);
    }

    //                                                                --- PLAYER MOVEMENT END HERE ---

    // ===== POWER-UP PICKUP =====
    if (l2_powerupActive &&
        fabsf(l2_player.x - l2_powerup.x) < 1.0f &&
        fabsf(l2_player.y - l2_powerup.y) < 1.0f)
    {
        // play powerup audio
        if (AEAudioIsValidAudio(l2_sfxPowerup))
            AEAudioPlay(l2_sfxPowerup, l2AudioGroup, 1.0f, 1.0f, 0);

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
            if (AEAudioIsValidAudio(l2_sfxButton))
                AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
        }

        // Mummy & Scorpion movement (frozen if freezeFrames > 0)
        if (l2_turnCounter % 2 == 0 && l2Power.freezeFrames <= 0)
        {
            // ---- BFS: find the next step on the shortest path from (startR,startC)
            // to (goalR,goalC), avoiding wall tiles (value == 1).
            // Writes the first-step grid coords into outR[0]/outC[0].
            // Returns false if already at goal or no path exists.
            auto L2BFSNextStep = [&](int startR, int startC, int goalR, int goalC,
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

            // World-space coords of the player for BFS goal
            int playerR, playerC;
            WorldToGrid(l2_player.x, l2_player.y, playerR, playerC);

            // ========== MUMMY 1 BFS MOVEMENT ==========
            {
                int mR, mC, nR[1], nC[1];
                WorldToGrid(l2_mummy.x, l2_mummy.y, mR, mC);
                if (L2BFSNextStep(mR, mC, playerR, playerC, nR, nC))
                {
                    float nx, ny;
                    GridToWorldCenter(nR[0], nC[0], nx, ny);

                    // Invincibility check - prevent moving onto player if immune
                    int pR, pC;
                    WorldToGrid(l2_player.x, l2_player.y, pR, pC);
                    if (nR[0] == pR && nC[0] == pC && L2IsInvincibleNow())
                    {
                        // Stay in place this turn
                    }
                    else
                    {
                        // Ensure target cell is not occupied by another main enemy or scorpion
                        bool blocked =
                            (fabsf(l2_mummy2.x - nx) < 1.0f && fabsf(l2_mummy2.y - ny) < 1.0f) ||
                            (fabsf(l2_scorpion.x - nx) < 1.0f && fabsf(l2_scorpion.y - ny) < 1.0f);
                        if (!blocked) { l2_mummy.x = nx; l2_mummy.y = ny; }
                    }
                }
            }

            // ========== MUMMY 2 BFS MOVEMENT ==========
            {
                int mR, mC, nR[1], nC[1];
                WorldToGrid(l2_mummy2.x, l2_mummy2.y, mR, mC);
                if (L2BFSNextStep(mR, mC, playerR, playerC, nR, nC))
                {
                    float nx, ny;
                    GridToWorldCenter(nR[0], nC[0], nx, ny);

                    // Invincibility check
                    int pR, pC;
                    WorldToGrid(l2_player.x, l2_player.y, pR, pC);
                    if (nR[0] == pR && nC[0] == pC && L2IsInvincibleNow())
                    {
                        // Do nothing
                    }
                    else
                    {
                        bool blocked =
                            (fabsf(l2_mummy.x - nx) < 1.0f && fabsf(l2_mummy.y - ny) < 1.0f) ||
                            (fabsf(l2_scorpion.x - nx) < 1.0f && fabsf(l2_scorpion.y - ny) < 1.0f);
                        if (!blocked) { l2_mummy2.x = nx; l2_mummy2.y = ny; }
                    }
                }
            }

            // ========== SCORPION BFS MOVEMENT ==========
            {
                int mR, mC, nR[1], nC[1];
                WorldToGrid(l2_scorpion.x, l2_scorpion.y, mR, mC);
                if (L2BFSNextStep(mR, mC, playerR, playerC, nR, nC))
                {
                    float nx, ny;
                    GridToWorldCenter(nR[0], nC[0], nx, ny);

                    // Invincibility check
                    int pR, pC;
                    WorldToGrid(l2_player.x, l2_player.y, pR, pC);
                    if (nR[0] == pR && nC[0] == pC && L2IsInvincibleNow())
                    {
                        // Do nothing
                    }
                    else
                    {
                        bool blocked =
                            (fabsf(l2_mummy.x - nx) < 1.0f && fabsf(l2_mummy.y - ny) < 1.0f) ||
                            (fabsf(l2_mummy2.x - nx) < 1.0f && fabsf(l2_mummy2.y - ny) < 1.0f);
                        if (!blocked) { l2_scorpion.x = nx; l2_scorpion.y = ny; }
                    }
                }
            }
        }

        L2TickPowers();
        l2_playerMoved = false;

        // ====== BOX MUMMY AI: BFS chase player every turn ======
        if (l2Power.freezeFrames <= 0)
        {
            int playerR2, playerC2;
            WorldToGrid(l2_player.x, l2_player.y, playerR2, playerC2);

            for (int i = 0; i < l2_boxMummyCount; ++i)
            {
                L2BoxMummy& bm = l2_boxMummies[i];

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

                // Invincibility check for box mummies
                int pR, pC;
                WorldToGrid(l2_player.x, l2_player.y, pR, pC);
                if (tr == pR && tc == pC && L2IsInvincibleNow())
                {
                    continue; // Skip moving this box mummy this turn
                }

                // Block if occupied by a main enemy or another box mummy
                bool blocked =
                    (fabsf(l2_mummy.x - nx) < 1.0f && fabsf(l2_mummy.y - ny) < 1.0f) ||
                    (fabsf(l2_mummy2.x - nx) < 1.0f && fabsf(l2_mummy2.y - ny) < 1.0f) ||
                    (fabsf(l2_scorpion.x - nx) < 1.0f && fabsf(l2_scorpion.y - ny) < 1.0f);
                for (int j = 0; !blocked && j < l2_boxMummyCount; ++j)
                    if (j != i &&
                        fabsf(l2_boxMummies[j].x - nx) < 1.0f &&
                        fabsf(l2_boxMummies[j].y - ny) < 1.0f)
                        blocked = true;

                if (!blocked) { bm.x = nx; bm.y = ny; }
            }
        }
    }

    // ====== UPDATE JUMP SCARE ANIMATION =======
    JumpScare_Update(); // Before lose conditions

    // Ensure jumpscare is drawn before level resets
    static bool pendingGameOverReset = false;

    // ===== LOSE CHECK =====
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
    if (pendingGameOverReset && !JumpScare_IsActive())
    {
        // play gameover
        if (AEAudioIsValidAudio(l2_sfxGameOver))
            AEAudioPlay(l2_sfxGameOver, l2AudioGroup, 1.0f, 1.0f, 0);

        ResetLevel2();
        pendingGameOverReset = false;
        std::cout << "Jump scare finished, resetting level...\n";
        l2_showLose = true;
        return; // skip remaining update logic this frame
    }

    // ===== WIN (EXIT DOOR) =====
    if (fabsf(l2_player.x - l2_exitPortal.x) < 1.0f &&
        fabsf(l2_player.y - l2_exitPortal.y) < 1.0f)
    {
        if (l2_coinCounter >= 1)
        {
            if (AEAudioIsValidAudio(l2_sfxExitDoor))
                AEAudioPlay(l2_sfxExitDoor, l2AudioGroup, 1.0f, 1.0f, 0);

            printf("L2: You Escaped!\n");

            gLastLevelPlayed = 2;  // tell WinPage we came from Level 2

            next = GS_WIN;
        }
        else
        {
            // Player hasn't collected a coin yet -- show reminder
            std::snprintf(l2_popupMsg, sizeof(l2_popupMsg), "Collect a coin first!");
            l2_popupFrames = 120;
        }
    }

    // Legacy coin entity collect
    if (fabsf(l2_player.x - l2_coin.x) < 1.0f &&
        fabsf(l2_player.y - l2_coin.y) < 1.0f)
    {
        ++l2_coinCounter;
        printf("L2 Coin! Total: %d\n", l2_coinCounter);
        l2_coin.x = l2_coin.y = 2000.0f;
        if (AEAudioIsValidAudio(l2_sfxButton))
            AEAudioPlay(l2_sfxButton, l2AudioGroup, 1.0f, 1.0f, 0);
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

    // --- Floor tiles (value == 0 and value == 4) ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(l2_FloorTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
        for (int col = 0; col < GRID_COLS; col++)
            if (level[row][col] == 0 || level[row][col] == 4)
            {
                float x, y; GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }

    // --- Coin tiles (value == 4) ---
    AEGfxTextureSet(l2_coin.pTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
        for (int col = 0; col < GRID_COLS; col++)
            if (level[row][col] == 4)
            {
                float x, y; GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, l2_coin.size, l2_coin.size);
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

    // Scorpion (scorpion.png)
    AEGfxTextureSet(l2_ScorpionTex, 0, 0);
    AEMtx33Scale(&scale, l2_scorpion.size, l2_scorpion.size);
    AEMtx33Trans(&trans, l2_scorpion.x, l2_scorpion.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

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

    // Render power‑up if active
    if (l2_powerupActive)
    {
        AEGfxTextureSet((l2_powerupType == L2_PWR_IMMUNE) ? l2_ImmuneTex : l2_FreezeTex, 0, 0);
        AEMtx33Scale(&scale, l2_powerup.size, l2_powerup.size);
        AEMtx33Trans(&trans, l2_powerup.x, l2_powerup.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- Exit portal (Exit.png) ---
    AEGfxTextureSet((l2_coinCounter > 0) ? l2_DoorOpenedTex : l2_exitPortal.pTex, 0, 0);
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

    // === Draw particles ===
    TrailParticle_Draw();

    // === Jump Scare ===
    JumpScare_Draw();

    // ===== HUD for active power-ups (top-left) =====
    if (l2Power.invFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "IMMUNE  %.1fs", l2Power.invFrames / 60.0f);
        AEGfxPrint(fontId, buf, -0.95f, 0.74f, 1.0f, 0.90f, 0.90f, 0.20f, 1.0f);
    }
    if (l2Power.freezeFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "FREEZE  %.1fs", l2Power.freezeFrames / 60.0f);
        AEGfxPrint(fontId, buf, -0.95f, 0.82f, 1.0f, 0.60f, 0.85f, 1.00f, 1.0f);
    }

    // Coin counter HUD
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Coins: %d", l2_coinCounter);
        AEGfxPrint(fontId, buf, -0.95f, 0.90f, 1.2f, 0.60f, 0.15f, 0.20f, 1.0f);
    }

    // ====== Hint: collect a coin before escaping (shown until first coin collected) ======
    if (l2_coinCounter == 0)
    {
        const char* hint = "Coins to escape: 0/1";
        float hw, hh;
        AEGfxGetPrintSize(fontId, hint, 1.2f, &hw, &hh);
        AEGfxPrint(fontId, hint, -hw * 0.5f, 0.90f, 1.2f, 0.60f, 0.15f, 0.20f, 1.0f);
    }

    // Treasure box popup message
    if (l2_popupFrames > 0)
    {
        float alpha = (l2_popupFrames < 60) ? l2_popupFrames / 60.0f : 1.0f;
        float popupScale = 0.85f;
        float pw, ph;
        AEGfxGetPrintSize(fontId, l2_popupMsg, popupScale, &pw, &ph);
        float centeredX = -pw * 0.5f;
        AEGfxPrint(fontId, l2_popupMsg, centeredX, 0.4f, popupScale, 1.0f, 1.0f, 0.4f, alpha);
    }

    // ===== Pause button (top-right) =====
    DrawPauseButton();

    // === Jump Scare ===
    JumpScare_Draw();

    // ---- Debug overlay: fill info struct and call shared draw function ----
    // Press F1 in-game to toggle on / off.
    if (Debug_IsActive())
    {
        DebugEntityInfo dbg;
        dbg.playerX = l2_player.x;
        dbg.playerY = l2_player.y;
        dbg.playerSize = l2_player.size;

        dbg.hasMummy1 = true;
        dbg.mummy1X = l2_mummy.x;
        dbg.mummy1Y = l2_mummy.y;
        dbg.mummy1Size = l2_mummy.size;

        dbg.hasMummy2 = true;
        dbg.mummy2X = l2_mummy2.x;
        dbg.mummy2Y = l2_mummy2.y;
        dbg.mummy2Size = l2_mummy2.size;

        dbg.hasScorpion = true;
        dbg.scorpionX = l2_scorpion.x;
        dbg.scorpionY = l2_scorpion.y;
        dbg.scorpionSize = l2_scorpion.size;

        dbg.exitX = l2_exitPortal.x;
        dbg.exitY = l2_exitPortal.y;
        dbg.exitSize = l2_exitPortal.size;
        dbg.coinX = l2_coin.x;
        dbg.coinY = l2_coin.y;
        dbg.coinSize = l2_coin.size;

        dbg.powerupActive = l2_powerupActive;
        dbg.powerupX = l2_powerup.x;  dbg.powerupY = l2_powerup.y;
        dbg.powerupSize = l2_powerup.size;

        dbg.treasureBoxActive = l2_treasureBoxActive;
        dbg.treasureBoxX = l2_treasureBox.x;
        dbg.treasureBoxY = l2_treasureBox.y;
        dbg.treasureBoxSize = l2_treasureBox.size;

        dbg.boxMummyCount = l2_boxMummyCount;
        for (int i = 0; i < l2_boxMummyCount; ++i)
        {
            dbg.boxMummyX[i] = l2_boxMummies[i].x;
            dbg.boxMummyY[i] = l2_boxMummies[i].y;
            dbg.boxMummySize[i] = l2_boxMummies[i].size;
        }

        dbg.coinCounter = l2_coinCounter;
        dbg.turnCounter = l2_turnCounter;
        dbg.invincibleActive = l2Power.invincible;
        dbg.invFrames = l2Power.invFrames;
        dbg.freezeActive = l2Power.freeze;
        dbg.freezeFrames = l2Power.freezeFrames;
        dbg.speedActive = l2Power.speed;
        dbg.speedTurns = l2Power.speedTurns;
        dbg.isPaused = l2_paused;
        dbg.isWin = l2_showWin;
        dbg.isLose = l2_showLose;
        dbg.popupFrames = l2_popupFrames;
        dbg.tileSize = GRID_TILE_SIZE;

        Debug_DrawOverlay(dbg);
    }

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

    // ==================== AUDIO UNLOAD (LEVEL 2) =====================
    if (AEAudioIsValidAudio(l2_sfxPlayerMove))
        AEAudioUnloadAudio(l2_sfxPlayerMove);

    if (AEAudioIsValidAudio(l2_sfxChest))
        AEAudioUnloadAudio(l2_sfxChest);

    if (AEAudioIsValidAudio(l2_sfxPowerup))
        AEAudioUnloadAudio(l2_sfxPowerup);

    if (AEAudioIsValidAudio(l2_sfxJumpscare))
        AEAudioUnloadAudio(l2_sfxJumpscare);

    if (AEAudioIsValidAudio(l2_sfxExitDoor))
        AEAudioUnloadAudio(l2_sfxExitDoor);

    if (AEAudioIsValidAudio(l2_sfxGameOver))
        AEAudioUnloadAudio(l2_sfxGameOver);

    AEAudioUnloadAudioGroup(l2AudioGroup);
    // =================================================================

    // ========== ORIGINAL TEXTURE & MESH CLEANUP ==========
    AEGfxTextureUnload(l2_player.pTex);
    AEGfxTextureUnload(l2_DesertBlockTex);
    AEGfxTextureUnload(l2_FloorTex);
    AEGfxTextureUnload(l2_mummy.pTex);
    AEGfxTextureUnload(l2_mummy2.pTex);
    AEGfxTextureUnload(l2_coin.pTex);
    AEGfxTextureUnload(l2_exitPortal.pTex);
    AEGfxTextureUnload(l2_DoorOpenedTex);

    // Unload power-up & scorpion textures
    AEGfxTextureUnload(l2_ImmuneTex);
    AEGfxTextureUnload(l2_FreezeTex);
    AEGfxTextureUnload(l2_ScorpionTex);

    // Unload treasure box texture
    if (l2_treasureBoxTex) { AEGfxTextureUnload(l2_treasureBoxTex); l2_treasureBoxTex = nullptr; }

    // Unload Jump Scare
    JumpScare_Unload();

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }

    l2_initialised = false;
}