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
/* End Header **************************************************************************/

#include "pch.h"

#include "leveleditor.hpp"
#include "GridUtils.h"
#include "Level1.h"
#include "JumpScare.h"
#include "gamestatemanager.h"
#include "GameStateList.h"
#include "Main.h"
#include <iostream>
#include <fstream>
#include <cmath> /* fabsf -ths */
#include <cstring> /* strlen -ths */
#include <cstdio>  /* snprintf for HUD text -ths */
#include <cstdlib>
#include "Confirmation.h"

// ====================== LEVEL 1 AUDIO VARIABLES ====================== // -ths
static AEAudio sfxPlayerMove;     // -ths
static AEAudio sfxChest;          // -ths
static AEAudio sfxPowerup;        // -ths
static AEAudio sfxJumpscare;      // -ths
static AEAudio sfxExitDoor;       // -ths
static AEAudio sfxGameOver;       // -ths
static AEAudio sfxButton;

static AEAudioGroup level1Group;  // -ths
// ===================================================================== // -ths


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
    s32 click_x, s32 click_y); // correct -ths

/* Forward declarations for local helpers used by SpawnTreasureBox / OpenTreasureBox */
static int RandInt(int minV, int maxV);
static void FindFreeSpawnCell(int startRow, int startCol, float& outX, float& outY,
    int avoidRow = -1, int avoidCol = -1, int minDist = 0, int maxRadius = 15);

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

// ========================== TREASURE BOX SYSTEM ===========================
// Treasure box system: replaces the old enemy spawning with an interactive chest.
// A treasure box sits on the map; when the player touches it, it despawns and
// randomly spawns EITHER a bonus coin (50 %) OR a new chasing mummy (50 %).
// Spawned mummies are stored in gBoxMummies[] and chase the player each turn.
// --------------------------------------------------------------------------

// Treasure box entity state
static Entity gTreasureBox;           // the treasure chest on the map
static bool   gTreasureBoxActive = false; // true while the box is visible/touchable
static AEGfxTexture* gTreasureBoxTex = nullptr; // TreasureBox.png (or fallback color)

// Mummies spawned by treasure boxes (max 8)
struct BoxMummy { float x, y, size; };
static BoxMummy gBoxMummies[8];
static int      gBoxMummyCount = 0;

// Treasure box popup message (shown for ~3 seconds after opening a chest)
static char gPopupMsg[64] = "";  // text to display, empty = no popup
static int  gPopupFrames = 0;   // counts down at 60fps; popup visible while > 0

// ----------------------------------------------------------------------------
// SpawnTreasureBox
// Places the treasure box at a random free cell at least 6 Manhattan cells
// away from the player. Called at init and after each box is opened.
// ----------------------------------------------------------------------------
static void SpawnTreasureBox()
{
    for (int tries = 0; tries < 256; ++tries)
    {
        int r = RandInt(0, GRID_ROWS - 1);
        int c = RandInt(0, GRID_COLS - 1);
        if (level[r][c] != 0) continue; // must be walkable

        // Keep it a fair distance from the player
        int pr, pc;
        WorldToGrid(player.x, player.y, pr, pc);
        if (abs(r - pr) + abs(c - pc) < 3) continue;

        float wx, wy;
        GridToWorldCenter(r, c, wx, wy);
        gTreasureBox.x = wx;
        gTreasureBox.y = wy;
        gTreasureBox.size = gridStep * 0.9f;
        gTreasureBoxActive = true;
        printf("TreasureBox spawned at grid (%d, %d) | world (%.1f, %.1f)\n", r, c, wx, wy);
        return;
    }
    // Fallback: retry without distance constraint so the box is always visible
    for (int tries = 0; tries < 256; ++tries)
    {
        int r = RandInt(0, GRID_ROWS - 1);
        int c = RandInt(0, GRID_COLS - 1);
        if (level[r][c] != 0) continue;
        float wx, wy;
        GridToWorldCenter(r, c, wx, wy);
        gTreasureBox.x = wx;
        gTreasureBox.y = wy;
        gTreasureBox.size = gridStep * 0.9f;
        gTreasureBoxActive = true;
        printf("TreasureBox spawned (fallback) at grid (%d, %d) | world (%.1f, %.1f)\n", r, c, wx, wy);
        return;
    }
    gTreasureBoxActive = false; // map is entirely walled (should never happen)
}

