/* Start Header ***************************************************************
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

#include  "GridUtils.h"
#include "Level1.h"
#include "gamestatemanager.h"
#include "GameStateList.h"
#include "Main.h"
#include <iostream>
#include <fstream>
#include <cmath>          /* fabsf -ths */
#include <cstring>        /* strlen -ths */

/* ------------------------------ NEW: minimal, compatible additions --------------------------------
   Everything added in this file is kept local to Level1 and uses your existing engine and globals. -ths
   We do NOT modify GSM/states or other modules. All new comments end with '-ths'.                   -ths
--------------------------------------------------------------------------------------------------- */

/* NEW: forward decls to use functions/vars defined in leveleditor.cpp safely without editing headers -ths */
extern void WorldToGrid(float worldX, float worldY, int& outRow, int& outCol);           // -ths
extern void GridToWorldCenter(int row, int col, float& outX, float& outY);              // -ths
extern bool canMove(float nextX, float nextY);                                           // -ths
extern int  level[18][32];                                                               // -ths
/*extern int  ROWS, COLS;   */                                                               // -ths

/* NEW: forward decls to reuse mouse + click helpers (already in your project) -ths */
extern void TransformScreentoWorld(s32& mouseX, s32& mouseY);                            // -ths
extern bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height,
    float click_x, float click_y);                                  // -ths

//                        --- Variables declaration start here --- (original)
static bool level1_initialised = false; // initialisation flag
Entity player;
Entity mummy;
Entity exitPortal; //may call it exit portal // Added for Win Condition
//static AEGfxVertexList* pMesh = nullptr; // Pointer for the square mesh
Entity coin; // Added for Coin Collection
Entity wall;
int coinCounter = 0; // To track how many coins the player has collected
// Logic for balancing
int turnCounter = 0; // To make the mummy move every 2nd turn
AEGfxTexture* gDesertBlockTex = nullptr;
int level1_counter = 0;
int live1_counter = 3; // If want to include lives, adjust number of lives here
bool playerMoved = false;
float gridStep = 50.0f;
float nextX = player.x;
float nextY = player.y;

//																--- Variables declaration end here ---
//float nextX = 0.0f;
//float nextY = 0.0f;
//                        --- Variables declaration end here --- (original)


// ========================== NEW: enemy variants & powerups (local-only) ===========================
// Enemy types for minimal variety without touching GSM -ths
enum EnemyType { ENEMY_SCOUT = 1, ENEMY_BRUTE = 2 }; // your mummy remains as-is; these are extra -ths

struct ExtraEnemy { float x, y, size; float r, g, b; EnemyType type; }; // kept simple -ths
static ExtraEnemy gExtraEnemies[8]; // tiny, fixed list to avoid containers -ths
static int gExtraEnemyCount = 0;    // number of extra enemies -ths

static void SpawnExtraEnemy(float x, float y, EnemyType t) // minimal spawner -ths
{
    if (gExtraEnemyCount >= (int)(sizeof(gExtraEnemies) / sizeof(gExtraEnemies[0]))) return; // -ths
    ExtraEnemy& e = gExtraEnemies[gExtraEnemyCount++];
    e.x = x; e.y = y; e.size = gridStep; e.type = t;
    if (t == ENEMY_SCOUT) { e.r = 0.9f; e.g = 0.5f; e.b = 0.0f; }  // orange -ths
    if (t == ENEMY_BRUTE) { e.r = 0.6f; e.g = 0.2f; e.b = 0.8f; }  // purple -ths
} // -ths

// Powerups encoded in tiles so the level editor remains unchanged -ths
enum PowerTile {
    TILE_POWER_SPEED = 5, // +1 extra tile per move for 4 turns -ths
    TILE_POWER_FREEZE = 6, // enemies skip moving for 3 turns -ths
    TILE_POWER_INVINCIBLE = 7, // ignore enemy touch for 4 turns (turn-based) -ths
    TILE_POWER_GOLD_5S = 8  // buff block = ~5 seconds immunity (frame-based) -ths
}; // -ths

static struct PowerState {
    bool   speed = false;      int    speedTurns = 0;     // -ths
    bool   freeze = false;     int    freezeTurns = 0;    // -ths
    bool   invincible = false; int    invTurns = 0;       // (turn-based) -ths
    int    invFrames = 0;      // (time-based, frame countdown) -ths
} gPower; // -ths

