// gs_play.cpp - Mummy Maze play state (Alpha Engine version)
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "AEEngine.h"
#include "grid.h"
#include "actors.h"
#include "collectibles.h"
#include "gs_play.h"

// -----------------------------------------------------------------------------
// Game state
// -----------------------------------------------------------------------------
static Grid gGrid;
static Actors gActors;
static PlayerState gPlayer;
static Collectibles gCollectibles;
static bool gWon = false;
static bool gLost = false;

// -----------------------------------------------------------------------------
// Render helpers
// -----------------------------------------------------------------------------
static inline u32 ARGB(u8 a, u8 r, u8 g, u8 b) {
    return (static_cast<u32>(a) << 24) |
        (static_cast<u32>(r) << 16) |
        (static_cast<u32>(g) << 8) |
        static_cast<u32>(b);
}

static const u32 COL_FLOOR = ARGB(0xFF, 0x22, 0x22, 0x22);
static const u32 COL_WALL = ARGB(0xFF, 0x88, 0x88, 0x88);
static const u32 COL_EXIT = ARGB(0xFF, 0x00, 0xCC, 0x00);
static const u32 COL_GATE_C = ARGB(0xFF, 0xCC, 0xAA, 0x00);
static const u32 COL_GATE_O = ARGB(0xFF, 0x00, 0x99, 0x00);
static const u32 COL_KEY = ARGB(0xFF, 0xFF, 0xE6, 0x33);
static const u32 COL_PLAYER = ARGB(0xFF, 0xFF, 0x00, 0x00);
static const u32 COL_MW = ARGB(0xFF, 0xF2, 0xF2, 0xF2);
static const u32 COL_MR = ARGB(0xFF, 0xFF, 0x66, 0x00);
static const u32 COL_SCORP = ARGB(0xFF, 0x88, 0x00, 0x88);
static const u32 COL_GEM = ARGB(0xFF, 0x00, 0xFF, 0xFF);
static const u32 COL_ANKH = ARGB(0xFF, 0xFF, 0xFF, 0x00);
static const u32 COL_SANDGLASS = ARGB(0xFF, 0x8B, 0x45, 0x13);

struct DrawSpace {
    float minX, maxX, minY, maxY;
    float cellW, cellH;
};

static DrawSpace MakeDrawSpace() {
    DrawSpace ds;
    ds.minX = AEGfxGetWinMinX();
    ds.maxX = AEGfxGetWinMaxX();
    ds.minY = AEGfxGetWinMinY();
    ds.maxY = AEGfxGetWinMaxY();

    if (gGrid.width > 0 && gGrid.height > 0) {
        ds.cellW = (ds.maxX - ds.minX) / static_cast<float>(gGrid.width);
        ds.cellH = (ds.maxY - ds.minY) / static_cast<float>(gGrid.height);
    }
    else {
        ds.cellW = 1.0f;
        ds.cellH = 1.0f;
    }

    return ds;
}

static void CellCenterF(const DrawSpace& ds, s32 x, s32 y, float& outX, float& outY) {
    outX = ds.minX + (static_cast<float>(x) + 0.5f) * ds.cellW;
    outY = ds.maxY - (static_cast<float>(y) + 0.5f) * ds.cellH;
}

// Static meshes
static AEGfxVertexList* M_FloorGrid = nullptr;
static AEGfxVertexList* M_Walls = nullptr;
static AEGfxVertexList* M_Exits = nullptr;
static AEGfxVertexList* M_Keys = nullptr;
static AEGfxVertexList* M_GateC = nullptr;
static AEGfxVertexList* M_GateO = nullptr;
static AEGfxVertexList* M_Player = nullptr;
static AEGfxVertexList* M_MW = nullptr;
static AEGfxVertexList* M_MR = nullptr;
static AEGfxVertexList* M_SC = nullptr;
static AEGfxVertexList* M_Gem = nullptr;
static AEGfxVertexList* M_Ankh = nullptr;
static AEGfxVertexList* M_Sandglass = nullptr;

static float gCellHalfW = -1.0f;
static float gCellHalfH = -1.0f;

static void AddWorldQuad(float cx, float cy, float hw, float hh, u32 color) {
    AEGfxTriAdd(cx - hw, cy - hh, color, 0.0f, 0.0f,
        cx + hw, cy - hh, color, 0.0f, 0.0f,
        cx - hw, cy + hh, color, 0.0f, 0.0f);
    AEGfxTriAdd(cx + hw, cy - hh, color, 0.0f, 0.0f,
        cx + hw, cy + hh, color, 0.0f, 0.0f,
        cx - hw, cy + hh, color, 0.0f, 0.0f);
}

