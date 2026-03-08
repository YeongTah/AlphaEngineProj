/* Start Header ******************************************************************
/*!
\file Level1.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par sharonjooai.lim@digipen.edu
\date January, 26, 2026
\brief This file defines the function Load, Initialize, Update, Draw, Free, Unload
 to produce the level in the game and manage their own counters loaded from text
 files.
Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header ****************************************************************** */
#include "leveleditor.hpp"
#include "pch.h"
#include "GridUtils.h"
#include "Level1.h"
#include "gamestatemanager.h"
#include "GameStateList.h"
#include "Main.h"
#include <iostream>
#include <fstream>
#include <cmath> /* fabsf -ths */
#include <cstring> /* strlen -ths */
#include <cstdio>  /* snprintf for HUD text -ths */

/* ------------------------------ NEW: minimal, compatible additions --------------------------------
 Everything added in this file is kept local to Level1 and uses your existing engine and globals. -ths
 We do NOT modify GSM/states or other modules. All new comments end with '-ths'. -ths
--------------------------------------------------------------------------------------------------- */

/* NEW: forward decls to use functions/vars defined in leveleditor.cpp safely without editing headers -ths */
extern void WorldToGrid(float worldX, float worldY, int& outRow, int& outCol); // -ths
extern void GridToWorldCenter(int row, int col, float& outX, float& outY); // -ths
extern bool canMove(float nextX, float nextY); // -ths
extern int level[18][32]; // -ths
/*extern int ROWS, COLS; */ // -ths

/* NEW: forward decls to reuse mouse + click helpers (already in your project) -ths */
extern void TransformScreentoWorld(s32& mouseX, s32& mouseY); // -ths
extern bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height,
    float click_x, float click_y); // -ths

// --- Variables declaration start here --- (original)
static bool level1_initialised = false; // Flag to prevent re-initialisation mid-level
Entity player; // The player entity (position, size, color, texture)
Entity mummy; // The main enemy mummy entity
Entity exitPortal; // The exit goal entity; reaching it triggers the win condition
Entity coin; // The legacy single coin entity (in addition to tile-based coins)
Entity wall; // Unused legacy colored rectangle wall (replaced by grid-based walls)
int coinCounter = 0; // Tracks total coins collected in this level session
int turnCounter = 0; // Counts player moves; used to throttle mummy movement (moves every 2nd player turn)
AEGfxTexture* gDesertBlockTex = nullptr; // Texture for wall/non-walkable tiles (DesertBlock.png)
static AEGfxTexture* gFloorTex = nullptr; // Texture for floor/walkable tiles (Floor.png)
int level1_counter = 0; // Countdown timer; when it hits 0 the level ends (legacy)
int live1_counter = 3; // Player lives count (adjust here to change starting lives)
bool playerMoved = false; // Set to true when the player makes a valid move this frame
float gridStep = 50.0f; // World units per one grid cell step (matches GRID_TILE_SIZE)
float nextX = player.x; // Stores the player's proposed next X position before validation
float nextY = player.y; // Stores the player's proposed next Y position before validation
// --- Variables declaration end here ---

// ========================== NEW: enemy variants & powerups (local-only) ===========================
// Enemy types for minimal variety without touching GSM -ths
enum EnemyType { ENEMY_SCOUT = 1, ENEMY_BRUTE = 2 }; // your mummy remains as-is; these are extra -ths
struct ExtraEnemy { float x, y, size; float r, g, b; EnemyType type; }; // kept simple -ths
static ExtraEnemy gExtraEnemies[8]; // Fixed-size array of extra enemies (max 8) -ths
static int gExtraEnemyCount = 0; // Current number of active extra enemies -ths

// ----------------------------------------------------------------------------
// SpawnExtraEnemy
// Adds a new extra enemy to gExtraEnemies[] at world position (x, y).
// Scout = orange, Brute = purple. Does nothing if the array is already full.
// ----------------------------------------------------------------------------
static void SpawnExtraEnemy(float x, float y, EnemyType t)
{
    if (gExtraEnemyCount >= (int)(sizeof(gExtraEnemies) / sizeof(gExtraEnemies[0]))) return;
    ExtraEnemy& e = gExtraEnemies[gExtraEnemyCount++];
    e.x = x; e.y = y; e.size = gridStep; e.type = t;
    if (t == ENEMY_SCOUT) { e.r = 0.9f; e.g = 0.5f; e.b = 0.0f; } // orange scout -ths
    if (t == ENEMY_BRUTE) { e.r = 0.6f; e.g = 0.2f; e.b = 0.8f; } // purple brute -ths
}

// Powerup tile values encoded in the level[][] grid (set via level editor).
// These values are read in Level1_Update when the player steps on their tile.
enum PowerTile {
    TILE_POWER_SPEED = 5, // Grants +1 extra tile per move for 4 turns -ths
    TILE_POWER_FREEZE = 6, // Enemies skip moving for 3 turns -ths
    TILE_POWER_INVINCIBLE = 7, // Player ignores enemy touch for 4 turns (turn-based) -ths
    TILE_POWER_GOLD_5S = 8 // Black buff block: ~5 seconds frame-based invincibility -ths
};

// Tracks all active powerup durations for the player this level session.
static struct PowerState {
    bool speed = false; int speedTurns = 0; // Speed boost state & remaining turns -ths
    bool freeze = false; int freezeTurns = 0; // Freeze state & remaining turns -ths
    bool invincible = false; int invTurns = 0; // Turn-based invincibility -ths
    int invFrames = 0; // Frame-countdown invincibility (~300 frames = ~5 seconds) -ths