static void   GiveSpeed(int turns) { gPower.speed = true;      gPower.speedTurns = turns; } // -ths
static void   GiveFreeze(int turns) { gPower.freeze = true;     gPower.freezeTurns = turns; } // -ths
static void   GiveInvincibleTurns(int turns) { gPower.invincible = true; gPower.invTurns = turns; } // -ths
static void   GiveInvincibleFrames(int frames) { if (frames > gPower.invFrames) gPower.invFrames = frames; } // -ths
static bool   IsInvincibleNow() { return gPower.invincible || (gPower.invFrames > 0); } // -ths
static void   TickPowers() // called once per player turn (turn-based only) -ths
{
    if (gPower.speed && --gPower.speedTurns <= 0) gPower.speed = false; // -ths
    if (gPower.freeze && --gPower.freezeTurns <= 0) gPower.freeze = false; // -ths
    if (gPower.invincible && --gPower.invTurns <= 0) gPower.invincible = false; // -ths
} // -ths

/* NEW: per-frame countdown for time-based invincibility; called each Update when not paused -ths */
static void TickFramePowers()
{
    if (gPower.invFrames > 0) --gPower.invFrames; // ~5 seconds at 60 FPS when set to 300 -ths
} // -ths


// ========================== NEW: overlay flags + button layout (file-scope) =======================
/* These must be declared at file scope so they exist everywhere. -ths */
static bool gPaused = false; // P toggles -ths
static bool gShowLose = false; // show lose overlay -ths
static bool gShowWin = false; // show win overlay  -ths

static float kBtnRetryX = -200.0f;  // center X for Retry -ths
static float kBtnRetryY = -130.0f;  // center Y for Retry -ths
static float kBtnExitX = 200.0f;  // center X for Exit  -ths
static float kBtnExitY = -130.0f;  // center Y for Exit  -ths
static float kBtnW = 280.0f;  // width              -ths
static float kBtnH = 90.0f;  // height             -ths


// ========================== NEW: world<->NDC helpers & UI draw helpers ============================
static inline float ToNDCX(float worldX) { return worldX / ((float)AEGfxGetWindowWidth() * 0.5f); }  // -ths
static inline float ToNDCY(float worldY) { return worldY / ((float)AEGfxGetWindowHeight() * 0.5f); }  // -ths

/* length-aware centering for AEGfxPrint (Roboto@32; tune ndcPerChar if you change scale/font) -ths */
static float CenteredTextX(float centerWorldX, const char* text, float scale)
{
    const float ndcPerChar = 0.0165f * scale; // empirically tuned -ths
    float halfText = 0.5f * ndcPerChar * (float)std::strlen(text);
    return ToNDCX(centerWorldX) - halfText; // left-x for AEGfxPrint -ths
} // -ths

/* solid rectangle button using the shared square mesh -ths */
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
} // -ths

/* NEW: snap any world position to the exact center of the grid cell (prevents “on the lines”) -ths */
static void SnapToGridCenter(float inX, float inY, float& outX, float& outY)
{
    int rr, cc;
    WorldToGrid(inX, inY, rr, cc);
    GridToWorldCenter(rr, cc, outX, outY);
} // -ths


// ========================== NEW: Save / Load (local) ==============================================
static bool SaveLevel1State(const char* path)
{
    std::ofstream f(path);
    if (!f.is_open()) return false;                      // -ths
    f << player.x << ' ' << player.y << '\n';
    f << coinCounter << ' ' << turnCounter << '\n';
    f << (int)gPower.speed << ' ' << gPower.speedTurns << ' '
        << (int)gPower.freeze << ' ' << gPower.freezeTurns << ' '
        << (int)gPower.invincible << ' ' << gPower.invTurns << ' '
        << gPower.invFrames << '\n';
    f << gExtraEnemyCount << '\n';
    for (int i = 0; i < gExtraEnemyCount; ++i)
        f << (int)gExtraEnemies[i].type << ' ' << gExtraEnemies[i].x << ' ' << gExtraEnemies[i].y << '\n';
    return true;                                          // -ths
} // -ths

static bool LoadLevel1State(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;                      // -ths
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
    return true;                                          // -ths
} // -ths


// ========================== NEW: ensure at least one buff tile (8) exists =========================
static bool HasBuffTile8()
{
    for (int r = 0; r < GRID_ROWS; ++r)
        for (int c = 0; c < GRID_COLS; ++c)
            if (level[r][c] == 8) return true;
    return false;
} // -ths