static AEGfxVertexList* BuildCenteredQuad(float halfW, float halfH, u32 color) {
    AEGfxMeshStart();
    AEGfxTriAdd(-halfW, -halfH, color, 0.0f, 0.0f,
        halfW, -halfH, color, 0.0f, 0.0f,
        -halfW, halfH, color, 0.0f, 0.0f);
    AEGfxTriAdd(halfW, -halfH, color, 0.0f, 0.0f,
        halfW, halfH, color, 0.0f, 0.0f,
        -halfW, halfH, color, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

static void FreeAllMeshes() {
    auto FreeMesh = [](AEGfxVertexList*& mesh) {
        if (mesh) {
            AEGfxMeshFree(mesh);
            mesh = nullptr;
        }
        };

    FreeMesh(M_FloorGrid);
    FreeMesh(M_Walls);
    FreeMesh(M_Exits);
    FreeMesh(M_Keys);
    FreeMesh(M_GateC);
    FreeMesh(M_GateO);
    FreeMesh(M_Player);
    FreeMesh(M_MW);
    FreeMesh(M_MR);
    FreeMesh(M_SC);
    FreeMesh(M_Gem);
    FreeMesh(M_Ankh);
    FreeMesh(M_Sandglass);

    gCellHalfW = gCellHalfH = -1.0f;
}

static void BuildStaticGeometry(const DrawSpace& ds) {
    const float hw = 0.45f * ds.cellW;
    const float hh = 0.45f * ds.cellH;

    if (gCellHalfW == hw && gCellHalfH == hh && M_FloorGrid) {
        return;
    }

    FreeAllMeshes();
    gCellHalfW = hw;
    gCellHalfH = hh;

    // Create floor grid mesh
    AEGfxMeshStart();
    for (s32 y = 0; y < gGrid.height; ++y) {
        for (s32 x = 0; x < gGrid.width; ++x) {
            float cx, cy;
            CellCenterF(ds, x, y, cx, cy);
            AddWorldQuad(cx, cy, hw, hh, COL_FLOOR);
        }
    }
    M_FloorGrid = AEGfxMeshEnd();

    // Create walls mesh
    AEGfxMeshStart();
    for (s32 y = 0; y < gGrid.height; ++y) {
        for (s32 x = 0; x < gGrid.width; ++x) {
            if (gGrid.tiles[Grid_Idx(&gGrid, x, y)] == TILE_WALL) {
                float cx, cy;
                CellCenterF(ds, x, y, cx, cy);
                AddWorldQuad(cx, cy, hw, hh, COL_WALL);
            }
        }
    }
    M_Walls = AEGfxMeshEnd();

    // Create exits mesh
    AEGfxMeshStart();
    for (s32 y = 0; y < gGrid.height; ++y) {
        for (s32 x = 0; x < gGrid.width; ++x) {
            if (gGrid.tiles[Grid_Idx(&gGrid, x, y)] == TILE_EXIT) {
                float cx, cy;
                CellCenterF(ds, x, y, cx, cy);
                AddWorldQuad(cx, cy, hw, hh, COL_EXIT);
            }
        }
    }
    M_Exits = AEGfxMeshEnd();

    // Create keys mesh
    AEGfxMeshStart();
    for (s32 y = 0; y < gGrid.height; ++y) {
        for (s32 x = 0; x < gGrid.width; ++x) {
            if (gGrid.tiles[Grid_Idx(&gGrid, x, y)] == TILE_KEY) {
                float cx, cy;
                CellCenterF(ds, x, y, cx, cy);
                AddWorldQuad(cx, cy, hw, hh, COL_KEY);
            }
        }
    }
    M_Keys = AEGfxMeshEnd();

    // Find and create gate meshes
    bool gateFound = false;
    for (s32 y = 0; y < gGrid.height && !gateFound; ++y) {
        for (s32 x = 0; x < gGrid.width && !gateFound; ++x) {
            if (gGrid.tiles[Grid_Idx(&gGrid, x, y)] == TILE_GATE) {
                float cx, cy;
                CellCenterF(ds, x, y, cx, cy);

                // Closed gate
                AEGfxMeshStart();
                AddWorldQuad(cx, cy, hw, hh, COL_GATE_C);
                M_GateC = AEGfxMeshEnd();

                // Open gate
                AEGfxMeshStart();
                AddWorldQuad(cx, cy, hw, hh, COL_GATE_O);
                M_GateO = AEGfxMeshEnd();

                gateFound = true;
            }
        }
    }

    if (!gateFound) {
        AEGfxMeshStart();
        AddWorldQuad(0.0f, 0.0f, hw, hh, COL_GATE_C);
        M_GateC = AEGfxMeshEnd();

        AEGfxMeshStart();
        AddWorldQuad(0.0f, 0.0f, hw, hh, COL_GATE_O);
        M_GateO = AEGfxMeshEnd();
    }

    // Create actor meshes
    M_Player = BuildCenteredQuad(hw, hh, COL_PLAYER);
    M_MW = BuildCenteredQuad(hw, hh, COL_MW);
    M_MR = BuildCenteredQuad(hw, hh, COL_MR);
    M_SC = BuildCenteredQuad(hw, hh, COL_SCORP);

    // Create collectible meshes
    M_Gem = BuildCenteredQuad(hw * 0.6f, hh * 0.6f, COL_GEM);
    M_Ankh = BuildCenteredQuad(hw * 0.6f, hh * 0.6f, COL_ANKH);
    M_Sandglass = BuildCenteredQuad(hw * 0.6f, hh * 0.6f, COL_SANDGLASS);
}

static void DrawMesh(AEGfxVertexList* mesh) {
    if (!mesh) {
        return;
    }

    static float identity[3][3] = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };

    AEGfxSetTransform(identity);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

static void DrawActorAt(AEGfxVertexList* mesh, float cx, float cy) {
    if (!mesh) {
        return;
    }

    AEMtx33 transform;
    AEMtx33Trans(&transform, cx, cy);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// -----------------------------------------------------------------------------
// FUNCTIONAL MAZE - Enemy can move but starts far away
// -----------------------------------------------------------------------------
static void BuildFunctionalMaze() {
    const s32 W = 11;
    const s32 H = 9;

    // Free existing grid
    if (gGrid.tiles) {
        free(gGrid.tiles);
        gGrid.tiles = nullptr;
    }

    // Initialize grid
    gGrid.width = W;
    gGrid.height = H;
    gGrid.gateOpen = false;

    // Allocate tiles
    gGrid.tiles = static_cast<Tile*>(malloc(sizeof(Tile) * static_cast<size_t>(W * H)));
    if (!gGrid.tiles) {
        return;
    }

    // Initialize all to walls
    for (s32 i = 0; i < W * H; ++i) {
        gGrid.tiles[i] = TILE_WALL;
    }

    // ============================================
    // FUNCTIONAL MAZE - Enemy can move but starts far
    // ============================================

    // Player's main path (LEFT side)
    gGrid.tiles[Grid_Idx(&gGrid, 1, 4)] = TILE_FLOOR;  // Player start
    gGrid.tiles[Grid_Idx(&gGrid, 2, 4)] = TILE_KEY;    // Key
    gGrid.tiles[Grid_Idx(&gGrid, 3, 4)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 4, 4)] = TILE_GATE;   // Gate
    gGrid.tiles[Grid_Idx(&gGrid, 5, 4)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 6, 4)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 7, 4)] = TILE_EXIT;   // Exit

    // Enemy area (RIGHT side - far from player)
    gGrid.tiles[Grid_Idx(&gGrid, 9, 2)] = TILE_FLOOR;  // Enemy start
    gGrid.tiles[Grid_Idx(&gGrid, 9, 3)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 9, 4)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 9, 5)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 9, 6)] = TILE_FLOOR;

    // Connection between enemy area and main path (LONG path)
    gGrid.tiles[Grid_Idx(&gGrid, 8, 6)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 7, 6)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 6, 6)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 5, 6)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 5, 5)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 5, 4)] = TILE_FLOOR;  // Connects to main path

    // Optional collectibles
    gGrid.tiles[Grid_Idx(&gGrid, 2, 2)] = TILE_FLOOR;  // Gem
    gGrid.tiles[Grid_Idx(&gGrid, 3, 2)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 2, 3)] = TILE_FLOOR;

    gGrid.tiles[Grid_Idx(&gGrid, 4, 1)] = TILE_FLOOR;  // Ankh

    gGrid.tiles[Grid_Idx(&gGrid, 7, 1)] = TILE_FLOOR;  // Sandglass
}

