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

/* ------------------------------ NEW: minimal, compatible additions --------------------------------
   Everything added in this file is kept local to Level1 and uses your existing engine and globals. -ths
   We do NOT modify GSM/states or other modules. All new comments end with '-ths'.                   -ths
--------------------------------------------------------------------------------------------------- */

/* NEW: forward decls to use functions/vars defined in leveleditor.cpp safely without editing headers -ths */
extern void WorldToGrid(float worldX, float worldY, int& outRow, int& outCol);           // -ths
extern bool canMove(float nextX, float nextY);                                           // -ths
extern int  level[18][32];                                                               // -ths

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
    TILE_POWER_INVINCIBLE = 7  // ignore enemy touch for 4 turns -ths
}; // -ths

static struct PowerState {
    bool speed = false;      int speedTurns = 0;       // -ths
    bool freeze = false;     int freezeTurns = 0;      // -ths
    bool invincible = false; int invTurns = 0;         // -ths
} gPower; // -ths

static void GiveSpeed(int turns) { gPower.speed = true;      gPower.speedTurns = turns; } // -ths
static void GiveFreeze(int turns) { gPower.freeze = true;     gPower.freezeTurns = turns; } // -ths
static void GiveInvincible(int turns) { gPower.invincible = true; gPower.invTurns = turns; } // -ths
static void TickPowers() // called once per player turn -ths
{
    if (gPower.speed && --gPower.speedTurns <= 0) gPower.speed = false; // -ths
    if (gPower.freeze && --gPower.freezeTurns <= 0) gPower.freeze = false; // -ths
    if (gPower.invincible && --gPower.invTurns <= 0) gPower.invincible = false; // -ths
} // -ths

// Minimal pause & overlays that don't need GSM changes -ths
static bool gPaused = false; // P toggles -ths
static bool gShowLose = false; // show lose overlay after being caught -ths
static bool gShowWin = false; // lightweight win overlay (your existing flow retained) -ths

// Save/Load small helper to Assets/save1.txt (no other changes) -ths
static bool SaveLevel1State(const char* path)
{
    std::ofstream f(path);
    if (!f.is_open()) return false; // -ths
    f << player.x << " " << player.y << "\n";
    f << coinCounter << " " << turnCounter << "\n";
    f << (int)gPower.speed << " " << gPower.speedTurns << " "
        << (int)gPower.freeze << " " << gPower.freezeTurns << " "
        << (int)gPower.invincible << " " << gPower.invTurns << "\n";
    f << gExtraEnemyCount << "\n";
    for (int i = 0; i < gExtraEnemyCount; ++i)
        f << (int)gExtraEnemies[i].type << " " << gExtraEnemies[i].x << " " << gExtraEnemies[i].y << "\n";
    return true; // -ths
} // -ths

static bool LoadLevel1State(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false; // -ths
    f >> player.x >> player.y;
    f >> coinCounter >> turnCounter;
    int sp, fr, iv;
    f >> sp >> gPower.speedTurns >> fr >> gPower.freezeTurns >> iv >> gPower.invTurns;
    gPower.speed = (sp != 0); gPower.freeze = (fr != 0); gPower.invincible = (iv != 0);
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
    return true; // -ths
} // -ths

//----------------------------------------------------------------------------
// Loads Level 1 resources and reads the level counter from a text file
//--------------------------------------------------------------------------- (original)
void Level1_Load()
{
    std::cout << "Level1:Load\n"; // Print onto standard output stream
    readfile();
    print_file();
    //loadLevelMap(1);
    // Loading of blue player texture
    player.pTex = AEGfxTextureLoad("Assets/Player.jpg");
    gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    pMesh = CreateSquareMesh();

    /* NEW: spawn a couple of extra enemies without touching GSM or your original mummy -ths */
    gExtraEnemyCount = 0;                           // reset extras -ths
    SpawnExtraEnemy(75.0f, 75.0f, ENEMY_SCOUT); // moves every turn toward player -ths
    SpawnExtraEnemy(425.0f, -125.0f, ENEMY_BRUTE); // moves every 3rd turn but strides 2 tiles -ths
}