    // ===================== ADDED: frame-based freeze (3 seconds) ===================== -ths
    int freezeFrames = 0; // Counts down in frames for real-time freeze (~180 @60fps) -ths
} gPower;

// Powerup grant helpers -- call these to activate the matching powerup -ths
static void GiveSpeed(int turns) { gPower.speed = true; gPower.speedTurns = turns; }
static void GiveFreeze(int turns) { gPower.freeze = true; gPower.freezeTurns = turns; }
static void GiveInvincibleTurns(int turns) { gPower.invincible = true; gPower.invTurns = turns; }
static void GiveInvincibleFrames(int frames) { if (frames > gPower.invFrames) gPower.invFrames = frames; }

// Returns true if the player is currently protected from any enemy (either turn- or frame-based). -ths
static bool IsInvincibleNow() { return gPower.invincible || (gPower.invFrames > 0); }

// ----------------------------------------------------------------------------
// TickPowers
// Decrements all turn-based powerup counters by 1 each time the player moves.
// Deactivates the powerup when the counter reaches zero.
// Call this once per player turn (inside the playerMoved block).
// ----------------------------------------------------------------------------
static void TickPowers()
{
    if (gPower.speed && --gPower.speedTurns <= 0) gPower.speed = false;
    if (gPower.freeze && --gPower.freezeTurns <= 0) gPower.freeze = false;
    if (gPower.invincible && --gPower.invTurns <= 0) gPower.invincible = false;
}

// ----------------------------------------------------------------------------
// TickFramePowers
// Decrements the frame-based invincibility counter by 1 per frame.
// Call this every Update frame when the game is not paused.
// When gPower.invFrames reaches 0, the immunity expires automatically.
// ----------------------------------------------------------------------------
static void TickFramePowers()
{
    if (gPower.invFrames > 0) --gPower.invFrames; // counts down at 60 FPS; set to 300 for ~5 seconds -ths
}

// ===================== ADDED: TickFreezeFrames (real-time freeze) ===================== -ths
static void TickFreezeFrames() { if (gPower.freezeFrames > 0) --gPower.freezeFrames; } // -ths

// ===================== ADDED: Random Power-Up Entity & helpers ===================== -ths
enum PowerupType { PWR_IMMUNE = 0, PWR_FREEZE = 1 }; // -ths
static Entity gPowerup;                // power-up pickup on the map -ths
static bool   gPowerupActive = false;  // active flag -ths
static int    gPowerupType = PWR_IMMUNE; // current type -ths
static AEGfxTexture* gImmuneTex = nullptr; // Immune.png -ths
static AEGfxTexture* gFreezeTex = nullptr; // Freeze.png -ths

// Returns a random integer in [min, max] using engine RNG -ths
static int RandInt(int minV, int maxV)
{
    float t = AERandFloat(); // [0..1] -ths
    int span = (maxV - minV + 1);
    return minV + (int)(t * (float)span);
}

// Find a random walkable (value==0) cell and place the power-up there -ths
static void SpawnRandomPowerup()
{
    // randomly decide the type 50/50 -ths
    gPowerupType = (AERandFloat() < 0.5f) ? PWR_IMMUNE : PWR_FREEZE; // -ths

    // try up to N times to find a free cell -ths
    for (int tries = 0; tries < 128; ++tries) // -ths
    {
        int r = RandInt(0, GRID_ROWS - 1); // -ths
        int c = RandInt(0, GRID_COLS - 1); // -ths
        if (level[r][c] == 0) // walkable -ths
        {
            float x, y;
            GridToWorldCenter(r, c, x, y); // -ths
            gPowerup.x = x; gPowerup.y = y;
            gPowerup.size = 30.0f; // small icon -ths
            gPowerupActive = true;
            return;
        }
    }
    // fallback off-screen if none found (unlikely) -ths
    gPowerupActive = false; // -ths
}

// ========================== NEW: overlay flags + button layout (file-scope) =======================
static bool gPaused = false; // True while P-key pause is active; Update skips game logic -ths
static bool gShowLose = false; // True while the Lose overlay is displayed -ths
static bool gShowWin = false; // True while the Win overlay is displayed -ths
// World-space center positions and dimensions for Retry / Exit buttons on overlays -ths
static float kBtnRetryX = -200.0f;
static float kBtnRetryY = -130.0f;
static float kBtnExitX = 200.0f;
static float kBtnExitY = -130.0f;
static float kBtnW = 280.0f;
static float kBtnH = 90.0f;

// ========================== NEW: world<- >NDC helpers & UI draw helpers ============================
// Converts a world X coordinate to Normalized Device Coordinates [-1, 1]. -ths
static inline float ToNDCX(float worldX) { return worldX / ((float)AEGfxGetWindowWidth() * 0.5f); }
// Converts a world Y coordinate to Normalized Device Coordinates [-1, 1]. -ths
static inline float ToNDCY(float worldY) { return worldY / ((float)AEGfxGetWindowHeight() * 0.5f); }