static void FreeGrid() {
    if (gGrid.tiles) {
        free(gGrid.tiles);
        gGrid.tiles = nullptr;
    }
    gGrid.width = 0;
    gGrid.height = 0;
    gGrid.gateOpen = false;
}

// -----------------------------------------------------------------------------
// Game logic
// -----------------------------------------------------------------------------
static void PlayerTryMoveOrWait(s32 dx, s32 dy, bool waitOnly) {
    Actor* playerActor = &gActors.data[gPlayer.index];

    if (!waitOnly && (dx != 0 || dy != 0)) {
        s32 newX = playerActor->x + dx;
        s32 newY = playerActor->y + dy;

        if (Grid_In(&gGrid, newX, newY) && !Grid_Blocking(&gGrid, newX, newY)) {
            playerActor->x = newX;
            playerActor->y = newY;

            // Check for key collection
            if (gGrid.tiles[Grid_Idx(&gGrid, newX, newY)] == TILE_KEY) {
                Grid_ToggleGate(&gGrid, newX, newY);
                gGrid.tiles[Grid_Idx(&gGrid, newX, newY)] = TILE_FLOOR;
            }

            // Check for collectibles
            s32 collectibleIdx = Col_FindAt(&gCollectibles, newX, newY);
            if (collectibleIdx >= 0) {
                Col_ApplyAndConsume(&gCollectibles, collectibleIdx, &gPlayer);
            }

            // Check for exit
            if (gGrid.tiles[Grid_Idx(&gGrid, newX, newY)] == TILE_EXIT) {
                gWon = true;
                return;
            }
        }
    }

    // Only process enemy turn if game is not won
    if (!gWon) {
        bool playerCaught = false;
        ResolveEnemyTurns(&gGrid, &gActors, &gPlayer, &playerCaught);

        if (playerCaught) {
            gLost = true;
        }
    }
}