static void EnsureBuffTilePresent()
{
    if (HasBuffTile8()) return;                           // nothing to do -ths
    int midR = GRID_ROWS / 2, midC = GRID_COLS / 2;                 // center-ish -ths
    for (int dr = -2; dr <= 2; ++dr)
    {
        for (int dc = -2; dc <= 2; ++dc)
        {
            int rr = midR + dr, cc = midC + dc;
            if (rr >= 0 && rr < GRID_ROWS && cc >= 0 && cc < GRID_COLS && level[rr][cc] == 0)
            {
                level[rr][cc] = 8;                        // place black buff -ths
                print_file();                             // persist change -ths
                return;
            }
        }
    }
    // fallback: force center if no empty found -ths
    level[midR][midC] = 8;
    print_file();
} // -ths


//----------------------------------------------------------------------------
// Reads Assets/level1.txt and populates the level[][] grid.
// The level editor also saves to Assets/level1.txt, so both always use the same file.
//----------------------------------------------------------------------------
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

    int  tile;
    char comma;
    for (int row = 0; row < GRID_ROWS; ++row)
        for (int col = 0; col < GRID_COLS; ++col)
            if (is >> tile >> comma)
                level[row][col] = tile;
            else
                level[row][col] = 0;

    is.close();
    std::cout << "Level1: Loaded grid from " << path << "\n";
}

//----------------------------------------------------------------------------
// Loads Level 1 resources and reads the level counter from a text file
//--------------------------------------------------------------------------- (original)
void Level1_Load()
{
    std::cout << "Level1:Load\n"; // Print onto standard output stream

    LoadLevelTxt();

    // Loading of blue player texture
    player.pTex = AEGfxTextureLoad("Assets/explorer.png");
    gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    mummy.pTex = AEGfxTextureLoad("Assets/Enemy.png");  // mummy texture
    coin.pTex = AEGfxTextureLoad("Assets/Coin.png");    // coin texturer
    exitPortal.pTex = AEGfxTextureLoad("Assets/Exit.png"); // exit portal texture
    pMesh = CreateSquareMesh();

    /* NEW: enemies much farther from player, snapped to grid centers -ths */
    gExtraEnemyCount = 0;

    // choose far cells and snap to center (so they don't sit on grid lines) -ths
    float ex, ey;
    GridToWorldCenter(2, 2, ex, ey);               // near top-left -ths
    SpawnExtraEnemy(ex, ey, ENEMY_SCOUT);
    GridToWorldCenter(GRID_ROWS - 3, GRID_COLS - 3, ex, ey); // near bottom-right -ths
    SpawnExtraEnemy(ex, ey, ENEMY_BRUTE);
}


//----------------------------------------------------------------------------
// Finds the nearest free (value == 0) grid cell starting from (startRow, startCol),
// searching outward in expanding rings. Skips any cell within minDistFromRow/Col
// (Manhattan distance) to enforce a safe gap from another entity.
//----------------------------------------------------------------------------
static void FindFreeSpawnCell(int startRow, int startCol, float& outX, float& outY,
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

                // Enforce minimum distance from avoided cell (e.g. player position)
                if (avoidRow >= 0 && avoidCol >= 0)
                {
                    int dist = abs(r - avoidRow) + abs(c - avoidCol); // Manhattan distance
                    if (dist < minDist) continue;
                }

                GridToWorldCenter(r, c, outX, outY);
                std::cout << "Spawn found at grid (" << r << "," << c << ")\n";
                return;
            }
        }
    }

    // Fallback
    GridToWorldCenter(startRow, startCol, outX, outY);
    std::cout << "Spawn fallback at grid (" << startRow << "," << startCol << ")\n";
}