// ----------------------------------------------------------------------------
// CenteredTextX
// Calculates the NDC left-edge X position needed to visually center a text
// string of 'text' around 'centerWorldX' when printed at 'scale'.
// Used to center-align labels on overlay buttons.
// ----------------------------------------------------------------------------
static float CenteredTextX(float centerWorldX, const char* text, float scale)
{
    const float ndcPerChar = 0.0165f * scale; // empirically tuned for Roboto@32 -ths
    float halfText = 0.5f * ndcPerChar * (float)std::strlen(text);
    return ToNDCX(centerWorldX) - halfText; // left-x for AEGfxPrint -ths
}

// ----------------------------------------------------------------------------
// DrawButtonRect
// Draws a solid colored rectangle at world position (cx, cy) with dimensions
// (w x h) using the shared pMesh. Used for overlay buttons (Retry, Exit).
// ----------------------------------------------------------------------------
static void DrawButtonRect(float cx, float cy, float w, float h, float r, float g, float b)
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(r, g, b, 1.0f);
    AEMtx33 s, t, m;
    AEMtx33Scale(&s, w, h);
    AEMtx33Trans(&t, cx, cy);
    AEMtx33Concat(&m, &t, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
}

// ----------------------------------------------------------------------------
// SnapToGridCenter
// Converts an arbitrary world position (inX, inY) to the exact center of the
// grid cell it occupies. Prevents entities from sitting on grid-line borders.
// ----------------------------------------------------------------------------
static void SnapToGridCenter(float inX, float inY, float& outX, float& outY)
{
    int rr, cc;
    WorldToGrid(inX, inY, rr, cc);
    GridToWorldCenter(rr, cc, outX, outY);
}

// ========================== NEW: Save / Load (local) ==============================================
// ----------------------------------------------------------------------------
// SaveLevel1State
// Writes the current Level 1 runtime state (player position, counters, active
// powerups, and extra enemy list) to 'path' as plain text.
// Returns true on success, false if the file cannot be opened.
// Triggered by F5 in Level1_Update.
// ----------------------------------------------------------------------------
static bool SaveLevel1State(const char* path)
{
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << player.x << ' ' << player.y << '\n';
    f << coinCounter << ' ' << turnCounter << '\n';
    f << (int)gPower.speed << ' ' << gPower.speedTurns << ' '
        << (int)gPower.freeze << ' ' << gPower.freezeTurns << ' '
        << (int)gPower.invincible << ' ' << gPower.invTurns << ' '
        << gPower.invFrames << '\n';
    f << gExtraEnemyCount << '\n';
    for (int i = 0; i < gExtraEnemyCount; ++i)
        f << (int)gExtraEnemies[i].type << ' ' << gExtraEnemies[i].x << ' ' << gExtraEnemies[i].y << '\n';
    return true;
}

// ----------------------------------------------------------------------------
// LoadLevel1State
// Reads a previously saved Level 1 state from 'path' and restores player
// position, counters, powerup durations, and extra enemy positions.
// Returns true on success, false if the file cannot be opened.
// Triggered by F9 in Level1_Update.
// ----------------------------------------------------------------------------
static bool LoadLevel1State(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;
    f >> player.x >> player.y;
    f >> coinCounter >> turnCounter;
    int sp, fr, iv;
    f >> sp >> gPower.speedTurns >> fr >> gPower.freezeTurns >> iv >> gPower.invTurns >> gPower.invFrames;
    gPower.speed = (sp != 0);
    gPower.freeze = (fr != 0);
    gPower.invincible = (iv != 0);
    f >> gExtraEnemyCount; if (gExtraEnemyCount < 0) gExtraEnemyCount = 0;
    if (gExtraEnemyCount > (int)(sizeof(gExtraEnemies) / sizeof(gExtraEnemies[0])))
        gExtraEnemyCount = (int)(sizeof(gExtraEnemies) / sizeof(gExtraEnemies[0]));
    for (int i = 0; i < gExtraEnemyCount; ++i)
    {
        int t; f >> t >> gExtraEnemies[i].x >> gExtraEnemies[i].y;
        gExtraEnemies[i].type = (EnemyType)t; gExtraEnemies[i].size = gridStep;
        if (gExtraEnemies[i].type == ENEMY_SCOUT) { gExtraEnemies[i].r = 0.9f; gExtraEnemies[i].g = 0.5f; gExtraEnemies[i].b = 0.0f; }
        if (gExtraEnemies[i].type == ENEMY_BRUTE) { gExtraEnemies[i].r = 0.6f; gExtraEnemies[i].g = 0.2f; gExtraEnemies[i].b = 0.8f; }
    }
    return true;
}

// ========================== NEW: ensure at least one buff tile (8) exists =========================
// ----------------------------------------------------------------------------
// HasBuffTile8
// Scans the entire level[][] grid and returns true if any cell has value 8
// (the black immunity buff block).
// ----------------------------------------------------------------------------
static bool HasBuffTile8()
{
    for (int r = 0; r < GRID_ROWS; ++r)
        for (int c = 0; c < GRID_COLS; ++c)
            if (level[r][c] == 8) return true;
    return false;
}