// -----------------------------------------------------------------------------
// State API
// -----------------------------------------------------------------------------
void GS_PlayLoad(void) {}
void GS_PlayUnload(void) {}

void GS_PlayInit(void) {
    srand(static_cast<unsigned int>(time(nullptr)));

    // Build the functional maze
    BuildFunctionalMaze();

    // Initialize systems
    Actors_Init(&gActors);
    Col_Init(&gCollectibles);

    // Add player at start position (1,4)
    s32 playerIdx = Actors_Add(&gActors, AT_PLAYER, 1, 4);
    if (playerIdx < 0) {
        return;
    }

    gPlayer.index = playerIdx;
    gPlayer.buffs.immunitySteps = 0;
    gPlayer.buffs.freezeEnemyTurns = 0;

    // Add enemy FAR AWAY at (9,2) - can move but needs many turns to reach player
    Actors_Add(&gActors, AT_MUMMY_WHITE, 9, 2);

    // Add collectibles (optional)
    Col_Add(&gCollectibles, CT_GEM, 2, 2);
    Col_Add(&gCollectibles, CT_ANKH, 4, 1);
    Col_Add(&gCollectibles, CT_SANDGLASS, 7, 1);

    // Reset win/lose flags
    gWon = false;
    gLost = false;

    // Build initial graphics geometry
    const DrawSpace ds = MakeDrawSpace();
    BuildStaticGeometry(ds);
}

static bool RepeatTick(bool pressed, bool held, double now,
    double& firstTime, double& lastTime,
    double firstDelay, double repeatPeriod) {
    if (pressed) {
        firstTime = lastTime = now;
        return true;
    }

    if (!held) {
        firstTime = lastTime = -1.0;
        return false;
    }

    if (firstTime < 0.0) {
        firstTime = lastTime = now;
        return true;
    }

    if ((now - firstTime) >= firstDelay && (now - lastTime) >= repeatPeriod) {
        lastTime = now;
        return true;
    }

    return false;
}