//----------------------------------------------------------------------------
// Sets up the initial state and prepares it for gameplay
//---------------------------------------------------------------------------
void Level1_Initialize()
{

    std::cout << "Level1:Initialize\n"; // Print onto standard output stream

    /* Always reset powers and overlays on (re)enter, regardless of init flag */
    gPower = {};
    gPaused = false; gShowLose = false; gShowWin = false;

    // Force re-initialisation every time Initialize is called (handles restart correctly)
    level1_initialised = false;

    // Initialise positions
    if (!level1_initialised) {
        player.size = GRID_TILE_SIZE;
        mummy.size = GRID_TILE_SIZE;
        gridStep = GRID_TILE_SIZE;
        float px = 0.0f, py = 0.0f;

        level1_initialised = true;

        // Find a free cell near the intended player spawn (center-left area)
        FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
        player.x = px;
        player.y = py;
        player.size = 50.0f;
        player.r = 0.0f; player.g = 0.0f; player.b = 1.0f;

        // Convert player world pos back to grid to use as avoidance reference for mummy
        int playerRow, playerCol;
        WorldToGrid(player.x, player.y, playerRow, playerCol);

        // Spawn mummy in the opposite corner, at least 10 cells away from player
        FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
        mummy.x = px; mummy.y = py;
        //mummy.size = 40.0f;
        mummy.size = 50.0f; // Adjusted to fit the grid better
        mummy.r = 1.0f; mummy.g = 0.0f; mummy.b = 0.0f;

        // Find a free cell near center-right for the exit portal
        FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS - 5, px, py);
        exitPortal.x = px;
        exitPortal.y = py;
        exitPortal.size = 40.0f;
        exitPortal.r = 1.0f; exitPortal.g = 1.0f; exitPortal.b = 0.0f;

        // Find a free cell for coin, away from player and exit
        FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
        coin.x = px;
        coin.y = py;
        coin.size = 30.0f;
        coin.r = 1.0f; coin.g = 0.5f; coin.b = 0.0f;

        wall.x = -60.0f;
        wall.y = 0.0f;
        //wall.size = 60.0f;
        wall.size = 50.0f; // Adjusted to fit the grid better
        wall.r = 0.2f; wall.g = 0.2f; wall.b = 0.2f;

        nextX = player.x;
        nextY = player.y;
        coinCounter = 0;
        turnCounter = 0;
        playerMoved = false;
        level1_initialised = true;
    }
}


//----------------------------------------------------------------------------
// Updates game logic and state each frame during gameplay
// --------------------------------------------------------------------------- (original)
void Level1_Update()
{
    level1_counter--; // Decrement counter for level
    if (level1_counter == 0)
    {
        level1_initialised = false; // Reset for next time
        next = MAINMENUSTATE; // sharon: for now setting it as go back to main menu as havent set up level 2, hence the gsm would make it just loop until the system closes itself
    }

    // Back / Quit
    if (AEInputCheckReleased(AEVK_B) || AEInputCheckReleased(AEVK_ESCAPE)) { next = LEVELPAGE; }
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) { next = GS_QUIT; }

    /* NEW: when overlay visible, handle UI & early-out -ths */
    if (gShowLose || gShowWin)
    {
        s32 mxS, myS; TransformScreentoWorld(mxS, myS);   // returns s32 -ths
        float mx = (float)mxS, my = (float)myS;           // explicit cast avoids warnings -ths

        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            if (IsAreaClicked(kBtnRetryX, kBtnRetryY, kBtnW, kBtnH, mx, my))
            {
                next = GS_LEVEL1; // go directly, Level1_Initialize always resets now
                gShowLose = gShowWin = false;
                return;
            }
            if (IsAreaClicked(kBtnExitX, kBtnExitY, kBtnW, kBtnH, mx, my))
            {
                next = MAINMENUSTATE;
                gShowLose = gShowWin = false;
                return;
            }
        }
        if (AEInputCheckReleased(AEVK_R)) { next = GS_LEVEL1; gShowLose = gShowWin = false; return; }
        if (AEInputCheckReleased(AEVK_RETURN)) { next = MAINMENUSTATE;                   gShowLose = gShowWin = false; return; }
        if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) { next = GS_QUIT; return; }

        return; // freeze normal update while overlays up -ths
    }

    /* NEW: Pause toggle & early-out -ths */
    if (AEInputCheckReleased(AEVK_P)) { gPaused = !gPaused; }
    if (gPaused) { return; }

    /* NEW: Save/Load (F5/F9) -ths */
    if (AEInputCheckReleased(AEVK_F5)) { if (SaveLevel1State("Assets/save1.txt")) std::cout << "Saved (Assets/save1.txt)\n"; }
    if (AEInputCheckReleased(AEVK_F9)) { if (LoadLevel1State("Assets/save1.txt")) std::cout << "Loaded (Assets/save1.txt)\n"; }

    // MOVEMENT UPDATE