// ----------------------------------------------------------------------------
// EnsureBuffTilePresent
// Guarantees that at least one tile-value-8 (immunity buff) exists in the grid.
// If none is found, it searches outward from the grid center for an empty cell
// (value 0) and places a buff tile there, then saves it to the level file.
// Falls back to forcing the center cell if no empty cell is found nearby.
// Called during Level1_Load to enforce the design requirement.
// ----------------------------------------------------------------------------
static void EnsureBuffTilePresent()
{
    if (HasBuffTile8()) return; // buff already in level, nothing to do -ths
    int midR = GRID_ROWS / 2, midC = GRID_COLS / 2;
    for (int dr = -2; dr <= 2; ++dr)
    {
        for (int dc = -2; dc <= 2; ++dc)
        {
            int rr = midR + dr, cc = midC + dc;
            if (rr >= 0 && rr < GRID_ROWS && cc >= 0 && cc < GRID_COLS && level[rr][cc] == 0)
            {
                level[rr][cc] = 8; // place black buff tile -ths
                print_file(); // persist the change to the .txt file -ths
                return;
            }
        }
    }
    // Fallback: force center cell if no empty cell was found -ths
    level[midR][midC] = 8;
    print_file();
}

// ----------------------------------------------------------------------------
// LoadLevelTxt <-- THIS IS THE FUNCTION THAT READS THE LEVEL FILE
// Reads "Assets/level1.txt" and fills the shared level[GRID_ROWS][GRID_COLS]
// array with tile values.
//
// File format: each cell is written as <value>, (number then comma).
// Rows are separated by newlines. Example row: 0,1,0,0,1,0,...
//
// Tile value meanings (defined in leveleditor.cpp Objects enum):
// 0 = empty / walkable floor
// 1 = NON_WALKABLE wall (rendered as DesertBlock)
// 2 = PLAYER_SPAWN (reserved; spawn logic uses FindFreeSpawnCell instead)
// 3 = ENEMY_SPAWN (reserved; mummy spawn also uses FindFreeSpawnCell)
// 4 = COIN tile (collected when player steps on it; tile becomes 0)
// 5 = Speed powerup tile
// 6 = Freeze powerup tile
// 7 = Invincibility powerup tile (turn-based)
// 8 = Immunity buff block (~5 seconds frame-based invincibility)
//
// If the file cannot be opened, all cells are set to 0 (open map, no walls).
// ----------------------------------------------------------------------------
static void LoadLevelTxt()
{
    const char* path = "Assets/level1.txt";
    std::ifstream is(path);
    if (!is.is_open())
    {
        std::cout << "Level1: Could not open " << path << " - grid will be all zeros\n";
        for (int r = 0; r < GRID_ROWS; ++r)
            for (int c = 0; c < GRID_COLS; ++c)
                level[r][c] = 0;
        return;
    }
    int tile;
    char comma;
    // Read each value-comma pair and store it in level[row][col]
    for (int row = 0; row < GRID_ROWS; ++row)
        for (int col = 0; col < GRID_COLS; ++col)
            if (is >> tile >> comma)
                level[row][col] = tile;
            else
                level[row][col] = 0; // fallback for truncated files
    is.close();
    std::cout << "Level1: Loaded grid from " << path << "\n";
}

// ----------------------------------------------------------------------------
// Level1_Load
// Called ONCE when entering Level 1 (before the game loop starts).
// Responsibilities:
// 1. Calls LoadLevelTxt() to populate level[][] from "Assets/level1.txt".
// 2. Loads all textures needed for this level (player, wall, floor, mummy,
// coin, exit portal).
// 3. Creates the shared pMesh (unit square) used for all rendering.
// 4. Spawns two extra enemies (Scout near top-left, Brute near bottom-right)
// snapped to grid cell centers.
// ----------------------------------------------------------------------------
void Level1_Load()
{
    std::cout << "Level1:Load\n";
    // Step 1: Load the tile map from disk into level[][]
    LoadLevelTxt();

    // Step 2: Load entity textures from Assets/
    player.pTex = AEGfxTextureLoad("Assets/explorer.png"); // player sprite
    gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png"); // wall tile texture
    gFloorTex = AEGfxTextureLoad("Assets/Floor.png"); // floor tile texture
    mummy.pTex = AEGfxTextureLoad("Assets/Enemy.png"); // main mummy texture
    coin.pTex = AEGfxTextureLoad("Assets/Coin.png"); // legacy coin texture
    exitPortal.pTex = AEGfxTextureLoad("Assets/Exit.png"); // exit portal texture

    // ====== ADDED: load power-up textures (immune / freeze) ====== -ths
    gImmuneTex = AEGfxTextureLoad("Assets/Immune.png"); // -ths
    gFreezeTex = AEGfxTextureLoad("Assets/Freeze.png"); // -ths

    // Step 3: Create the unit square mesh used to draw all sprites and tiles
    pMesh = CreateSquareMesh();

    // Step 4: Spawn extra enemy entities at far grid cells
    gExtraEnemyCount = 0;
    float ex, ey;
    GridToWorldCenter(2, 2, ex, ey); // near top-left -ths
    SpawnExtraEnemy(ex, ey, ENEMY_SCOUT); // orange scout -ths
    GridToWorldCenter(GRID_ROWS - 3, GRID_COLS - 3, ex, ey); // near bottom-right -ths
    SpawnExtraEnemy(ex, ey, ENEMY_BRUTE); // purple brute -ths
}