// ----------------------------------------------------------------------------
// OpenTreasureBox
// Called when the player steps onto the treasure box.
// 50 % chance: awards +1 coin instantly.
// 50 % chance: spawns a new chasing mummy near the box position.
// The box then despawns and is re-placed at a new location.
// ----------------------------------------------------------------------------
static void OpenTreasureBox()
{
    gTreasureBoxActive = false;

    // Play chest audio if available
    if (AEAudioIsValidAudio(sfxChest))
        AEAudioPlay(sfxChest, level1Group, 1.0f, 1.0f, 0);

    bool spawnMummy = (AERandFloat() >= 0.5f); // 50/50

    if (!spawnMummy)
    {
        // Reward: instant bonus coin
        coinCounter++;
        printf("Treasure Box: COIN! Total coins: %d\n", coinCounter);
        std::snprintf(gPopupMsg, sizeof(gPopupMsg), "Treasure: +1 Coin! (Total: %d)", coinCounter);
        gPopupFrames = 180; // show for ~3 seconds
    }
    else
    {
        // Trigger jump scare when mummy spawns from chest
        JumpScare_Trigger();

        // Play sound for the jump scare
        if (AEAudioIsValidAudio(sfxJumpscare))
            AEAudioPlay(sfxJumpscare, level1Group, 1.0f, 1.0f, 0);

        // Hazard: spawn a new chasing mummy near the box
        if (gBoxMummyCount < (int)(sizeof(gBoxMummies) / sizeof(gBoxMummies[0])))
        {
            float sx, sy;
            int br, bc;
            WorldToGrid(gTreasureBox.x, gTreasureBox.y, br, bc);
            // Find a nearby free cell that is NOT the player's cell
            int pr, pc;
            WorldToGrid(player.x, player.y, pr, pc);
            FindFreeSpawnCell(br, bc, sx, sy, pr, pc, 2);

            BoxMummy& m = gBoxMummies[gBoxMummyCount++];
            m.x = sx; m.y = sy; m.size = gridStep;
            printf("Treasure Box: MUMMY spawned at (%.0f, %.0f)!\n", sx, sy);
            std::snprintf(gPopupMsg, sizeof(gPopupMsg), "Treasure: A Mummy appeared!");
            gPopupFrames = 180; // show for ~3 seconds
        }
    }

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

static void SpawnRandomPowerup()
{
    gPowerupType = (AERandFloat() < 0.5f) ? PWR_IMMUNE : PWR_FREEZE;

    int pr, pc;
    WorldToGrid(player.x, player.y, pr, pc);
    const int NEAR_RADIUS = 5;

    // Store all valid nearby cells
    struct Candidate { int r, c; };
    Candidate candidates[256];
    int candidateCount = 0;

    for (int radius = 1; radius <= NEAR_RADIUS; ++radius)
    {
        for (int dr = -radius; dr <= radius; ++dr)
        {
            for (int dc = -radius; dc <= radius; ++dc)
            {
                // Only outer ring of the current radius
                if (abs(dr) != radius && abs(dc) != radius) continue;

                int r = pr + dr;
                int c = pc + dc;
                if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) continue;
                if (level[r][c] != 0) continue;        // must be walkable
                if (r == pr && c == pc) continue;      // don't spawn on player

                candidates[candidateCount].r = r;
                candidates[candidateCount].c = c;
                candidateCount++;
            }
        }
    }

    // If we found any nearby free cells, pick one randomly
    if (candidateCount > 0)
    {
        int idx = RandInt(0, candidateCount - 1);
        int r = candidates[idx].r;
        int c = candidates[idx].c;
        float x, y;
        GridToWorldCenter(r, c, x, y);
        gPowerup.x = x; gPowerup.y = y;
        gPowerup.size = 30.0f;
        gPowerupActive = true;
        return;
    }

    // Fallback: random walkable cell anywhere (avoid player)
    for (int tries = 0; tries < 128; ++tries)
    {
        int r = RandInt(0, GRID_ROWS - 1);
        int c = RandInt(0, GRID_COLS - 1);
        if (level[r][c] == 0)
        {
            int pr2, pc2;
            WorldToGrid(player.x, player.y, pr2, pc2);
            if (r == pr2 && c == pc2) continue;

            float x, y;
            GridToWorldCenter(r, c, x, y);
            gPowerup.x = x; gPowerup.y = y;
            gPowerup.size = 30.0f;
            gPowerupActive = true;
            return;
        }
    }
    gPowerupActive = false;
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

// ========================== Treasure box popup message ==========================

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
float CenteredTextX(float centerWorldX, const char* text, float scale)
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
// powerups, and box-mummy list) to 'path' as plain text.
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
    f << gBoxMummyCount << '\n';
    for (int i = 0; i < gBoxMummyCount; ++i)
        f << gBoxMummies[i].x << ' ' << gBoxMummies[i].y << '\n';
    return true;
}

