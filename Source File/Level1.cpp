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
/* End Header *************************************************************** */

#include "pch.h"
#include "Level1.h"
#include "gamestatemanager.h"
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
extern int  ROWS, COLS;                                                                  // -ths

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
float nextX = 0.0f;
float nextY = 0.0f;
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
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
            if (level[r][c] == 8) return true;
    return false;
} // -ths

static void EnsureBuffTilePresent()
{
    if (HasBuffTile8()) return;                           // nothing to do -ths
    int midR = ROWS / 2, midC = COLS / 2;                 // center-ish -ths
    for (int dr = -2; dr <= 2; ++dr)
    {
        for (int dc = -2; dc <= 2; ++dc)
        {
            int rr = midR + dr, cc = midC + dc;
            if (rr >= 0 && rr < ROWS && cc >= 0 && cc < COLS && level[rr][cc] == 0)
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
// Loads Level 1 resources and reads the level counter from a text file
//--------------------------------------------------------------------------- (original)
void Level1_Load()
{
    std::cout << "Level1:Load\n"; // Print onto standard output stream
    readfile();
    EnsureBuffTilePresent();                /* ensure a black buff tile exists -ths */
    print_file();

    // Loading of blue player texture
    player.pTex = AEGfxTextureLoad("Assets/Player.jpg");
    gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    pMesh = CreateSquareMesh();

    /* NEW: enemies much farther from player, snapped to grid centers -ths */
    gExtraEnemyCount = 0;

    // choose far cells and snap to center (so they don't sit on grid lines) -ths
    float ex, ey;
    GridToWorldCenter(2, 2, ex, ey);               // near top-left -ths
    SpawnExtraEnemy(ex, ey, ENEMY_SCOUT);
    GridToWorldCenter(ROWS - 3, COLS - 3, ex, ey); // near bottom-right -ths
    SpawnExtraEnemy(ex, ey, ENEMY_BRUTE);
}


//----------------------------------------------------------------------------
// Sets up the initial state and prepares it for gameplay
// --------------------------------------------------------------------------- (original)
void Level1_Initialize()
{
    std::cout << "Level1:Initialize\n"; // Print onto standard output stream
    // Initialise positions only once
    if (!level1_initialised) {

        /* NEW: on fresh enter or Retry, reload the grid and re‑add buff tile if missing -ths */
        readfile();                 // reload "level1.txt" so consumed tiles (like 8) come back -ths
        EnsureBuffTilePresent();    // guarantees there is at least one black buff block -ths
        print_file();               // optional: persist in case we just added one -ths
        // ------------------------------------------------------------------ -ths

        // snap player to grid center as well (prevents partial offsets) -ths
        float px, py;
        GridToWorldCenter(ROWS / 2 + 1, COLS / 2 - 4, px, py); /* close to center-left -ths */
        player.x = px; player.y = py;

        /*    player.size = 40.0f;*/
        player.size = 50.0f; // Adjusted to fit the grid better as the size of grid is 50.0f in leveleditor -- YT
        player.r = 0.0f; player.g = 0.0f; player.b = 1.0f;

        /* MOVE MUMMY FARTHER AWAY FROM PLAYER and snap to grid -ths */
        GridToWorldCenter(3, 3, px, py);
        mummy.x = px; mummy.y = py;
        //mummy.size = 40.0f;
        mummy.size = 50.0f; // Adjusted to fit the grid better
        mummy.r = 1.0f; mummy.g = 0.0f; mummy.b = 0.0f;

        GridToWorldCenter(ROWS / 2, COLS / 2 + 4, px, py);
        exitPortal.x = px;
        exitPortal.y = py;
        exitPortal.size = 40.0f;
        exitPortal.r = 1.0f; exitPortal.g = 1.0f; exitPortal.b = 0.0f;

        coin.x = 25.0f;
        coin.y = 75.0f;
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

    /* NEW: reset powers and overlays on (re)enter -ths */
    gPower = {};
    gPaused = false; gShowLose = false; gShowWin = false; // -ths
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
                /* Robust restart + force re-init of Level 1 -ths */
                previous = current;        // ensure previous=GS_LEVEL1 -ths
                next = GS_RESTART;     // main loop will swap back to previous -ths
                level1_initialised = false; // force Level1_Initialize() to run fresh -ths

                // clear overlay and powers immediately (prevents one-frame carryover) -ths
                gShowLose = gShowWin = false;
                gPower.speed = false; gPower.speedTurns = 0;
                gPower.freeze = false; gPower.freezeTurns = 0;
                gPower.invincible = false; gPower.invTurns = 0;
                gPower.invFrames = 0;

                return;
            }
            if (IsAreaClicked(kBtnExitX, kBtnExitY, kBtnW, kBtnH, mx, my))
            {
                next = MAINMENUSTATE;
                gShowLose = gShowWin = false;
                return;
            }
        }
        if (AEInputCheckReleased(AEVK_R)) { previous = current; next = GS_RESTART; gShowLose = gShowWin = false; return; }
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
    float testNextX = player.x;
    float testNextY = player.y;
    if (AEInputCheckTriggered(AEVK_W)) testNextY += gridStep;
    else if (AEInputCheckTriggered(AEVK_S)) testNextY -= gridStep;
    else if (AEInputCheckTriggered(AEVK_A)) testNextX -= gridStep;
    else if (AEInputCheckTriggered(AEVK_D)) testNextX += gridStep;

    // Grid based collision
    if ((testNextX != player.x || testNextY != player.y) && canMove(testNextX, testNextY)) {
        player.x = testNextX;
        player.y = testNextY;
        playerMoved = true;

        /* NEW: Speed extra tile attempt -ths */
        if (gPower.speed) {
            float ex = 0.0f, ey = 0.0f;
            if (AEInputCheckTriggered(AEVK_W))      ey = gridStep;
            else if (AEInputCheckTriggered(AEVK_S)) ey = -gridStep;
            else if (AEInputCheckTriggered(AEVK_A)) ex = -gridStep;
            else if (AEInputCheckTriggered(AEVK_D)) ex = gridStep;
            float sx = player.x + ex, sy = player.y + ey;
            if ((ex != 0.0f || ey != 0.0f) && canMove(sx, sy)) { player.x = sx; player.y = sy; }
        }
    }

    // frame powers tick
    TickFramePowers();

    // turn-based phase when player moved
    if (playerMoved)
    {
        turnCounter++;
        int r, c; WorldToGrid(player.x, player.y, r, c);

        // coin pickup
        if (level[r][c] == 4) { level[r][c] = 0; ++coinCounter; std::cout << "Collected! Coins: " << coinCounter << "\n"; }

        // power pickups
        if (level[r][c] == TILE_POWER_SPEED) { level[r][c] = 0; GiveSpeed(4);              std::cout << "Speed x2 (4 turns)\n"; }
        else if (level[r][c] == TILE_POWER_FREEZE) { level[r][c] = 0; GiveFreeze(3);             std::cout << "Freeze enemies (3 turns)\n"; }
        else if (level[r][c] == TILE_POWER_INVINCIBLE) { level[r][c] = 0; GiveInvincibleTurns(4);    std::cout << "Invincible (4 turns)\n"; }
        else if (level[r][c] == TILE_POWER_GOLD_5S) { level[r][c] = 0; GiveInvincibleFrames(300); std::cout << "Invincible (~5 seconds)\n"; }

        // mummy moves every 2 turns
        if (turnCounter % 2 == 0)
        {
            float diffX = player.x - mummy.x;
            float diffY = player.y - mummy.y;
            if (fabsf(diffX) > 1.0f) {
                float stepX = (diffX > 0) ? gridStep : -gridStep;
                if (canMove(mummy.x + stepX, mummy.y)) mummy.x += stepX;
            }
            diffY = player.y - mummy.y;
            if (fabsf(diffY) > 1.0f) {
                float stepY = (diffY > 0) ? gridStep : -gridStep;
                if (canMove(mummy.x, mummy.y + stepY)) mummy.y += stepY;
            }
        }

        // extra enemies
        for (int i = 0; i < gExtraEnemyCount; ++i) {
            bool moveIt = false;
            if (gPower.freeze) moveIt = false;
            else if (gExtraEnemies[i].type == ENEMY_SCOUT) moveIt = true;
            else if (gExtraEnemies[i].type == ENEMY_BRUTE) moveIt = (turnCounter % 3 == 0);
            if (!moveIt) continue;

            float dx = player.x - gExtraEnemies[i].x;
            float dy = player.y - gExtraEnemies[i].y;
            if (fabsf(dx) >= fabsf(dy)) {
                float step = (dx > 0 ? gridStep : -gridStep);
                float nx = gExtraEnemies[i].x + step;
                if (canMove(nx, gExtraEnemies[i].y)) gExtraEnemies[i].x = nx;
            }
            else {
                float step = (dy > 0 ? gridStep : -gridStep);
                float ny = gExtraEnemies[i].y + step;
                if (canMove(gExtraEnemies[i].x, ny)) gExtraEnemies[i].y = ny;
            }
            /* snap after move to avoid drift from float ops (keeps centers perfect) -ths */
            SnapToGridCenter(gExtraEnemies[i].x, gExtraEnemies[i].y, gExtraEnemies[i].x, gExtraEnemies[i].y);
        }

        printf("Turn: %d \n Player: (%.0f, %.0f) \n Mummy: (%.0f, %.0f)\n",
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
        gShowWin = true;
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
// Renders or draws the visual representation each frame
// --------------------------------------------------------------------------- (original)
void Level1_Draw()
{
    // background
    AEGfxSetBackgroundColor(255.0f, 255.0f, 255.0f);

    AEMtx33 transform, scale, trans;

    // player (texture fallback safe) -ths
    if (player.pTex)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(player.pTex, 0, 0);
    }
    else
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_NONE);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 1.0f, 1.0f);
    }
    AEMtx33Scale(&scale, player.size, player.size);
    AEMtx33Trans(&trans, player.x, player.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // switch to color for others
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);

    // mummy
    AEGfxSetColorToMultiply(mummy.r, mummy.g, mummy.b, 1.0f);
    AEMtx33Scale(&scale, mummy.size, mummy.size);
    AEMtx33Trans(&trans, mummy.x, mummy.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Render exit portal (Yellow Square) + label (CENTERED) -ths
    AEGfxSetColorToMultiply(exitPortal.r, exitPortal.g, exitPortal.b, 1.0f);
    AEMtx33Scale(&scale, exitPortal.size, exitPortal.size);
    AEMtx33Trans(&trans, exitPortal.x, exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Center "EXIT" over/right of the exit tile (world -> NDC) --- -ths
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    {
        const float lblScale = 1.0f; // same size as before -ths
        // horizontally centered; nudge +20 world pixels so text appears to the right of the tile -ths
        float exitTextX = CenteredTextX(exitPortal.x + 20.0f, "EXIT", lblScale);
        // slight vertical nudge above tile center for readability -ths
        float exitTextY = ToNDCY(exitPortal.y + 5.0f);
        AEGfxPrint(fontId, "EXIT", exitTextX, exitTextY, lblScale, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    // extra enemies
    for (int i = 0; i < gExtraEnemyCount; ++i)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_NONE);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(gExtraEnemies[i].r, gExtraEnemies[i].g, gExtraEnemies[i].b, 1.0f);
        AEMtx33Scale(&scale, gExtraEnemies[i].size, gExtraEnemies[i].size);
        AEMtx33Trans(&trans, gExtraEnemies[i].x, gExtraEnemies[i].y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // HUD
    if (gPower.speed)      AEGfxPrint(fontId, "SPEED", -0.98f, 0.90f, 0.8f, 1.0f, 1.0f, 0.0f, 1.0f);
    if (gPower.freeze)     AEGfxPrint(fontId, "FREEZE", -0.98f, 0.84f, 0.8f, 0.0f, 1.0f, 1.0f, 1.0f);
    if (IsInvincibleNow()) AEGfxPrint(fontId, "INVINCIBLE", -0.98f, 0.78f, 0.8f, 1.0f, 0.0f, 1.0f, 1.0f);

    // tiles (walls, coins, buff blocks)
    generateLevel();

    // ===== Overlays with RGB(200,200,200) buttons =====
    if (gPaused)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.6f);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.6f);
        AEMtx33 s, t, m; AEMtx33Scale(&s, (float)AEGfxGetWindowWidth(), (float)AEGfxGetWindowHeight());
        AEMtx33Trans(&t, 0.0f, 0.0f); AEMtx33Concat(&m, &t, &s);
        AEGfxSetTransform(m.m); AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxPrint(fontId, "PAUSED (P to Resume)", -0.22f, 0.02f, 1.4f, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (gShowLose)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.80f, 0.00f, 0.00f, 1.0f);
        AEMtx33 s, t, m; AEMtx33Scale(&s, (float)AEGfxGetWindowWidth(), (float)AEGfxGetWindowHeight());
        AEMtx33Trans(&t, 0.0f, 0.0f); AEMtx33Concat(&m, &t, &s);
        AEGfxSetTransform(m.m); AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxPrint(fontId, "CAUGHT! (R to Restart from Menu)", -0.35f, 0.25f, 1.2f, 1.0f, 1.0f, 1.0f, 1.0f);

        const float L = 200.0f / 255.0f;
        DrawButtonRect(kBtnRetryX, kBtnRetryY, kBtnW, kBtnH, L, L, L);
        DrawButtonRect(kBtnExitX, kBtnExitY, kBtnW, kBtnH, L, L, L);
        const float lblScale = 1.2f;
        AEGfxPrint(fontId, "RETRY", CenteredTextX(kBtnRetryX, "RETRY", lblScale), ToNDCY(kBtnRetryY) + 0.01f, lblScale, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(fontId, "EXIT", CenteredTextX(kBtnExitX, "EXIT", lblScale), ToNDCY(kBtnExitY) + 0.01f, lblScale, 0.0f, 0.0f, 0.0f, 1.0f);
        const char* help = "R: Retry  |  ENTER: Main Menu  |  Q: Quit";
        const float helpScale = 0.9f;
        AEGfxPrint(fontId, help, CenteredTextX(0.0f, help, helpScale), ToNDCY(kBtnExitY - 70.0f), helpScale, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (gShowWin)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.00f, 0.70f, 0.00f, 1.0f);
        AEMtx33 s, t, m; AEMtx33Scale(&s, (float)AEGfxGetWindowWidth(), (float)AEGfxGetWindowHeight());
        AEMtx33Trans(&t, 0.0f, 0.0f); AEMtx33Concat(&m, &t, &s);
        AEGfxSetTransform(m.m); AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxPrint(fontId, "YOU ESCAPED!", -0.20f, 0.25f, 1.4f, 1.0f, 1.0f, 1.0f, 1.0f);

        const float L = 200.0f / 255.0f;
        DrawButtonRect(kBtnRetryX, kBtnRetryY, kBtnW, kBtnH, L, L, L);
        DrawButtonRect(kBtnExitX, kBtnExitY, kBtnW, kBtnH, L, L, L);
        const float lblScale = 1.2f;
        AEGfxPrint(fontId, "RETRY", CenteredTextX(kBtnRetryX, "RETRY", lblScale), ToNDCY(kBtnRetryY) + 0.01f, lblScale, 0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxPrint(fontId, "EXIT", CenteredTextX(kBtnExitX, "EXIT", lblScale), ToNDCY(kBtnExitY) + 0.01f, lblScale, 0.0f, 0.0f, 0.0f, 1.0f);
        const char* help = "R: Retry  |  ENTER: Main Menu  |  Q: Quit";
        const float helpScale = 0.9f;
        AEGfxPrint(fontId, help, CenteredTextX(0.0f, help, helpScale), ToNDCY(kBtnExitY - 70.0f), helpScale, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}


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
    AEGfxTextureUnload(player.pTex);
    AEGfxTextureUnload(gDesertBlockTex);
    if (pMesh) { AEGfxMeshFree(pMesh); pMesh = nullptr; }
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
    // Re-snap to grid centers to avoid any drift -ths
    float px, py;
    GridToWorldCenter(ROWS / 2 + 1, COLS / 2 - 4, px, py);
    player.x = px; player.y = py;

    GridToWorldCenter(3, 3, px, py);
    mummy.x = px; mummy.y = py;

    coin.x = 25.0f; coin.y = 75.0f;
    nextX = player.x;
    nextY = player.y;
    coinCounter = 0;
    turnCounter = 0;
    playerMoved = false;

    /* NEW: reset powers on reset for consistency (drop both invincibilities) -ths */
    gPower.speed = false; gPower.speedTurns = 0;
    gPower.freeze = false; gPower.freezeTurns = 0;
    gPower.invincible = false; gPower.invTurns = 0;
    gPower.invFrames = 0;

    level1_initialised = false; /* ensure a clean re-init on next enter -ths */
}