// ----------------------------------------------------------------------------
// FindFreeSpawnCell
// Searches outward from (startRow, startCol) in expanding square rings to find
// the nearest empty (value == 0) grid cell that is at least 'minDist' Manhattan
// distance away from (avoidRow, avoidCol).
//
// Parameters:
// startRow/Col - center of search, typically a "preferred" spawn area
// outX / outY - receives the world-space center of the found cell
// avoidRow/Col - grid cell to keep away from (e.g. player spawn); -1 to skip
// minDist - minimum Manhattan distance from avoidRow/Col
// maxRadius - how many rings to search before giving up (then uses fallback)
//
// Used by Level1_Initialize and ResetLevel1 to place player, mummy, coin, and exit
// at safe, non-overlapping positions without hardcoding coordinates.
// ----------------------------------------------------------------------------
static void FindFreeSpawnCell(int startRow, int startCol, float& outX, float& outY,
    int avoidRow = -1, int avoidCol = -1, int minDist = 0, int maxRadius = 15)
{
    // Clamp start cell to grid bounds
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
                // Only check cells on the outer ring of the current radius
                if (abs(dr) != radius && abs(dc) != radius) continue;
                int r = startRow + dr;
                int c = startCol + dc;
                if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) continue;
                if (level[r][c] != 0) continue; // skip walls and special tiles
                // Enforce safe distance from the avoid cell (e.g. player spawn)
                if (avoidRow >= 0 && avoidCol >= 0)
                {
                    int dist = abs(r - avoidRow) + abs(c - avoidCol); // Manhattan distance
                    if (dist < minDist) continue;
                }
                // Found a valid cell - convert to world coordinates and return
                GridToWorldCenter(r, c, outX, outY);
                std::cout << "Spawn found at grid (" << r << "," << c << ")\n";
                return;
            }
        }
    }
    // Fallback: use the start cell even if it wasn't empty
    GridToWorldCenter(startRow, startCol, outX, outY);
    std::cout << "Spawn fallback at grid (" << startRow << "," << startCol << ")\n";
}

// ----------------------------------------------------------------------------
// Level1_Initialize
// Called ONCE after Level1_Load, before the game loop begins (and again on
// state re-entry, e.g. after GS_RESTART).
// Responsibilities:
// 1. Resets all powerup state and overlay flags (pause, win, lose).
// 2. Forces level1_initialised = false so the block below always runs.
// 3. Sizes and positions all entities:
// - Player : FindFreeSpawnCell starting at center-left (row GRID_ROWS/2, col 4)
// - Mummy : FindFreeSpawnCell starting at top-right corner, at least 10 cells
// (Manhattan) away from the player
// - Exit : FindFreeSpawnCell at center-right (col GRID_COLS-5)
// - Coin : FindFreeSpawnCell at grid center
// - Wall : Legacy fixed position (no longer used for collision)
// 4. Resets counters (coinCounter, turnCounter, playerMoved).
// NOTE: There is NO hardcoded spawn position; all positions adapt to whatever
// walls are currently in level[][] (loaded from level1.txt).
// ----------------------------------------------------------------------------
void Level1_Initialize()
{
    std::cout << "Level1:Initialize\n";
    // Always reset powerups and overlays on every (re)entry
    gPower = {};
    gPaused = false; gShowLose = false; gShowWin = false;

    // ======= ADDED: clear frame-based freeze each entry ======= -ths
    gPower.freezeFrames = 0; // -ths

    // Force re-initialisation every time (handles restart correctly)
    level1_initialised = false;
    if (!level1_initialised) {
        player.size = GRID_TILE_SIZE;
        mummy.size = GRID_TILE_SIZE;
        gridStep = GRID_TILE_SIZE;
        float px = 0.0f, py = 0.0f;
        level1_initialised = true;

        // --- Player spawn: center-left area, nearest free cell ---
        FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
        player.x = px;
        player.y = py;
        player.size = 50.0f;
        player.r = 0.0f; player.g = 0.0f; player.b = 1.0f; // blue tint (not visible with texture)
        // Convert player world pos to grid coords for mummy avoidance check
        int playerRow, playerCol;
        WorldToGrid(player.x, player.y, playerRow, playerCol);

        // --- Mummy spawn: top-right corner, min 10 cells from player ---
        FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
        mummy.x = px;
        mummy.y = py;
        mummy.size = 50.0f;
        mummy.r = 1.0f; mummy.g = 0.0f; mummy.b = 0.0f; // red tint

        // --- Exit portal spawn: center-right area ---
        FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        exitPortal.x = px;
        exitPortal.y = py;
        exitPortal.size = 40.0f;
        exitPortal.r = 1.0f; exitPortal.g = 1.0f; exitPortal.b = 0.0f; // yellow tint

        // --- Coin spawn: grid center ---
        FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
        coin.x = px;
        coin.y = py;
        coin.size = 30.0f;
        coin.r = 1.0f; coin.g = 0.5f; coin.b = 0.0f; // orange tint

        // ====== ADDED: spawn a random power-up at a free cell ====== -ths
        SpawnRandomPowerup(); // -ths

        // Legacy wall entity (fixed position, no longer used for collision)
        wall.x = -60.0f;
        wall.y = 0.0f;
        wall.size = 50.0f;
        wall.r = 0.2f; wall.g = 0.2f; wall.b = 0.2f; // dark grey

        // Reset movement / game counters
        nextX = player.x;
        nextY = player.y;
        coinCounter = 0;
        turnCounter = 0;
        playerMoved = false;
        level1_initialised = true;
    }
}