void GS_PlayUpdate(void) {
    static double leftFirst = -1.0, leftLast = -1.0;
    static double rightFirst = -1.0, rightLast = -1.0;
    static double upFirst = -1.0, upLast = -1.0;
    static double downFirst = -1.0, downLast = -1.0;

    const double firstDelay = 0.25;
    const double repeatPeriod = 0.10;
    const double now = AEGetTime(nullptr);

    // Handle win/lose states
    if (gWon) {
        if (AEInputCheckTriggered(AEVK_R)) {
            GS_PlayFree();
            GS_PlayInit();
        }
        return;
    }

    if (gLost) {
        if (AEInputCheckTriggered(AEVK_R)) {
            GS_PlayFree();
            GS_PlayInit();
        }
        return;
    }

    // Handle restart
    if (AEInputCheckTriggered(AEVK_R)) {
        GS_PlayFree();
        GS_PlayInit();
        return;
    }

    // Get input states
    bool leftPressed = AEInputCheckTriggered(AEVK_LEFT);
    bool rightPressed = AEInputCheckTriggered(AEVK_RIGHT);
    bool upPressed = AEInputCheckTriggered(AEVK_UP);
    bool downPressed = AEInputCheckTriggered(AEVK_DOWN);

    bool leftHeld = AEInputCheckCurr(AEVK_LEFT);
    bool rightHeld = AEInputCheckCurr(AEVK_RIGHT);
    bool upHeld = AEInputCheckCurr(AEVK_UP);
    bool downHeld = AEInputCheckCurr(AEVK_DOWN);

    bool waitPressed = AEInputCheckTriggered(AEVK_SPACE);

    // Determine movement direction
    s32 dx = 0, dy = 0;
    bool acted = false;

    if (RepeatTick(leftPressed, leftHeld, now, leftFirst, leftLast, firstDelay, repeatPeriod)) {
        dx = -1;
        acted = true;
    }
    else if (RepeatTick(rightPressed, rightHeld, now, rightFirst, rightLast, firstDelay, repeatPeriod)) {
        dx = 1;
        acted = true;
    }
    else if (RepeatTick(upPressed, upHeld, now, upFirst, upLast, firstDelay, repeatPeriod)) {
        dy = -1;
        acted = true;
    }
    else if (RepeatTick(downPressed, downHeld, now, downFirst, downLast, firstDelay, repeatPeriod)) {
        dy = 1;
        acted = true;
    }

    // Execute player action
    if (acted || waitPressed) {
        PlayerTryMoveOrWait(dx, dy, waitPressed && !acted);
    }
}

void GS_PlayDraw(void) {
    AEGfxSetBackgroundColor(0.08f, 0.08f, 0.12f);
    AEGfxStart();
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);

    const DrawSpace ds = MakeDrawSpace();
    BuildStaticGeometry(ds);

    // Draw the game board
    DrawMesh(M_FloorGrid);
    DrawMesh(M_Walls);
    DrawMesh(M_Exits);
    DrawMesh(M_Keys);

    // Draw gate (open or closed)
    if (M_GateC && M_GateO) {
        DrawMesh(gGrid.gateOpen ? M_GateO : M_GateC);
    }

    // Draw collectibles
    for (s32 i = 0; i < gCollectibles.count; ++i) {
        const Collectible& col = gCollectibles.data[i];
        if (!col.active) {
            continue;
        }

        float cx, cy;
        CellCenterF(ds, col.x, col.y, cx, cy);

        AEGfxVertexList* mesh = nullptr;
        switch (col.type) {
        case CT_GEM: mesh = M_Gem; break;
        case CT_ANKH: mesh = M_Ankh; break;
        case CT_SANDGLASS: mesh = M_Sandglass; break;
        default: break;
        }

        if (mesh) {
            DrawActorAt(mesh, cx, cy);
        }
    }

    // Draw actors
    for (s32 i = 0; i < gActors.count; ++i) {
        const Actor& actor = gActors.data[i];
        if (!actor.alive) {
            continue;
        }

        float cx, cy;
        CellCenterF(ds, actor.x, actor.y, cx, cy);

        AEGfxVertexList* mesh = nullptr;
        if (i == gPlayer.index) {
            mesh = M_Player;
        }
        else if (actor.type == AT_MUMMY_WHITE) {
            mesh = M_MW;
        }
        else if (actor.type == AT_MUMMY_RED) {
            mesh = M_MR;
        }
        else if (actor.type == AT_SCORPION) {
            mesh = M_SC;
        }

        if (mesh) {
            DrawActorAt(mesh, cx, cy);
        }
    }

    AEGfxEnd();
}

void GS_PlayFree(void) {
    FreeAllMeshes();
    Col_Free(&gCollectibles);
    Actors_Free(&gActors);
    FreeGrid();

    gWon = false;
    gLost = false;
}