// ----------------------------------------------------------------------------
// LoadLevel1State
// Reads a previously saved Level 1 state from 'path' and restores player
// position, counters, powerup durations, and box-mummy positions.
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
    f >> gBoxMummyCount; if (gBoxMummyCount < 0) gBoxMummyCount = 0;
    if (gBoxMummyCount > (int)(sizeof(gBoxMummies) / sizeof(gBoxMummies[0])))
        gBoxMummyCount = (int)(sizeof(gBoxMummies) / sizeof(gBoxMummies[0]));
    for (int i = 0; i < gBoxMummyCount; ++i)
    {
        f >> gBoxMummies[i].x >> gBoxMummies[i].y;
        gBoxMummies[i].size = gridStep;
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
// 4. Initialises the treasure box state (actual spawn happens in Initialize).
// ----------------------------------------------------------------------------
void Level1_Load()
{
    std::cout << "Level1:Load\n";

    // ===================== AUDIO LOAD FOR LEVEL 1 ======================= // -ths
    // Create an audio group for Level1 sounds                             // -ths
    level1Group = AEAudioCreateGroup();                                    // -ths

    // Load sound effects                                                   // -ths
    sfxPlayerMove = AEAudioLoadSound("Assets/audio/player.wav");           // -ths
    sfxChest = AEAudioLoadSound("Assets/audio/chest.wav");            // -ths
    sfxPowerup = AEAudioLoadSound("Assets/audio/powerup.wav");          // -ths
    //sfxButton   = AEAudioLoadSound("Assets/audio/button.wav");
    sfxJumpscare = AEAudioLoadSound("Assets/audio/jumpscare.wav");        // -ths
    sfxExitDoor = AEAudioLoadSound("Assets/audio/exit.wav");             // -ths
    sfxGameOver = AEAudioLoadSound("Assets/audio/gameover.wav");         // -ths
    sfxButton = AEAudioLoadSound("Assets/audio/button.wav");
    // ==================================================================== // -ths

    // Step 1: Load the tile map from disk into level[][]
    LoadLevelTxt();

    // Step 2: Load entity textures from Assets/
    player.pTex = AEGfxTextureLoad("Assets/explorer.png");       // player sprite
    gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");    // wall tile texture
    gFloorTex = AEGfxTextureLoad("Assets/Floor.png");          // floor tile texture
    mummy.pTex = AEGfxTextureLoad("Assets/Enemy.png");          // main mummy texture
    coin.pTex = AEGfxTextureLoad("Assets/Coin.png");           // legacy coin texture
    exitPortal.pTex = AEGfxTextureLoad("Assets/DoorClosed.png");     // exit portal texture

    // ====== ADDED: load power-up textures (immune / freeze) ====== -ths
    gImmuneTex = AEGfxTextureLoad("Assets/Immune.png"); // -ths
    gFreezeTex = AEGfxTextureLoad("Assets/Freeze.png"); // -ths

    // ====== ADDED: load treasure box texture ======
    gTreasureBoxTex = AEGfxTextureLoad("Assets/TreasureChest.png");

    // Load jump scare texture
    JumpScare_Load();

    // Step 3: Create the unit square mesh used to draw all sprites and tiles
    pMesh = CreateSquareMesh();

    // Step 4: Initialise treasure box state (actual spawn happens in Initialize)
    gBoxMummyCount = 0;
    gTreasureBoxActive = false;
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
    int avoidRow, int avoidCol, int minDist, int maxRadius)
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

    // =============================================================
    // STOP ALL PREVIOUS AUDIO (stops MainMenu BGM completely) // -ths
    // =============================================================

    // ============================================================= // -ths

    // Always reset powerups and overlays on every (re)entry
    gPower = {};
    gPaused = false;
    gShowLose = false;
    gShowWin = false;

    // ======= ADDED: clear frame-based freeze each entry ======= -ths
    gPower.freezeFrames = 0; // -ths

    // Initialise jump scare
    JumpScare_Init();

    // Force re-initialisation every time (handles restart correctly)
    level1_initialised = false;

    if (!level1_initialised)
    {
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
        player.r = 0.0f;
        player.g = 0.0f;
        player.b = 1.0f; // blue tint (texture overrides tint)

        // Convert player world pos to grid coords for mummy avoidance
        int playerRow, playerCol;
        WorldToGrid(player.x, player.y, playerRow, playerCol);

        // --- Mummy spawn: top-right corner, min 10 cells from player ---
        FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
        mummy.x = px;
        mummy.y = py;
        mummy.size = 50.0f;
        mummy.r = 1.0f;
        mummy.g = 0.0f;
        mummy.b = 0.0f; // red tint

        // --- Exit portal spawn: center-right area ---
        FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        exitPortal.x = px;
        exitPortal.y = py;
        exitPortal.size = 50.0f;
        exitPortal.r = 1.0f;
        exitPortal.g = 1.0f;
        exitPortal.b = 0.0f; // yellow tint

        // --- Coin spawn: grid center ---
        FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
        coin.x = px;
        coin.y = py;
        coin.size = GRID_TILE_SIZE * 0.8f;  // match tile-based coin draw size and editor
        coin.r = 1.0f;
        coin.g = 0.5f;
        coin.b = 0.0f; // orange tint

        // ====== ADDED: spawn a random power-up at a free cell ====== -ths
        SpawnRandomPowerup(); // -ths

        // ====== Initialise treasure box (gridStep is now set) ======
        gBoxMummyCount = 0;
        SpawnTreasureBox();

        // Legacy wall entity (unused for collision now)
        wall.x = -60.0f;
        wall.y = 0.0f;
        wall.size = 50.0f;
        wall.r = 0.2f;
        wall.g = 0.2f;
        wall.b = 0.2f;

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
// 2. Back (B) and Quit (ESC) key handling.
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
// Also checks box mummies spawned by the treasure box.
// Jump Scare is also shown.
// 9. Win check: if player reaches exitPortal cell, set next = GS_WIN.
// 10. Legacy coin entity collect (moves coin off-screen on contact).
// ----------------------------------------------------------------------------
void Level1_Update()
{
    level1_counter--;
    if (level1_counter == 0)
    {
        level1_initialised = false;
        next = MAINMENUSTATE;
    }

    // --- Navigation keys ---
    if (AEInputCheckReleased(AEVK_B)) {
        next = LEVELPAGE;
    }
    if (AEInputCheckReleased(AEVK_ESCAPE) ||
        0 == AESysDoesWindowExist()) {
        next = GS_QUIT;
    }


    // ==================== WIN / LOSE OVERLAY HANDLING ==================== // -ths
    if (gShowLose || gShowWin)
    {
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            // Level Select button (0,60) 280x70 -ths
            if (IsAreaClicked(0.0f, 60.0f, 280.0f, 70.0f, mxS, myS))
            {
                next = LEVELPAGE;
                gShowLose = gShowWin = false;
                return;
            }
            // Restart button (0,-20) 280x70 -ths
            if (IsAreaClicked(0.0f, -20.0f, 280.0f, 70.0f, mxS, myS))
            {
                next = GS_LEVEL1;
                gShowLose = gShowWin = false;
                return;
            }
            // Quit button (0,-100) 280x70 -ths
            if (IsAreaClicked(0.0f, -100.0f, 280.0f, 70.0f, mxS, myS))
            {
                next = GS_QUIT;
                gShowLose = gShowWin = false;
                return;
            }
        }

        // Keyboard handling with confirmation (unchanged) -ths
        if (AEInputCheckReleased(AEVK_R))
        {
            if (AEAudioIsValidAudio(sfxButton))
                AEAudioPlay(sfxButton, level1Group, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL1, GS_LEVEL1, "Are you sure you want to restart?");
            next = CONFIRM;
            gShowLose = false;
            return;
        }
        if (AEInputCheckReleased(AEVK_RETURN) || AEInputCheckReleased(AEVK_B))
        {
            if (AEAudioIsValidAudio(sfxButton))
                AEAudioPlay(sfxButton, level1Group, 1.0f, 1.0f, 0);
            Confirmation_Level(GS_LEVEL1, LEVELPAGE, "Are you sure you want to go to Level Select?");
            next = CONFIRM;
            gShowLose = false;
            return;
        }
        if (AEInputCheckReleased(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
        {
            next = GS_QUIT;
            return;
        }
        return; // freeze game logic
    }
    // =========================================================================
    // ADDED: PAUSE BUTTON CLICK DETECTION (top-right corner) -ths
    // =========================================================================
    // --- Pause overlay handling ---
    {
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            if (IsAreaClicked(750.0f, 420.0f, 80.0f, 40.0f, mxS, myS))
            {
                // Play click sound (use level‑specific sfxButton)
                if (AEAudioIsValidAudio(sfxButton))   // for Level1 use sfxButton, for Level2 use l2_sfxButton, etc.
                    AEAudioPlay(sfxButton, level1Group, 1.0f, 1.0f, 0);
                gPaused = true;                       // toggle pause overlay
                return;
            }
        }
    }
    if (gPaused)
    {
        // Mouse clicks on pause overlay buttons -ths
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            // Resume button (0,80) 280x70 -ths
            if (IsAreaClicked(0.0f, 80.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(sfxButton))
                    AEAudioPlay(sfxButton, level1Group, 1.0f, 1.0f, 0);
                gPaused = false;
                return;
            }
            // Restart button (0,0) 280x70 -ths
            if (IsAreaClicked(0.0f, 0.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(sfxButton))
                    AEAudioPlay(sfxButton, level1Group, 1.0f, 1.0f, 0);
                next = GS_RESTART;   // was GS_LEVEL1; changed -ths
                return;
            }
            // Level Select button (0,-80) 280x70 -ths
            if (IsAreaClicked(0.0f, -80.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(sfxButton))
                    AEAudioPlay(sfxButton, level1Group, 1.0f, 1.0f, 0);
                next = LEVELPAGE;
                return;
            }
            // Quit button (0,-160) 280x70 -ths
            if (IsAreaClicked(0.0f, -160.0f, 280.0f, 70.0f, mxS, myS))
            {
                if (AEAudioIsValidAudio(sfxButton))
                    AEAudioPlay(sfxButton, level1Group, 1.0f, 1.0f, 0);
                next = GS_QUIT;
                return;
            }
        }

        // Keyboard shortcuts
        if (AEInputCheckReleased(AEVK_R))
        {
            next = GS_RESTART;   // was GS_RESTART (already correct) -ths
            return;
        }
        return; // freeze game logic while paused
    }

    // --- Save (F5) / Load (F9) ---
    if (AEInputCheckReleased(AEVK_F5)) { if (SaveLevel1State("Assets/save1.txt")) std::cout << "Saved (Assets/save1.txt)\n"; }
    if (AEInputCheckReleased(AEVK_F9)) { if (LoadLevel1State("Assets/save1.txt")) std::cout << "Loaded (Assets/save1.txt)\n"; }

    // ====== Frame counters ====== -ths
    TickFramePowers();
    TickFreezeFrames();
    if (gPopupFrames > 0) --gPopupFrames;

    // --- Player movement ---
    float testNextX = player.x;
    float testNextY = player.y;

    if (AEInputCheckTriggered(AEVK_W)) testNextY += gridStep;
    else if (AEInputCheckTriggered(AEVK_S)) testNextY -= gridStep;
    else if (AEInputCheckTriggered(AEVK_A)) testNextX -= gridStep;
    else if (AEInputCheckTriggered(AEVK_D)) testNextX += gridStep;

    bool attemptedMove = (testNextX != player.x || testNextY != player.y); // -ths

    if (attemptedMove && IsTileWalkable(testNextX, testNextY))
    {
        player.x = testNextX;
        player.y = testNextY;
        playerMoved = true;

        // ====== PLAY MOVEMENT AUDIO ====== -ths
        if (AEAudioIsValidAudio(sfxPlayerMove))
            AEAudioPlay(sfxPlayerMove, level1Group, 1.0f, 1.0f, 0); // -ths
    }

    // Legacy
    bool playerWallCollision = (fabsf(testNextX - wall.x) < (player.size / 2.0f + wall.size / 2.0f)) &&
        (fabsf(testNextY - wall.y) < (player.size / 2.0f + wall.size / 2.0f));

    // ======= POWER-UP PICKUP ======= -ths
    if (gPowerupActive &&
        fabsf(player.x - gPowerup.x) < 1.0f &&
        fabsf(player.y - gPowerup.y) < 1.0f)
    {
        // Play powerup audio -ths
        if (AEAudioIsValidAudio(sfxPowerup))
            AEAudioPlay(sfxPowerup, level1Group, 1.0f, 1.0f, 0); // -ths

        if (gPowerupType == PWR_IMMUNE)
        {
            GiveInvincibleFrames(300);
        }
        else
        {
            gPower.freezeFrames = 180;
        }

        gPowerupActive = false;
        gPowerup.x = gPowerup.y = 2000.0f;
    }

    // --- Per-turn logic ---
    if (playerMoved)
    {
        turnCounter++;

        int r, c;
        WorldToGrid(player.x, player.y, r, c);
        if (level[r][c] == 4) {
            level[r][c] = 0;
            coinCounter++;
            std::cout << "Collected! Coins: " << coinCounter << "\n";
        }

        // Enemy freeze stop -ths
        if (turnCounter != 0 && gPower.freezeFrames <= 0)
        {
            // ---- BFS: find the next step toward the player ----
            // Returns the grid coords of the first step on the shortest path
            // from (startR,startC) to (goalR,goalC), avoiding NON_WALKABLE cells.
            // If no path exists the mummy stays put.
            auto BFSNextStep = [&](int startR, int startC, int goalR, int goalC,
                int outR[], int outC[]) -> bool
                {
                    if (startR == goalR && startC == goalC) return false;

                    // visited + parent arrays on the stack (18x32 = 576 cells)
                    bool visited[GRID_ROWS][GRID_COLS] = {};
                    int  parentR[GRID_ROWS][GRID_COLS];
                    int  parentC[GRID_ROWS][GRID_COLS];
                    for (int i = 0; i < GRID_ROWS; ++i)
                        for (int j = 0; j < GRID_COLS; ++j)
                        {
                            parentR[i][j] = -1; parentC[i][j] = -1;
                        }

                    // Simple queue using a fixed array
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

                    // Trace back to find first step from start
                    int tr = goalR, tc = goalC;
                    while (parentR[tr][tc] != startR || parentC[tr][tc] != startC)
                    {
                        int pr2 = parentR[tr][tc], pc2 = parentC[tr][tc];
                        tr = pr2; tc = pc2;
                    }
                    outR[0] = tr; outC[0] = tc;
                    return true;
                };

            // Move main mummy one BFS step toward player
            int mummyR, mummyC, playerR, playerC;
            WorldToGrid(mummy.x, mummy.y, mummyR, mummyC);
            WorldToGrid(player.x, player.y, playerR, playerC);

            int nextR[1], nextC[1];
            if (BFSNextStep(mummyR, mummyC, playerR, playerC, nextR, nextC))
            {
                // Check the target cell is not occupied by a box mummy
                float nx, ny;
                GridToWorldCenter(nextR[0], nextC[0], nx, ny);
                bool blocked = false;
                for (int i = 0; i < gBoxMummyCount; ++i)
                    if (fabsf(gBoxMummies[i].x - nx) < 1.0f && fabsf(gBoxMummies[i].y - ny) < 1.0f)
                    {
                        blocked = true; break;
                    }
                if (!blocked)
                {
                    mummy.x = nx;
                    mummy.y = ny;
                }
            }
        }

        TickPowers();
        playerMoved = false;

        // ====== BOX MUMMY AI: BFS chase player every 2nd turn ======
        if (turnCounter % 2 == 0 && gPower.freezeFrames <= 0)
        {
            for (int i = 0; i < gBoxMummyCount; ++i)
            {
                BoxMummy& bm = gBoxMummies[i];

                int bmR, bmC, playerR2, playerC2;
                WorldToGrid(bm.x, bm.y, bmR, bmC);
                WorldToGrid(player.x, player.y, playerR2, playerC2);

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

                // Don't move onto main mummy or another box mummy
                bool blocked = (fabsf(mummy.x - nx) < 1.0f && fabsf(mummy.y - ny) < 1.0f);
                if (!blocked)
                    for (int j = 0; j < gBoxMummyCount; ++j)
                        if (j != i && fabsf(gBoxMummies[j].x - nx) < 1.0f && fabsf(gBoxMummies[j].y - ny) < 1.0f)
                        {
                            blocked = true; break;
                        }

                if (!blocked)
                {
                    bm.x = nx;
                    bm.y = ny;
                }
            }
        }
    }

    const bool effectiveInv = IsInvincibleNow();

    // ====== UPDATE JUMP SCARE ANIMATION =======
    JumpScare_Update(); // Before lose conditions

    // Ensure jumpscare is drawn before level resets
    static bool pendingGameOverReset = false;

    // ====== MAIN MUMMY CATCH ======
    if (turnCounter > 0 && !effectiveInv &&
        fabsf(player.x - mummy.x) < 1.0f &&
        fabsf(player.y - mummy.y) < 1.0f)
    {
        // S 22/3: shifted audio to jumpscare and losing screen logic
        //if (AEAudioIsValidAudio(sfxJumpscare))
        //    AEAudioPlay(sfxJumpscare, level1Group, 1.0f, 1.0f, 0);
        //if (AEAudioIsValidAudio(sfxGameOver))
        //    AEAudioPlay(sfxGameOver, level1Group, 1.0f, 1.0f, 0);

        if (!JumpScare_IsActive() && !pendingGameOverReset)
        {
            if (AEAudioIsValidAudio(sfxJumpscare))
                AEAudioPlay(sfxJumpscare, level1Group, 1.0f, 1.0f, 0);

            // Trigger jump scare immediately when caught
            JumpScare_Trigger();
            pendingGameOverReset = true; // Mark that we need to reset after jump scare
            std::cout << "CAUGHT! Playing jump scare...\n";
        }

        // S: 22/3 need to move to reset to later else level will reset immediately upon losing,
        // S: 22/3 causing the jump scare to load AFTER game over scene
        //ResetLevel1();
        //printf("Caught by the Mummy! Level Reset!\n");
        //gShowLose = true;
        //return; // skip remaining update logic this frame
    }

    // ====== TREASURE BOX TOUCH ======
    if (gTreasureBoxActive &&
        fabsf(player.x - gTreasureBox.x) < 1.0f &&
        fabsf(player.y - gTreasureBox.y) < 1.0f)
    {
        OpenTreasureBox(); // randomly gives coin or spawns mummy; re-places box
    }

    // ====== BOX MUMMY CATCH ======
    if (turnCounter > 0 && !effectiveInv)
    {
        for (int i = 0; i < gBoxMummyCount; ++i)
        {
            if (fabsf(player.x - gBoxMummies[i].x) < 1.0f &&
                fabsf(player.y - gBoxMummies[i].y) < 1.0f)
            {
                // S 22/3: shifted audio to jumpscare and losing screen logic
                //if (AEAudioIsValidAudio(sfxJumpscare))
                //    AEAudioPlay(sfxJumpscare, level1Group, 1.0f, 1.0f, 0);
                //if (AEAudioIsValidAudio(sfxGameOver))
                //    AEAudioPlay(sfxGameOver, level1Group, 1.0f, 1.0f, 0);

                if (!JumpScare_IsActive() && !pendingGameOverReset)
                {
                    if (AEAudioIsValidAudio(sfxJumpscare))
                        AEAudioPlay(sfxJumpscare, level1Group, 1.0f, 1.0f, 0);

                    // Trigger jump scare immediately when caught
                    JumpScare_Trigger();
                    pendingGameOverReset = true; // Mark that we need to reset after jump scare
                    std::cout << "CAUGHT! Playing jump scare...\n";
                }

                // S: 22/3 need to move to reset to later else level will reset immediately upon losing,
                // S: 22/3 causing the jump scare to load AFTER game over scene
                //ResetLevel1();
                //printf("Caught by a Box Mummy! Level Reset!\n");
                //gShowLose = true;
                //return; // skip remaining update logic this frame
            }
        }
    }

    // ========= LEVEL RESET FOR LOSE CONDITION =========
    // All lose conditions and jump scare occurance is in
    // Reset level can commence
    if (pendingGameOverReset && !JumpScare_IsActive())
    {
        if (AEAudioIsValidAudio(sfxGameOver))
            AEAudioPlay(sfxGameOver, level1Group, 1.0f, 1.0f, 0);

        ResetLevel1();
        pendingGameOverReset = false;
        std::cout << "Jump scare finished, resetting level...\n";
        gShowLose = true;
        return; // skip remaining update logic this frame
    }

    // ====== EXIT PORTAL WIN ====== -ths
    if (fabsf(player.x - exitPortal.x) < 1.0f &&
        fabsf(player.y - exitPortal.y) < 1.0f)
    {
        if (coinCounter >= 1)
        {
            // Play exit-door audio -ths
            if (AEAudioIsValidAudio(sfxExitDoor))
                AEAudioPlay(sfxExitDoor, level1Group, 1.0f, 1.0f, 0); // -ths

            printf("You Escaped the Maze!\n");
            level1_counter = 0;
            next = GS_WIN;
        }
        else
        {
            // Player hasn't collected a coin yet -- show reminder
            std::snprintf(gPopupMsg, sizeof(gPopupMsg), "Collect a coin first!");
            gPopupFrames = 120;
        }
    }

    // --- Legacy coin entity collect ---
    if (fabsf(player.x - coin.x) < 1.0f &&
        fabsf(player.y - coin.y) < 1.0f)
    {
        ++coinCounter;
        printf("Coin Collected! Total Coins: %d\n", coinCounter);
        coin.x = 2000.0f;
        coin.y = 2000.0f;
    }
}

// ============================================================================
// DrawPauseButton - draws a gray rectangle and centers "PAUSE" text inside it -ths
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
    const float textScale = 0.65f;        // <-- changed from 0.8f
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
    if (gShowLose) { LosePage_Draw(); return; }
    if (gShowWin) { WinPage_Draw(); return; }
    if (gPaused) { PausePage_Draw(); return; }

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    AEMtx33 transform, scale, trans;

    // --- Draw floor texture on walkable tiles ---
    AEGfxTextureSet(gFloorTex, 0, 0);
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
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
        }
    }

    // --- Draw wall texture on NON-WALKABLE tiles ---
    AEGfxTextureSet(gDesertBlockTex, 0, 0);
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

    // ====== ADDED: Draw coin tiles (value 4) ====== -ths
    AEGfxTextureSet(coin.pTex, 0, 0);  // use the coin texture -ths
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            if (level[row][col] == 4)
            {
                float x, y;
                GridToWorldCenter(row, col, x, y);
                AEMtx33Scale(&scale, coin.size, coin.size); // use coin.size to stay in sync with editor and legacy coin -ths
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    // --- Render Player ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(player.pTex, 0, 0);
    AEMtx33Scale(&scale, player.size, player.size);
    AEMtx33Trans(&trans, player.x, player.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Render Mummy ---
    AEGfxTextureSet(mummy.pTex, 0, 0);
    AEMtx33Scale(&scale, mummy.size, mummy.size);
    AEMtx33Trans(&trans, mummy.x, mummy.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // ====== DRAW TREASURE BOX ======
    if (gTreasureBoxActive)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(gTreasureBoxTex, 0, 0);
        AEMtx33Scale(&scale, gTreasureBox.size, gTreasureBox.size);
        AEMtx33Trans(&trans, gTreasureBox.x, gTreasureBox.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // ====== DRAW BOX MUMMIES (reuse main mummy texture, red-tinted) ======
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 0.3f, 0.3f, 1.0f); // reddish tint to distinguish
    AEGfxTextureSet(mummy.pTex, 0, 0);
    for (int i = 0; i < gBoxMummyCount; ++i)
    {
        AEMtx33Scale(&scale, gBoxMummies[i].size, gBoxMummies[i].size);
        AEMtx33Trans(&trans, gBoxMummies[i].x, gBoxMummies[i].y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }
    // Reset tint to white for subsequent draws
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    // --- Legacy Coin rendering ---
    if (coin.x < 1000.0f)
    {
        AEGfxTextureSet(coin.pTex, 0, 0);
        AEMtx33Scale(&scale, coin.size, coin.size);
        AEMtx33Trans(&trans, coin.x, coin.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // ======= POWERUP DRAWING ======= -ths
    if (gPowerupActive)
    {
        AEGfxTextureSet((gPowerupType == PWR_IMMUNE) ? gImmuneTex : gFreezeTex, 0, 0); // -ths
        AEMtx33Scale(&scale, gPowerup.size, gPowerup.size); // -ths
        AEMtx33Trans(&trans, gPowerup.x, gPowerup.y);       // -ths
        AEMtx33Concat(&transform, &trans, &scale);          // -ths
        AEGfxSetTransform(transform.m);                     // -ths
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);         // -ths
    }

    // --- Exit portal ---
    AEGfxTextureSet(exitPortal.pTex, 0, 0);
    AEMtx33Scale(&scale, exitPortal.size, exitPortal.size);
    AEMtx33Trans(&trans, exitPortal.x, exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Jump Scare --- 
    JumpScare_Draw();

    // ===== HUD FOR ACTIVE POWER-UPS ===== -ths
    if (gPower.invFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "IMMUNE  %.1fs", gPower.invFrames / 60.0f); // -ths
        AEGfxPrint(fontId, buf, -0.95f, 0.74f, 1.0f, 0.90f, 0.90f, 0.20f, 1.0f);     // -ths
    }
    if (gPower.freezeFrames > 0)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "FREEZE  %.1fs", gPower.freezeFrames / 60.0f); // -ths
        AEGfxPrint(fontId, buf, -0.95f, 0.82f, 1.0f, 0.60f, 0.85f, 1.00f, 1.0f);       // -ths
    }

    // ====== Coin counter HUD (just below immunity/freeze stack) ======
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Coins: %d", coinCounter);
        AEGfxPrint(fontId, buf, -0.95f, 0.90f, 1.2f, 0.60f, 0.15f, 0.20f, 1.0f); // maroon
    }

    // ====== Hint: collect a coin before escaping (shown until first coin collected) ======
    if (coinCounter == 0)
    {
        const char* hint = "Coins to escape: 0/1";
        float hw, hh;
        AEGfxGetPrintSize(fontId, hint, 0.7f, &hw, &hh);
        AEGfxPrint(fontId, hint, -hw * 0.5f, 0.90f, 1.2f, 0.60f, 0.15f, 0.20f, 1.0f); // maroon, top-center
    }

    // ====== Treasure box popup message -- centered, fades out after ~3 seconds ======
    if (gPopupFrames > 0)
    {
        float alpha = (gPopupFrames < 60) ? gPopupFrames / 60.0f : 1.0f;
        float popupScale = 0.85f;
        float pw, ph;
        AEGfxGetPrintSize(fontId, gPopupMsg, popupScale, &pw, &ph);
        float centeredX = -pw * 0.5f;
        AEGfxPrint(fontId, gPopupMsg, centeredX, 0.4f, popupScale, 1.0f, 1.0f, 0.4f, alpha);
    }

    // ====================================================================
    // ADD: Top‑right PAUSE BUTTON (Draw only during gameplay) -ths
    // ====================================================================
    DrawPauseButton();   // renders the pause button using helper from earlier -ths
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

    // ---------------- TEXTURE UNLOAD ----------------
    AEGfxTextureUnload(player.pTex);
    AEGfxTextureUnload(gDesertBlockTex);
    AEGfxTextureUnload(gFloorTex);
    AEGfxTextureUnload(mummy.pTex);
    AEGfxTextureUnload(coin.pTex);
    AEGfxTextureUnload(exitPortal.pTex);

    // ====== ADDED: unload power-up textures ====== -ths
    AEGfxTextureUnload(gImmuneTex);  // -ths
    AEGfxTextureUnload(gFreezeTex);  // -ths

    // ------ Unload Jump Scare ------
    JumpScare_Unload();

    // ====== Unload treasure box texture ======
    if (gTreasureBoxTex) { AEGfxTextureUnload(gTreasureBoxTex); gTreasureBoxTex = nullptr; }

    // ---------------- AUDIO UNLOAD ---------------- // -ths
    if (AEAudioIsValidAudio(sfxPlayerMove)) AEAudioUnloadAudio(sfxPlayerMove);   // -ths
    if (AEAudioIsValidAudio(sfxChest))      AEAudioUnloadAudio(sfxChest);        // -ths
    if (AEAudioIsValidAudio(sfxPowerup))    AEAudioUnloadAudio(sfxPowerup);      // -ths
    if (AEAudioIsValidAudio(sfxJumpscare))  AEAudioUnloadAudio(sfxJumpscare);    // -ths
    if (AEAudioIsValidAudio(sfxExitDoor))   AEAudioUnloadAudio(sfxExitDoor);     // -ths
    if (AEAudioIsValidAudio(sfxGameOver))   AEAudioUnloadAudio(sfxGameOver);     // -ths
    if (AEAudioIsValidAudio(sfxButton)) AEAudioUnloadAudio(sfxButton);

    // Unload group (no harm if empty) -ths
    AEAudioUnloadAudioGroup(level1Group);  // -ths
    // ------------------------------------------------

    // Mesh cleanup
    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }

    level1_initialised = false;
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

    // ====== ADDED: respawn exit portal on reset ====== -ths
    FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
    exitPortal.x = px; exitPortal.y = py; // -ths

    // ====== ADDED: respawn power-up on reset ====== -ths
    SpawnRandomPowerup(); // -ths

    // ====== Treasure box: clear spawned mummies and re-place the box ======
    gBoxMummyCount = 0;
    SpawnTreasureBox();

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

    // Clear any lingering treasure box popup
    gPopupFrames = 0;
    gPopupMsg[0] = '\0';
}