// ----------------------------------------------------------------------------
// Level1_Update
// Called every frame during the Level 1 game loop.
// Handles (in order):
// 1. Legacy level1_counter decrement -- transitions to MAINMENUSTATE at 0.
// 2. Back (B/ESC) and Quit (Q) key handling.
// 3. Win/Lose overlay input (mouse click on Retry/Exit buttons, R, ENTER, Q).
// Returns early -- game logic is frozen while overlays are visible.
// 4. Pause toggle (P) -- returns early when paused.
// 5. Save (F5) / Load (F9) to/from "Assets/save1.txt".
// 6. Player movement: WASD triggers a candidate position; IsTileWalkable()
// validates it against the grid before applying.
// 7. Per-turn logic (runs only when playerMoved == true):
// a. Coin collection: if level[r][c] == 4, remove tile and add to counter.
// b. Mummy AI: every 2nd turn, move mummy one step horizontally then
// vertically toward the player (axis-priority chase), using canMove()
// to respect walls.
// c. TickPowers() -- decrement turn-based powerup durations.
// 8. Lose check: if player and mummy share the same cell (and player has moved
// at least once and is not invincible), call ResetLevel1() and show lose overlay.
// Also checks gExtraEnemies.
// 9. Win check: if player reaches exitPortal cell, set next = GS_WIN.
// 10. Legacy coin entity collect (moves coin off-screen on contact).
// ----------------------------------------------------------------------------
void Level1_Update()
{
    level1_counter--; // Decrement legacy level timer
    if (level1_counter == 0)
    {
        level1_initialised = false;
        next = MAINMENUSTATE; // Legacy: return to main menu when timer expires
    }

    // --- Navigation keys ---
    if (AEInputCheckReleased(AEVK_B) ||
        AEInputCheckReleased(AEVK_ESCAPE)) {
        next = LEVELPAGE;
    }
    if (AEInputCheckReleased(AEVK_Q) ||
        0 == AESysDoesWindowExist()) {
        next = GS_QUIT;
    }

    // --- Win / Lose overlay input handling ---
    // While an overlay is active, only handle UI buttons; game logic is frozen.
    if (gShowLose ||
        gShowWin)
    {
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);
        float mx = (float)mxS, my = (float)myS;
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            // "Retry" button: restart level 1
            if (IsAreaClicked(kBtnRetryX, kBtnRetryY, kBtnW, kBtnH, mx, my))
            {
                next = GS_LEVEL1;
                gShowLose = gShowWin = false;
                return;
            }
            // "Exit" button: return to main menu
            if (IsAreaClicked(kBtnExitX, kBtnExitY, kBtnW, kBtnH, mx, my))
            {
                next = MAINMENUSTATE;
                gShowLose = gShowWin = false;
                return;
            }
        }
        if (AEInputCheckReleased(AEVK_R)) { next = GS_LEVEL1; gShowLose = gShowWin = false; return; }
        if (AEInputCheckReleased(AEVK_RETURN)) { next = MAINMENUSTATE; gShowLose = gShowWin = false; return; }
        if (AEInputCheckReleased(AEVK_Q) ||
            0 == AESysDoesWindowExist()) {
            next = GS_QUIT; return;
        }
        return; // Freeze normal update while overlays are visible
    }

    // --- Pause toggle ---
    if (AEInputCheckReleased(AEVK_P)) { gPaused = !gPaused; }
    if (gPaused) { return; } // Skip all game logic while paused

    // --- Save (F5) / Load (F9) ---
    if (AEInputCheckReleased(AEVK_F5)) { if (SaveLevel1State("Assets/save1.txt")) std::cout << "Saved (Assets/save1.txt)\n"; }
    if (AEInputCheckReleased(AEVK_F9)) { if (LoadLevel1State("Assets/save1.txt")) std::cout << "Loaded (Assets/save1.txt)\n"; }

    // ====== ADDED: frame counters per frame (inv & freeze) ====== -ths
    TickFramePowers();   // invFrames countdown -ths
    TickFreezeFrames();  // freezeFrames countdown -ths

    // --- Player movement ---
    // Step 1: Calculate candidate position based on WASD input
    float testNextX = player.x;
    float testNextY = player.y;
    if (AEInputCheckTriggered(AEVK_W)) testNextY += gridStep; // move up
    else if (AEInputCheckTriggered(AEVK_S)) testNextY -= gridStep; // move down
    else if (AEInputCheckTriggered(AEVK_A)) testNextX -= gridStep; // move left
    else if (AEInputCheckTriggered(AEVK_D)) testNextX += gridStep; // move right

    // Step 2: Validate with IsTileWalkable (checks level[][] via WorldToGrid)
    // Only apply the move if the target tile value is NOT 1 (NON_WALKABLE)
    if ((testNextX != player.x ||
        testNextY != player.y)) {
        if (IsTileWalkable(testNextX, testNextY)) {
            player.x = testNextX;
            player.y = testNextY;
            playerMoved = true; // Triggers mummy AI and coin checks this turn
        }
    }

    // Legacy AABB wall check (kept for reference; superseded by grid-based check above)
    bool playerWallCollision = (fabsf(testNextX - wall.x) < (player.size / 2.0f + wall.size / 2.0f)) &&
        (fabsf(testNextY - wall.y) < (player.size / 2.0f + wall.size / 2.0f));

    // ======= ADDED: power-up pickup check ======= -ths
    if (gPowerupActive &&
        fabsf(player.x - gPowerup.x) < 1.0f &&
        fabsf(player.y - gPowerup.y) < 1.0f)
    {
        if (gPowerupType == PWR_IMMUNE)
        {
            GiveInvincibleFrames(300); // ~5 seconds @60fps -ths
        }
        else // PWR_FREEZE
        {
            gPower.freezeFrames = 180; // ~3 seconds @60fps -ths
        }
        gPowerupActive = false;
        gPowerup.x = gPowerup.y = 2000.0f; // move off-screen (same pattern as coin) -ths
    }

    // --- Per-turn logic (runs once per valid player move) ---
    if (playerMoved)
    {
        turnCounter++;
        // Tile-based coin collection: tile value 4 = COIN
        // Convert player world pos to grid and check for coin tile
        int r, c;
        WorldToGrid(player.x, player.y, r, c);
        if (level[r][c] == 4) {
            level[r][c] = 0; // Clear coin tile so it cannot be collected again
            coinCounter++;
            std::cout << "Collected! Coins: " << coinCounter << "\n";
        }

        // Mummy AI: moves toward player every 2nd player turn
        // Uses axis-priority (horizontal first, then vertical) greedy chase.
        // canMove() validates the mummy's next cell against level[][] walls.
        // ======= ADDED: skip enemy advance while freezeFrames > 0 ======= -ths
        if (turnCounter % 2 == 0 && gPower.freezeFrames <= 0)
        {
            float diffX = player.x - mummy.x;
            float diffY = player.y - mummy.y;
            // Horizontal step: try to close the X gap first
            if (fabsf(diffX) > 1.0f) {
                float stepX = (diffX > 0) ? gridStep : -gridStep;
                if (canMove(mummy.x + stepX, mummy.y)) {
                    mummy.x += stepX;
                }
            }
            // Vertical step: re-evaluate diffY after possible horizontal move
            diffY = player.y - mummy.y;
            if (fabsf(diffY) > 1.0f) {
                float stepY = (diffY > 0) ? gridStep : -gridStep;
                if (canMove(mummy.x, mummy.y + stepY)) {
                    mummy.y += stepY;
                }
            }
        }

        printf("Turn: %d \n Player: (%.0f, %.0f) \n Mummy: (%.0f, %.0f)\n",
            turnCounter, player.x, player.y, mummy.x, mummy.y);

        TickPowers(); // Decrement turn-based powerup counters
        playerMoved = false;
    }

    const bool effectiveInv = IsInvincibleNow(); // Cache invincibility state for checks below
    // --- Lose check: player caught by main mummy ---
    // Only triggers after at least 1 move (avoids false positive on spawn overlap)
    if (turnCounter > 0 && !effectiveInv &&
        fabsf(player.x - mummy.x) < 1.0f && fabsf(player.y - mummy.y) < 1.0f)
    {
        ResetLevel1();
        printf("Caught by the Mummy! Level Reset!\n");
        gShowLose = true;
    }

    // --- Lose check: player caught by any extra enemy ---
    if (turnCounter > 0 && !effectiveInv) {
        for (int i = 0; i < gExtraEnemyCount; ++i) {
            if (fabsf(player.x - gExtraEnemies[i].x) < 1.0f &&
                fabsf(player.y - gExtraEnemies[i].y) < 1.0f) {
                ResetLevel1();
                printf("Caught by an Enemy! Level Reset!\n"); gShowLose = true; break;
            }
        }
    }

    // --- Win check: player reached the exit portal cell ---
    if (fabsf(player.x - exitPortal.x) < 1.0f && fabsf(player.y - exitPortal.y) < 1.0f)
    {
        printf("You Escaped the Maze!\n");
        level1_counter = 0;
        next = GS_WIN; // Transition to the win page
    }

    // --- Legacy coin entity collect (not tile-based; falls back to x>1000 guard) ---
    if (fabsf(player.x - coin.x) < 1.0f && fabsf(player.y - coin.y) < 1.0f)
    {
        ++coinCounter;
        printf("Coin Collected! Total Coins: %d\n", coinCounter);
        coin.x = 2000.0f; coin.y = 2000.0f; // Move off-screen to "delete" it
    }
}