// 1. Calculate potential next position based on input
    float testNextX = player.x;
    float testNextY = player.y;

    if (AEInputCheckTriggered(AEVK_W))      testNextY += gridStep;
    else if (AEInputCheckTriggered(AEVK_S)) testNextY -= gridStep;
    else if (AEInputCheckTriggered(AEVK_A)) testNextX -= gridStep;
    else if (AEInputCheckTriggered(AEVK_D)) testNextX += gridStep;

    // 2. Use the new Utility function to check the grid
    if ((testNextX != player.x || testNextY != player.y)) {
        if (IsTileWalkable(testNextX, testNextY)) {
            player.x = testNextX;
            player.y = testNextY;
            playerMoved = true; // This triggers the Mummy's turn
        }
    }

    // Bounding Box Collision Check for Player vs Wall
    bool playerWallCollision = (fabsf(testNextX - wall.x) < (player.size / 2.0f + wall.size / 2.0f)) &&
        (fabsf(testNextY - wall.y) < (player.size / 2.0f + wall.size / 2.0f));

    //// Collision Check: Only move if the next position isn't the wall
    //if ((testNextX != player.x || testNextY != player.y) && !playerWallCollision) {
    //	player.x = testNextX;
    //	player.y = testNextY;
    //	playerMoved = true;
    //																			-- the above commented out collision function is to replaced the grey wall with the gridbased wall.... -- YT
    // Replaced the old AABB wall check with the Grid check to fit the new grid-based movement and level design in level editor -- YT
    //if ((testNextX != player.x || testNextY != player.y) && canMove(testNextX, testNextY)) {
    //	player.x = testNextX;
    //	player.y = testNextY;
    //	playerMoved = true;
    //}


    //// Player Movement logic 
    //if (AEInputCheckTriggered(AEVK_W)) { player.y += gridStep; playerMoved = true; }
    //else if (AEInputCheckTriggered(AEVK_S)) { player.y -= gridStep; playerMoved = true; }
    //else if (AEInputCheckTriggered(AEVK_A)) { player.x -= gridStep; playerMoved = true; }
    //else if (AEInputCheckTriggered(AEVK_D)) { player.x += gridStep; playerMoved = true; }

    //													--- Basic Mummy AI (Balanced for winnable gameplay) ---   -- Debugging mummy movement 5/3 --  YT


    if (playerMoved)
    {
        turnCounter++;

        // 2. Coin Collection (Using value 4)
        int r, c;
        WorldToGrid(player.x, player.y, r, c);
        if (level[r][c] == 4) { // We use '4' because enum isn't in header
            level[r][c] = 0;    // Change back to EMPTY
            coinCounter++;
            std::cout << "Collected! Coins: " << coinCounter << "\n";
        }

        //				                                             -- Enemy movement one step toward player every 2 steps taken by player  --- YT 6/3
        if (turnCounter % 2 == 0)
        {
            //  Calculate the target direction based on player position
            float diffX = player.x - mummy.x;
            float diffY = player.y - mummy.y;

            //  MOVE HORIZONTALLY FIRST
            if (fabsf(diffX) > 1.0f) {
                float stepX = (diffX > 0) ? gridStep : -gridStep;
                if (canMove(mummy.x + stepX, mummy.y)) {
                    mummy.x += stepX;
                }
            }

            //  MOVE VERTICALLY SECOND
            // By checking mummy.x again, we ensure it moved to the new tile first
            diffY = player.y - mummy.y;
            if (fabsf(diffY) > 1.0f) {
                float stepY = (diffY > 0) ? gridStep : -gridStep;
                if (canMove(mummy.x, mummy.y + stepY)) {
                    mummy.y += stepY;
                }
            }
        }

        printf("Turn: %d | Player: (%.0f, %.0f) | Mummy: (%.0f, %.0f)\n",
            turnCounter, player.x, player.y, mummy.x, mummy.y);

        TickPowers(); // turn-based durations -ths
        playerMoved = false;
    }

    // effective inv
    const bool effectiveInv = IsInvincibleNow();

    // lose checks
    if (!effectiveInv && fabsf(player.x - mummy.x) < 1.0f && fabsf(player.y - mummy.y) < 1.0f)
    {
        ResetLevel1();
        printf("Caught by the Mummy! Level Reset!\n");
        gShowLose = true;
    }
    if (!effectiveInv) {
        for (int i = 0; i < gExtraEnemyCount; ++i) {
            if (fabsf(player.x - gExtraEnemies[i].x) < 1.0f &&
                fabsf(player.y - gExtraEnemies[i].y) < 1.0f) {
                ResetLevel1();
                printf("Caught by an Enemy! Level Reset!\n"); gShowLose = true; break;
            }
        }
    }

    // win check
    if (fabsf(player.x - exitPortal.x) < 1.0f && fabsf(player.y - exitPortal.y) < 1.0f)
    {
        printf("You Escaped the Maze!\n");
        level1_counter = 0;
        next = GS_WIN;
    }

    // legacy coin entity
    if (fabsf(player.x - coin.x) < 1.0f && fabsf(player.y - coin.y) < 1.0f)
    {
        ++coinCounter;
        printf("Coin Collected! Total Coins: %d\n", coinCounter);
        coin.x = 2000.0f; coin.y = 2000.0f;
    }
}