//----------------------------------------------------------------------------
// Sets up the initial state and prepares it for gameplay
// --------------------------------------------------------------------------- (original)
void Level1_Initialize()
{
    std::cout << "Level1:Initialize\n"; // Print onto standard output stream
    // Initialise positions only once
    if (!level1_initialised) {
        player.x = 225.0f;
        player.y = -125.0f;
        /*    player.size = 40.0f;*/
        player.size = 50.0f; // Adjusted to fit the grid better as the size of grid is 50.0f in leveleditor -- YT
        player.r = 0.0f; player.g = 0.0f; player.b = 1.0f;
        mummy.x = 325.0f;
        mummy.y = 175.0f;
        //mummy.size = 40.0f;
        mummy.size = 50.0f; // Adjusted to fit the grid better
        mummy.r = 1.0f; mummy.g = 0.0f; mummy.b = 0.0f;
        exitPortal.x = 425.0f;
        exitPortal.y = 25.0f;
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
    // sharon 3/3: Not sure if the below is necessary anymore. Have moved out the texture part to load instead
//    //                        
//
//    //                                    Initialize Mummy
//    // -- Uncomment when textures are used --    
//        mummy.x = 200.0f;
//        mummy.y = 200.0f;
//        mummy.width = 64.0f;
//        mummy.height = 64.0f;
//    
//
//                        
//        Initialize Font System
//    
//
//                                        Initialize Player
//    // -- Uncomment when textures are used --    
//        player.x = 0.0f;
//        player.y = 0.0f;
//        player.width = 64.0f;
//        player.height = 64.0f;

    /* NEW: reset powers and overlays on (re)enter -ths */
    gPower = {};
    gPaused = false; gShowLose = false; gShowWin = false; // -ths
}

//----------------------------------------------------------------------------
// Updates game logic and state each frame during gameplay
// --------------------------------------------------------------------------- (original)
void Level1_Update()
{
    // std::cout << "Level1:Update\n"; // Print onto standard output stream yt 25-2 comment up first, my computer cannot stand D:
    level1_counter--; // Decrement counter for level
    if (level1_counter == 0)
    {
        // Level 1 completed
        //next = GS_LEVEL2;
        level1_initialised = false; // Reset for next time
        next = MAINMENUSTATE; // sharon: for now setting it as go back to main menu as havent set up level 2, hence the gsm would make it just loop until the system closes itself
    }
    //// sharon 2/3: Loop for level 1 with lives implemented. commented out for the time being until system is fixed
    //if (level1_counter <= 0)
    //{
    //    live1_counter--; // Decrement life
    //    // Level 1 iteration completed
    //    if (live1_counter > 0)
    //    {
    //        next = GS_RESTART; // Will restart Level 1
    //    }
    //    else
    //    {
    //        next = MAINMENUSTATE; // Sharon 2/3 : No lives left, for now setting it as go back to main menu as havent set up level 2, hence the gsm would make it just loop until the system closes itself
    //    }
    //}
    // Move back to main menu upon triggering "B"
    if (AEInputCheckReleased(AEVK_B)
        || (AEInputCheckReleased(AEVK_ESCAPE)))
    {
        next = LEVELPAGE;
        std::cout << "Back key Released" << '\n'; // Debug purposes
    }
    // Quit game when Q is hit or when the window is closed
    if (AEInputCheckReleased(AEVK_Q)
        || 0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
        std::cout << "Q key Released" << '\n'; // Debug purposes
    }

    /* NEW: Pause toggle & early out while paused (overlay drawn in Draw) -ths */
    if (AEInputCheckReleased(AEVK_P)) { gPaused = !gPaused; } // -ths
    if (gPaused) { return; } // freeze updates while paused; resume with P again -ths

    /* NEW: Save/Load (F5/F9) to Assets/save1.txt with minimal I/O -ths */
    if (AEInputCheckReleased(AEVK_F5)) { if (SaveLevel1State("Assets/save1.txt")) std::cout << "Saved (Assets/save1.txt)\n"; } // -ths
    if (AEInputCheckReleased(AEVK_F9)) { if (LoadLevel1State("Assets/save1.txt")) std::cout << "Loaded (Assets/save1.txt)\n"; } // -ths

    // MOVEMENT UPDATE
    float testNextX = player.x;
    float testNextY = player.y;
    if (AEInputCheckTriggered(AEVK_W)) testNextY += gridStep;
    else if (AEInputCheckTriggered(AEVK_S)) testNextY -= gridStep;
    else if (AEInputCheckTriggered(AEVK_A)) testNextX -= gridStep;
    else if (AEInputCheckTriggered(AEVK_D)) testNextX += gridStep;
    // Bounding Box Collision Check for Player vs Wall
    bool playerWallCollision = (fabsf(testNextX - wall.x) < (player.size / 2.0f + wall.size / 2.0f)) &&
        (fabsf(testNextY - wall.y) < (player.size / 2.0f + wall.size / 2.0f));
    //// Collision Check: Only move if the next position isn't the wall
    //if ((testNextX != player.x || testNextY != player.y) && !playerWallCollision) {
    //    player.x = testNextX;
    //    player.y = testNextY;
    //    playerMoved = true;
    //                                    -- the above commented out collision function is to replaced the grey wall with the gridbased wall.... -- YT
    // Replaced the old AABB wall check with the Grid check to fit the new grid-based movement and level design in level editor -- YT
    if ((testNextX != player.x || testNextY != player.y) && canMove(testNextX, testNextY)) {
        player.x = testNextX;
        player.y = testNextY;
        playerMoved = true;

        /* NEW: Speed power attempts one extra tile in the same direction (safe) -ths */
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
    //// Player Movement logic
    //if (AEInputCheckTriggered(AEVK_W)) { player.y += gridStep; playerMoved = true; }
    //else if (AEInputCheckTriggered(AEVK_S)) { player.y -= gridStep; playerMoved = true; }
    //else if (AEInputCheckTriggered(AEVK_A)) { player.x -= gridStep; playerMoved = true; }
    //else if (AEInputCheckTriggered(AEVK_D)) { player.x += gridStep; playerMoved = true; }
    //                        --- Basic Mummy AI (Balanced for winnable gameplay) --- -- Debugging mummy movement 5/3 -- YT
    if (playerMoved)
    {
        turnCounter++;
        // 2. Coin Collection (Using value 4)
        int r, c;
        WorldToGrid(player.x, player.y, r, c);
        if (level[r][c] == 4) { // We use '4' because enum isn't in header
            level[r][c] = 0; // Change back to EMPTY
            coinCounter++;
            std::cout << "Collected! Coins: " << coinCounter << "\n";
        }

        /* NEW: Powerup pick-up (tiles 5..7) integrated next to your coin logic -ths */
        if (level[r][c] == TILE_POWER_SPEED) { level[r][c] = 0; GiveSpeed(4);      std::cout << "Speed x2 (4 turns)\n"; }       // -ths
        else if (level[r][c] == TILE_POWER_FREEZE) { level[r][c] = 0; GiveFreeze(3);     std::cout << "Freeze enemies (3 turns)\n"; } // -ths
        else if (level[r][c] == TILE_POWER_INVINCIBLE) { level[r][c] = 0; GiveInvincible(4); std::cout << "Invincible (4 turns)\n"; }     // -ths

        //                -- Enemy movement one step toward player every 2 steps taken by player --- YT 6/3
        if (turnCounter % 2 == 0)
        {
            // Calculate the target direction based on player position
            float diffX = player.x - mummy.x;
            float diffY = player.y - mummy.y;
            // MOVE HORIZONTALLY FIRST
            if (fabsf(diffX) > 1.0f) {
                float stepX = (diffX > 0) ? gridStep : -gridStep;
                if (canMove(mummy.x + stepX, mummy.y)) {
                    mummy.x += stepX;
                }
            }
            // MOVE VERTICALLY SECOND
            // By checking mummy.x again, we ensure it moved to the new tile first
            diffY = player.y - mummy.y;
            if (fabsf(diffY) > 1.0f) {
                float stepY = (diffY > 0) ? gridStep : -gridStep;
                if (canMove(mummy.x, mummy.y + stepY)) {
                    mummy.y += stepY;
                }
            }
        }

        /* NEW: Move extra enemies (Scout every turn, Brute every 3rd turn with double stride) -ths */
        for (int i = 0; i < gExtraEnemyCount; ++i) {
            bool shouldMove = false;
            if (gPower.freeze)                              shouldMove = false;            // freeze blocks all -ths
            else if (gExtraEnemies[i].type == ENEMY_SCOUT)  shouldMove = true;             // each turn -ths
            else if (gExtraEnemies[i].type == ENEMY_BRUTE)  shouldMove = (turnCounter % 3 == 0); // 3rd turn -ths
            if (!shouldMove) continue;

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
            if (gExtraEnemies[i].type == ENEMY_BRUTE) { // second step same turn -ths
                dx = player.x - gExtraEnemies[i].x;
                dy = player.y - gExtraEnemies[i].y;
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
            }
        }

        printf("Turn: %d \n Player: (%.0f, %.0f) \n Mummy: (%.0f, %.0f)\n",
            turnCounter, player.x, player.y, mummy.x, mummy.y);

        /* NEW: power durations tick down after enemy phase -ths */
        TickPowers(); // -ths
        playerMoved = false; // Reset flag after processing turn
    }
    //                        --- Lose Condition (Caught by Mummy) ---
    if (fabsf(player.x - mummy.x) < 1.0f && fabsf(player.y - mummy.y) < 1.0f)
    {
        // Sharon 2/3: commented out these as its shifted to initialise and reset
        //// Reset Player and Mummy
        //player.x = 225.0f;
        //player.y = -125.0f;
        //mummy.x = 325.0f;
        //mummy.y = 175.0f;
        //turnCounter = 0;
        //// --- RESET COIN HERE ---
        //coin.x = 25.0f; // Position it somewhere in the middle
        //coin.y = 75.0f;
        //coinCounter = 0;
        //                === BETWEEN GS_RESTART AND MANUAL RESTART WHICH IS BETTER? NEED TO TEST ===
        //next = GS_RESTART;
        ResetLevel1();
        printf("Caught by the Mummy! Level Reset!\n");
        gShowLose = true; /* NEW: show a small lose overlay without GSM change -ths */
    }

    /* NEW: also lose if an extra enemy touches (unless invincible) -ths */
    if (!gPower.invincible) {
        for (int i = 0; i < gExtraEnemyCount; ++i) {
            if (fabsf(player.x - gExtraEnemies[i].x) < 1.0f &&
                fabsf(player.y - gExtraEnemies[i].y) < 1.0f) {
                ResetLevel1();
                printf("Caught by an Enemy! Level Reset!\n"); gShowLose = true; break;
            }
        }
    }

    //                        --- Win Condition (Reached Exit Portal) ---
    if (fabsf(player.x - exitPortal.x) < 1.0f && fabsf(player.y - exitPortal.y) < 1.0f)
    {
        // Sharon 2/3: commented out these as its shifted to initialise
        //// Reset Player and Mummy
        //player.x = 225.0f;
        //player.y = -125.0f;
        //mummy.x = 325.0f;
        //mummy.y = 175.0f;
        //turnCounter = 0;
        //// --- RESET COIN HERE ---
        //coin.x = 25.0f; // Position it somewhere in the middle
        //coin.y = 75.0f;
        //coinCounter = 0; // Reset score to 0 for the new attempt
        printf("You Escaped the Maze!\n");
        level1_counter = 0; // Trigger level completion
        gShowWin = true;    /* NEW: show a small win overlay (flow remains the same) -ths */
    }
    //                                --- Coin Collection Condition ---
    // We check if player is at the same grid position as the coin
    if (fabsf(player.x - coin.x) < 1.0f && fabsf(player.y - coin.y) < 1.0f)
    {
        coinCounter++;
        printf("Coin Collected! Total Coins: %d\n", coinCounter);
        // Move the coin off-screen or to a new spot so it doesn't trigger again immediately
        coin.x = 2000.0f;
        coin.y = 2000.0f;
    }
    //                         --- update logic end here ---
}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame
// --------------------------------------------------------------------------- (original)
void Level1_Draw()
{
    //std::cout << "Level1:Draw\n"; // Print onto standard output stream yt 25-2 comment up first, my computer cannot stand D:
    // Sharon 2/3: Creation of mesh AND player, wall, enemy positions is done in Load, not draw
    //                                --- rendering logic goes here ---
        // Set the background to white.
    AEGfxSetBackgroundColor(255.0f, 255.0f, 255.0f);
    //AEGfxSetRenderMode(AE_GFX_RM_COLOR); // Using colors, not textures
    AEMtx33 transform, scale, trans;
    //                                Render Player with Texture
    // Use Texture mode for the player
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    // Set up blending for transparency (this is the key fix!)
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Use white so the texture colors show correctly
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f); // Don't add any color
    AEGfxTextureSet(player.pTex, 0, 0); // Bind the player texture
    AEMtx33Scale(&scale, player.size, player.size);
    AEMtx33Trans(&trans, player.x, player.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR); // Switch back to color mode for other entities
    AEGfxSetBlendMode(AE_GFX_BM_NONE); // Disable blending for solid colors
    // Render Mummy (Red Square)
    AEGfxSetColorToMultiply(mummy.r, mummy.g, mummy.b, 1.0f);
    AEMtx33Scale(&scale, mummy.size, mummy.size);
    AEMtx33Trans(&trans, mummy.x, mummy.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    //                                Render exit portal (Yellow Square)
    AEGfxSetColorToMultiply(exitPortal.r, exitPortal.g, exitPortal.b, 1.0f);
    AEMtx33Scale(&scale, exitPortal.size, exitPortal.size);
    AEMtx33Trans(&trans, exitPortal.x, exitPortal.y);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxPrint(fontId, "EXIT", 0.48f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    //                                Render Coin (Orange Square)
    //if (coin.x < 1000.0f) // Only render if the coin hasn't been "eaten"
    //{
    //    AEGfxSetColorToMultiply(coin.r, coin.g, coin.b, 1.0f);
    //    AEMtx33Scale(&scale, coin.size, coin.size);
    //    AEMtx33Trans(&trans, coin.x, coin.y);
    //    AEMtx33Concat(&transform, &trans, &scale);
    //    AEGfxSetTransform(transform.m);
    //    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    //    // The text is now tied to the coin's active state
    //    AEGfxPrint(fontId, "COIN", -0.05f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    //}
    ////                                Render Wall (Dark Grey Square)         -- comment this out cause there is already walll in leveleditor -- YT
    //AEGfxSetColorToMultiply(wall.r, wall.g, wall.b, 1.0f);
    //AEMtx33Scale(&scale, wall.size, wall.size);
    //AEMtx33Trans(&trans, wall.x, wall.y);
    //AEMtx33Concat(&transform, &trans, &scale);
    //AEGfxSetTransform(transform.m);
    //AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    /* NEW: draw extra enemies (colored squares) -ths */
    for (int i = 0; i < gExtraEnemyCount; ++i) {
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

    /* NEW: tiny HUD for active powers -ths */
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    if (gPower.speed)      AEGfxPrint(fontId, "SPEED", -0.98f, 0.90f, 0.8f, 1, 1, 0, 1);
    if (gPower.freeze)     AEGfxPrint(fontId, "FREEZE", -0.98f, 0.84f, 0.8f, 0, 1, 1, 1);
    if (gPower.invincible) AEGfxPrint(fontId, "INVINCIBLE", -0.98f, 0.78f, 0.8f, 1, 0, 1, 1);

    // This generates the level for each level. need an additional source file for it -- uncommmented this file to collaborate with level editor.
    generateLevel();

    /* NEW: pause / win / lose overlays (simple text, no GSM changes) -ths */
    if (gPaused) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.6f);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.6f);
        AEMtx33 fullS, fullT, fullM;
        AEMtx33Scale(&fullS, (float)AEGfxGetWindowWidth(), (float)AEGfxGetWindowHeight());
        AEMtx33Trans(&fullT, 0.0f, 0.0f);
        AEMtx33Concat(&fullM, &fullT, &fullS);
        AEGfxSetTransform(fullM.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxPrint(fontId, "PAUSED (P to Resume)", -0.22f, 0.02f, 1.4f, 1, 1, 1, 1);
    }
    if (gShowLose) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.5f);
        AEGfxSetColorToMultiply(0.2f, 0.0f, 0.0f, 0.5f);
        AEMtx33 fullS, fullT, fullM;
        AEMtx33Scale(&fullS, (float)AEGfxGetWindowWidth(), (float)AEGfxGetWindowHeight());
        AEMtx33Trans(&fullT, 0.0f, 0.0f);
        AEMtx33Concat(&fullM, &fullT, &fullS);
        AEGfxSetTransform(fullM.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxPrint(fontId, "CAUGHT! (R to Restart from Menu)", -0.35f, 0.02f, 1.2f, 1, 1, 1, 1);
    }
    if (gShowWin) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.5f);
        AEGfxSetColorToMultiply(0.0f, 0.2f, 0.0f, 0.5f);
        AEMtx33 fullS, fullT, fullM;
        AEMtx33Scale(&fullS, (float)AEGfxGetWindowWidth(), (float)AEGfxGetWindowHeight());
        AEMtx33Trans(&fullT, 0.0f, 0.0f);
        AEMtx33Concat(&fullM, &fullT, &fullS);
        AEGfxSetTransform(fullM.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxPrint(fontId, "YOU ESCAPED!", -0.18f, 0.02f, 1.4f, 1, 1, 1, 1);
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
    player.x = 225.0f;
    player.y = -125.0f;
    mummy.x = 325.0f;
    mummy.y = 175.0f;
    coin.x = 25.0f;
    coin.y = 75.0f;
    nextX = player.x;
    nextY = player.y;
    coinCounter = 0;
    turnCounter = 0;
    playerMoved = false;

    /* NEW: reset powers on reset for consistency -ths */
    gPower = {};
}