// ----------------------------------------------------------------------------
// Level1_Draw
// Called every frame to render Level 1.
// Rendering order (back to front):
// 1. If a Lose/Win/Pause overlay is active, delegate to its draw function
// and return immediately (the overlay covers the whole screen).
// 2. Floor: iterate all cells with value == 0 and draw gFloorTex.
// 3. Walls: iterate all cells with value == 1 and draw gDesertBlockTex.
// 4. Player: texture at (player.x, player.y) sized player.size x player.size.
// 5. Mummy: texture at (mummy.x, mummy.y) sized mummy.size x mummy.size.
// 6. Coin entity: only drawn when coin.x < 1000 (not yet collected).
// 7. Exit portal: texture at (exitPortal.x, exitPortal.y).
// All entities and tiles use the shared pMesh (unit square scaled by a matrix).
// ----------------------------------------------------------------------------
void Level1_Draw()
{
    AEGfxSetBackgroundColor(0.22f, 0.14f, 0.09f);
    // Redirect rendering to overlay draw functions when overlays are active
    if (gShowLose) { LosePage_Draw(); return; }
    if (gShowWin) { WinPage_Draw(); return; }
    if (gPaused) { PausePage_Draw(); return; }

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    AEMtx33 transform, scale, trans;

    // --- Draw floor texture on every walkable (value == 0) cell ---
    AEGfxTextureSet(gFloorTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            if (level[row][col] == 0)
            {
                float x, y;
                GridToWorldCenter(row, col, x, y); // get tile center in world space
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    // --- Draw wall texture on every non-walkable (value == 1) cell ---
    AEGfxTextureSet(gDesertBlockTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            if (level[row][col] == 1) // 1 = NON_WALKABLE
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

    // --- Render Player (explorer.png texture) ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(player.pTex, 0, 0);
    AEMtx33Scale(&scale, player.size, player.size);
    AEMtx33Trans(&trans, player.x, player.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Render Mummy (Enemy.png texture) ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(mummy.pTex, 0, 0);
    AEMtx33Scale(&scale, mummy.size, mummy.size);
    AEMtx33Trans(&trans, mummy.x, mummy.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Render legacy coin entity (only while coin.x < 1000, i.e. not collected) ---
    if (coin.x < 1000.0f)
    {
        AEGfxTextureSet(coin.pTex, 0, 0);
        AEMtx33Scale(&scale, coin.size, coin.size);
        AEMtx33Trans(&trans, coin.x, coin.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // ======= ADDED: draw power-up icon if active ======= -ths
    if (gPowerupActive)
    {
        AEGfxTextureSet((gPowerupType == PWR_IMMUNE) ? gImmuneTex : gFreezeTex, 0, 0); // -ths
        AEMtx33Scale(&scale, gPowerup.size, gPowerup.size); // -ths
        AEMtx33Trans(&trans, gPowerup.x, gPowerup.y);       // -ths
        AEMtx33Concat(&transform, &trans, &scale);          // -ths
        AEGfxSetTransform(transform.m);                     // -ths
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);         // -ths
    }

    // --- Render Exit Portal (Exit.png texture) ---
    AEGfxTextureSet(exitPortal.pTex, 0, 0);
    AEMtx33Scale(&scale, exitPortal.size, exitPortal.size);
    AEMtx33Trans(&trans, exitPortal.x, exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // ===== ADDED: HUD for active power-ups (top-left) ===== -ths
    if (gPower.invFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "IMMUNE  %.1fs", gPower.invFrames / 60.0f); // ~60 FPS -ths
        AEGfxPrint(fontId, buf, -0.95f, 0.90f, 0.8f, 0.90f, 0.90f, 0.20f, 1.0f);     // yellow-ish -ths
    }
    if (gPower.freezeFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "FREEZE  %.1fs", gPower.freezeFrames / 60.0f); // ~60 FPS -ths
        AEGfxPrint(fontId, buf, -0.95f, 0.82f, 0.8f, 0.60f, 0.85f, 1.00f, 1.0f);       // cyan-ish -ths
    }
    // ===== END ADDED HUD ===== -ths

    // Reset render state to clean defaults after drawing
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
}

// ----------------------------------------------------------------------------
// Level1_Free
// Called after the game loop exits this state, before Unload.
// Currently empty -- no heap memory was allocated that needs explicit freeing
// beyond what Unload handles (textures, mesh).
// ----------------------------------------------------------------------------
void Level1_Free()
{
    std::cout << "Level1:Free\n";
}

// ----------------------------------------------------------------------------
// Level1_Unload
// Called when permanently leaving Level 1 (e.g. going to main menu or quit).
// Unloads all GPU textures and frees the shared mesh to prevent memory leaks.
// Resets level1_initialised so Initialize runs fully on next entry.
// ----------------------------------------------------------------------------
void Level1_Unload()
{
    std::cout << "Level1:Unload\n";
    // Unload all textures loaded in Level1_Load
    AEGfxTextureUnload(player.pTex);
    AEGfxTextureUnload(gDesertBlockTex);
    AEGfxTextureUnload(gFloorTex);
    AEGfxTextureUnload(mummy.pTex);
    AEGfxTextureUnload(coin.pTex);
    AEGfxTextureUnload(exitPortal.pTex);

    // ====== ADDED: unload power-up textures ====== -ths
    AEGfxTextureUnload(gImmuneTex);  // -ths
    AEGfxTextureUnload(gFreezeTex);  // -ths

    // Free the vertex mesh
    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
    level1_initialised = false; // Allow full re-init on next entry
}

// ===== HELPER FUNCTIONS =====
// ----------------------------------------------------------------------------
// ResetLevel1
// Resets all entity positions to new safe spawn cells (same logic as Initialize)
// without going through a full state transition. Called when the player is
// caught by the mummy (not a full level reload -- textures stay loaded).
//
// Spawn placement:
// - Player : center-left area (col 4)
// - Mummy : top-right corner, at least 10 Manhattan cells from player
// - Coin : grid center
// Also resets all counters (coinCounter, turnCounter, playerMoved) and clears
// all active powerup states.
// ----------------------------------------------------------------------------
void ResetLevel1()
{
    float px, py;
    // Re-spawn player at center-left area
    FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
    player.x = px; player.y = py;
    // Re-spawn mummy away from the player
    int playerRow, playerCol;
    WorldToGrid(player.x, player.y, playerRow, playerCol);
    FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
    mummy.x = px; mummy.y = py;
    // Re-spawn legacy coin at grid center
    FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
    coin.x = px; coin.y = py;

    // ====== ADDED: respawn power-up on reset ====== -ths
    SpawnRandomPowerup(); // -ths

    // Reset movement tracking
    nextX = player.x;
    nextY = player.y;
    coinCounter = 0;
    turnCounter = 0;
    playerMoved = false;
    // Clear all powerup states
    gPower.speed = false; gPower.speedTurns = 0;
    gPower.freeze = false; gPower.freezeTurns = 0;
    gPower.invincible = false; gPower.invTurns = 0;
    gPower.invFrames = 0;
    gPower.freezeFrames = 0; // -ths
}