//----------------------------------------------------------------------------
// Renders the visual representation each frame, including the grid tiles
// ---------------------------------------------------------------------------
void Level1_Draw()
{
    AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);

    // Skip game rendering entirely when overlay is active - the overlay draws its own full screen
    if (gShowLose) { LosePage_Draw(); return; }
    if (gShowWin) { WinPage_Draw();  return; }
    if (gPaused) { PausePage_Draw(); return; }

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxTextureSet(gDesertBlockTex, 0, 0);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    AEMtx33 transform, scale, trans;

    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            // Use your enum/constants (1 = NON_WALKABLE)
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
            // You can add additional 'else if' blocks here to draw 
            // other objects based on the values (e.g., case 4 for coins)
        }
    }

    // 3. Render Player (Texture Mode)
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(player.pTex, 0, 0);
    AEMtx33Scale(&scale, player.size, player.size);
    AEMtx33Trans(&trans, player.x, player.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // 4. Render Mummy and Exit Portal (Color Mode)
    //AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    //AEGfxSetBlendMode(AE_GFX_BM_NONE);

    // Render Mummy
    //AEGfxSetColorToMultiply(mummy.r, mummy.g, mummy.b, 1.0f);
    AEGfxTextureSet(mummy.pTex, 0, 0); // Set the mummy texture
    AEMtx33Scale(&scale, mummy.size, mummy.size);
    AEMtx33Trans(&trans, mummy.x, mummy.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // coin render
    if (coin.x < 1000.0f)
    {
        AEGfxTextureSet(coin.pTex, 0, 0);
        AEMtx33Scale(&scale, coin.size, coin.size);
        AEMtx33Trans(&trans, coin.x, coin.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Render Exit Portal
    //AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    //AEGfxSetColorToMultiply(exitPortal.r, exitPortal.g, exitPortal.b, 1.0f);
    AEGfxTextureSet(exitPortal.pTex, 0, 0);
    AEMtx33Scale(&scale, exitPortal.size, exitPortal.size);
    AEMtx33Trans(&trans, exitPortal.x, exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    DrawGridLines(pMesh);	// Render Grid Tiles
}
// level1 draw end here 



//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data
// --------------------------------------------------------------------------- (original)
void Level1_Free()
{
    std::cout << "Level1:Free\n"; // Print onto standard output stream
}


//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level
// --------------------------------------------------------------------------- (original)
void Level1_Unload()
{
    std::cout << "Level1:Unload\n"; // Print onto standard output stream

    // Unload Texture here
    AEGfxTextureUnload(player.pTex);
    AEGfxTextureUnload(gDesertBlockTex);
    AEGfxTextureUnload(mummy.pTex);
    AEGfxTextureUnload(coin.pTex);
    AEGfxTextureUnload(exitPortal.pTex);
    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }

    level1_initialised = false; // Reset for next time level is loaded
}


//                        ===== HELPER FUNCTIONS =====
//----------------------------------------------------------------------------
// Resets level when mummy catches player. A manual reset is needed as
// initialise only runs once at the entry of a level, so need a reset for if
// player dies within the level itself.
// --------------------------------------------------------------------------- (original)
void ResetLevel1()
{
    float px, py;

    // Player - center-left area
    FindFreeSpawnCell(GRID_ROWS / 2, 4, px, py);
    player.x = px; player.y = py;

    // Mummy - opposite corner, at least 10 cells from player
    int playerRow, playerCol;
    WorldToGrid(player.x, player.y, playerRow, playerCol);
    FindFreeSpawnCell(2, GRID_COLS - 3, px, py, playerRow, playerCol, 10);
    mummy.x = px; mummy.y = py;

    // Coin - center of map
    FindFreeSpawnCell(GRID_ROWS / 2, GRID_COLS / 2, px, py);
    coin.x = px; coin.y = py;

    nextX = player.x;
    nextY = player.y;
    coinCounter = 0;
    turnCounter = 0;
    playerMoved = false;

    gPower.speed = false; gPower.speedTurns = 0;
    gPower.freeze = false; gPower.freezeTurns = 0;
    gPower.invincible = false; gPower.invTurns = 0;
    gPower.invFrames = 0;
}