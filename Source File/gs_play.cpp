// gs_play.cpp - Mummy Maze play state (Alpha Engine version, C++14)
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdio>
#include "AEEngine.h"
#include "AETypes.h"
#include "grid.h"
#include "actors.h"
#include "collectibles.h"
#include "gs_play.h"

// -----------------------------------------------------------------------------
// Game state
// -----------------------------------------------------------------------------
static Grid         gGrid;
static Actors       gActors;
static PlayerState  gPlayer;
static Collectibles gCollectibles;
static bool gWon = false;
static bool gLost = false;
static bool gPaused = false;
static const s32 kDefaultHealth = 3;

// -----------------------------------------------------------------------------
// Render helpers
// -----------------------------------------------------------------------------
static inline u32 ARGB(u8 a, u8 r, u8 g, u8 b) {
    return (static_cast<u32>(a) << 24)
        | (static_cast<u32>(r) << 16)
        | (static_cast<u32>(g) << 8)
        | static_cast<u32>(b);
}
static const u32 COL_FLOOR = ARGB(0xFF, 0x22, 0x22, 0x22);
static const u32 COL_WALL = ARGB(0xFF, 0x88, 0x88, 0x88);
static const u32 COL_EXIT = ARGB(0xFF, 0x00, 0xCC, 0x00);
static const u32 COL_GATE_C = ARGB(0xFF, 0xCC, 0xAA, 0x00);
static const u32 COL_GATE_O = ARGB(0xFF, 0x00, 0x99, 0x00);
static const u32 COL_KEY = ARGB(0xFF, 0xFF, 0xE6, 0x33);
static const u32 COL_SAVE = ARGB(0xFF, 0x33, 0x66, 0xCC);

static const u32 COL_PLAYER = ARGB(0xFF, 0xFF, 0x00, 0x00);
static const u32 COL_MW = ARGB(0xFF, 0xF2, 0xF2, 0xF2);
static const u32 COL_MR = ARGB(0xFF, 0xFF, 0x66, 0x00);
static const u32 COL_SCORP = ARGB(0xFF, 0x88, 0x00, 0x88);

static const u32 COL_GEM = ARGB(0xFF, 0x00, 0xFF, 0xFF);
static const u32 COL_ANKH = ARGB(0xFF, 0xFF, 0xFF, 0x00);
static const u32 COL_SANDGLASS = ARGB(0xFF, 0x8B, 0x45, 0x13);
static const u32 COL_HEART = ARGB(0xFF, 0xFF, 0x22, 0x22);

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
        ds.cellW = 1.0f; ds.cellH = 1.0f;
    }
    return ds;
}
static void CellCenterF(const DrawSpace& ds, s32 x, s32 y, float& outX, float& outY) {
    outX = ds.minX + (static_cast<float>(x) + 0.5f) * ds.cellW;
    outY = ds.maxY - (static_cast<float>(y) + 0.5f) * ds.cellH;
}

// -----------------------------------------------------------------------------
// Font (Roboto)
// -----------------------------------------------------------------------------
static s8 gFontId = -1;

// Adjust search paths to where you store the font
static const char* kFontSearchPaths[] = {
  "./Roboto-Regular.ttf",               // next to .exe
  "assets/Roboto-Regular.ttf",
  "assets/fonts/Roboto-Regular.ttf"
};

static s8 TryLoadFont(const char* path, int px) {
    // Returns -1 when unsuccessful (per Alpha Engine header)
    return AEGfxCreateFont(path, px);
}

static void EnsureGameFontLoaded() {
    if (gFontId != -1) return;

    // Try Roboto first
    for (const char* p : kFontSearchPaths) {
        gFontId = TryLoadFont(p, 32); // generous size for readability
        if (gFontId != -1) break;
    }

    // Fallbacks (Windows system fonts) in case Roboto isn't found
    if (gFontId == -1) {
        const char* sysFonts[] = {
          "C:\\\\Windows\\\\Fonts\\\\arial.ttf",
          "C:\\\\Windows\\\\Fonts\\\\segoeui.ttf",
          "C:\\\\Windows\\\\Fonts\\\\calibri.ttf"
        };
        for (const char* p : sysFonts) {
            gFontId = TryLoadFont(p, 32);
            if (gFontId != -1) break;
        }
    }
}

// -----------------------------------------------------------------------------
// Static meshes
// -----------------------------------------------------------------------------
static AEGfxVertexList* M_FloorGrid = nullptr;
static AEGfxVertexList* M_Walls = nullptr;
static AEGfxVertexList* M_Exits = nullptr;
static AEGfxVertexList* M_Keys = nullptr;
static AEGfxVertexList* M_GateC = nullptr;
static AEGfxVertexList* M_GateO = nullptr;
static AEGfxVertexList* M_Save = nullptr;

static AEGfxVertexList* M_Player = nullptr;
static AEGfxVertexList* M_MW = nullptr;
static AEGfxVertexList* M_MR = nullptr;
static AEGfxVertexList* M_SC = nullptr;

static AEGfxVertexList* M_Gem = nullptr;
static AEGfxVertexList* M_Ankh = nullptr;
static AEGfxVertexList* M_Sandglass = nullptr;
static AEGfxVertexList* M_Heart = nullptr;

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
        if (mesh) { AEGfxMeshFree(mesh); mesh = nullptr; }
        };
    FreeMesh(M_FloorGrid);
    FreeMesh(M_Walls);
    FreeMesh(M_Exits);
    FreeMesh(M_Keys);
    FreeMesh(M_GateC);
    FreeMesh(M_GateO);
    FreeMesh(M_Save);

    FreeMesh(M_Player);
    FreeMesh(M_MW);
    FreeMesh(M_MR);
    FreeMesh(M_SC);

    FreeMesh(M_Gem);
    FreeMesh(M_Ankh);
    FreeMesh(M_Sandglass);
    FreeMesh(M_Heart);

    gCellHalfW = gCellHalfH = -1.0f;
}
static void BuildStaticGeometry(const DrawSpace& ds) {
    const float hw = 0.45f * ds.cellW;
    const float hh = 0.45f * ds.cellH;
    if (gCellHalfW == hw && gCellHalfH == hh && M_FloorGrid) {
        return;
    }
    FreeAllMeshes();
    gCellHalfW = hw; gCellHalfH = hh;

    // Floor grid
    AEGfxMeshStart();
    for (s32 y = 0; y < gGrid.height; ++y)
        for (s32 x = 0; x < gGrid.width; ++x) {
            float cx = 0.0f, cy = 0.0f; CellCenterF(ds, x, y, cx, cy);
            AddWorldQuad(cx, cy, hw, hh, COL_FLOOR);
        }
    M_FloorGrid = AEGfxMeshEnd();

    // Walls
    AEGfxMeshStart();
    for (s32 y = 0; y < gGrid.height; ++y)
        for (s32 x = 0; x < gGrid.width; ++x)
            if (gGrid.tiles[Grid_Idx(&gGrid, x, y)] == TILE_WALL) {
                float cx = 0.0f, cy = 0.0f; CellCenterF(ds, x, y, cx, cy);
                AddWorldQuad(cx, cy, hw, hh, COL_WALL);
            }
    M_Walls = AEGfxMeshEnd();

    // Exits
    AEGfxMeshStart();
    for (s32 y = 0; y < gGrid.height; ++y)
        for (s32 x = 0; x < gGrid.width; ++x)
            if (gGrid.tiles[Grid_Idx(&gGrid, x, y)] == TILE_EXIT) {
                float cx = 0.0f, cy = 0.0f; CellCenterF(ds, x, y, cx, cy);
                AddWorldQuad(cx, cy, hw, hh, COL_EXIT);
            }
    M_Exits = AEGfxMeshEnd();

    // Keys
    AEGfxMeshStart();
    for (s32 y = 0; y < gGrid.height; ++y)
        for (s32 x = 0; x < gGrid.width; ++x)
            if (gGrid.tiles[Grid_Idx(&gGrid, x, y)] == TILE_KEY) {
                float cx = 0.0f, cy = 0.0f; CellCenterF(ds, x, y, cx, cy);
                AddWorldQuad(cx, cy, hw, hh, COL_KEY);
            }
    M_Keys = AEGfxMeshEnd();

    // SAVE tiles
    AEGfxMeshStart();
    bool anySave = false;
    for (s32 y = 0; y < gGrid.height; ++y)
        for (s32 x = 0; x < gGrid.width; ++x)
            if (gGrid.tiles[Grid_Idx(&gGrid, x, y)] == TILE_SAVE) {
                anySave = true;
                float cx = 0.0f, cy = 0.0f; CellCenterF(ds, x, y, cx, cy);
                AddWorldQuad(cx, cy, hw, hh, COL_SAVE);
            }
    M_Save = AEGfxMeshEnd();
    if (!anySave) {
        AEGfxMeshStart();
        AddWorldQuad(0.0f, 0.0f, hw, hh, COL_SAVE);
        AEGfxVertexList* m = AEGfxMeshEnd();
        AEGfxMeshFree(m);
    }

    // Gate (open/closed meshes in place of the tile)
    bool gateFound = false;
    for (s32 y = 0; y < gGrid.height && !gateFound; ++y)
        for (s32 x = 0; x < gGrid.width && !gateFound; ++x)
            if (gGrid.tiles[Grid_Idx(&gGrid, x, y)] == TILE_GATE) {
                float cx = 0.0f, cy = 0.0f; CellCenterF(ds, x, y, cx, cy);
                AEGfxMeshStart(); AddWorldQuad(cx, cy, hw, hh, COL_GATE_C); M_GateC = AEGfxMeshEnd();
                AEGfxMeshStart(); AddWorldQuad(cx, cy, hw, hh, COL_GATE_O); M_GateO = AEGfxMeshEnd();
                gateFound = true;
            }
    if (!gateFound) {
        AEGfxMeshStart(); AddWorldQuad(0.0f, 0.0f, hw, hh, COL_GATE_C); M_GateC = AEGfxMeshEnd();
        AEGfxMeshStart(); AddWorldQuad(0.0f, 0.0f, hw, hh, COL_GATE_O); M_GateO = AEGfxMeshEnd();
    }

    // Actor meshes
    M_Player = BuildCenteredQuad(hw, hh, COL_PLAYER);
    M_MW = BuildCenteredQuad(hw, hh, COL_MW);
    M_MR = BuildCenteredQuad(hw, hh, COL_MR);
    M_SC = BuildCenteredQuad(hw, hh, COL_SCORP);

    // Collectible meshes
    M_Gem = BuildCenteredQuad(hw * 0.6f, hh * 0.6f, COL_GEM);
    M_Ankh = BuildCenteredQuad(hw * 0.6f, hh * 0.6f, COL_ANKH);
    M_Sandglass = BuildCenteredQuad(hw * 0.6f, hh * 0.6f, COL_SANDGLASS);
    M_Heart = BuildCenteredQuad(hw * 0.6f, hh * 0.6f, COL_HEART);
}

static void DrawMesh(AEGfxVertexList* mesh) {
    if (!mesh) return;
    static float identity[3][3] = {
      {1.0f, 0.0f, 0.0f},
      {0.0f, 1.0f, 0.0f},
      {0.0f, 0.0f, 1.0f}
    };
    AEGfxSetTransform(identity);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}
static void DrawActorAt(AEGfxVertexList* mesh, float cx, float cy) {
    if (!mesh) return;
    AEMtx33 transform{}; // value-initialize to silence analyzer
    AEMtx33Trans(&transform, cx, cy);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// -----------------------------------------------------------------------------
// Level setup: functional maze with longer enemy path and a save tile
// -----------------------------------------------------------------------------
static void BuildFunctionalMaze() {
    const s32 W = 11;
    const s32 H = 9;
    // Free existing grid
    if (gGrid.tiles) { free(gGrid.tiles); gGrid.tiles = nullptr; }
    // Initialize grid
    gGrid.width = W; gGrid.height = H; gGrid.gateOpen = false;
    // Allocate tiles
    gGrid.tiles = static_cast<Tile*>(malloc(sizeof(Tile) * static_cast<size_t>(W * H)));
    if (!gGrid.tiles) { return; }
    // Initialize all to walls
    for (s32 i = 0; i < W * H; ++i) gGrid.tiles[i] = TILE_WALL;

    // Path & objectives
    gGrid.tiles[Grid_Idx(&gGrid, 1, 4)] = TILE_FLOOR; // Player start
    gGrid.tiles[Grid_Idx(&gGrid, 2, 4)] = TILE_KEY;   // Key
    gGrid.tiles[Grid_Idx(&gGrid, 3, 4)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 4, 4)] = TILE_GATE;  // Gate
    gGrid.tiles[Grid_Idx(&gGrid, 5, 4)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 6, 4)] = TILE_FLOOR; // mark as floor first
    gGrid.tiles[Grid_Idx(&gGrid, 7, 4)] = TILE_EXIT;  // Exit

    // Enemy area (RIGHT side - far from player)
    gGrid.tiles[Grid_Idx(&gGrid, 9, 2)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 9, 3)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 9, 4)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 9, 5)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 9, 6)] = TILE_FLOOR;

    // Long connector
    gGrid.tiles[Grid_Idx(&gGrid, 8, 6)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 7, 6)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 6, 6)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 5, 6)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 5, 5)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 5, 4)] = TILE_FLOOR; // connects to main path

    // Optional collectible floors locations
    gGrid.tiles[Grid_Idx(&gGrid, 2, 2)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 3, 2)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 2, 3)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 4, 1)] = TILE_FLOOR;
    gGrid.tiles[Grid_Idx(&gGrid, 7, 1)] = TILE_FLOOR;

    // A convenient SAVE tile near exit (replaces floor)
    gGrid.tiles[Grid_Idx(&gGrid, 6, 4)] = TILE_SAVE;
}

static void FreeGrid() {
    if (gGrid.tiles) { free(gGrid.tiles); gGrid.tiles = nullptr; }
    gGrid.width = 0; gGrid.height = 0; gGrid.gateOpen = false;
}

// -----------------------------------------------------------------------------
// Save/Load (binary)
// -----------------------------------------------------------------------------
struct SaveHeader { u32 magic; u32 version; };
struct SaveGame {
    SaveHeader hdr;
    s32 roomId;

    // grid
    s32 width, height;
    u8  gateOpen;

    // player
    s32 playerIndex;
    s32 px, py;
    s32 health;
    s32 immSteps;
    s32 freezeTurns;

    // state flags
    u8  won, lost, paused;

    // counts
    s32 actorCount;
    s32 collectibleCount;
};

static bool SaveActors(FILE* f) {
    fwrite(&gActors.count, sizeof(s32), 1, f);
    for (s32 i = 0; i < gActors.count; ++i) {
        const Actor& a = gActors.data[i];
        fwrite(&a.type, sizeof(a.type), 1, f);
        fwrite(&a.x, sizeof(a.x), 1, f);
        fwrite(&a.y, sizeof(a.y), 1, f);
        u8 alive = a.alive ? 1 : 0;
        fwrite(&alive, sizeof(u8), 1, f);
    }
    return true;
}
static bool LoadActors(FILE* f) {
    s32 count = 0;
    if (fread(&count, sizeof(s32), 1, f) != 1) return false;
    Actors_Free(&gActors); Actors_Init(&gActors);
    for (s32 i = 0; i < count; ++i) {
        ActorType t; s32 x, y; u8 alive;
        if (fread(&t, sizeof(t), 1, f) != 1) return false;
        if (fread(&x, sizeof(x), 1, f) != 1) return false;
        if (fread(&y, sizeof(y), 1, f) != 1) return false;
        if (fread(&alive, sizeof(u8), 1, f) != 1) return false;
        s32 idx = Actors_Add(&gActors, t, x, y);
        if (idx < 0) return false;
        gActors.data[idx].alive = (alive != 0);
    }
    return true;
}
static bool SaveCollectibles(FILE* f) {
    fwrite(&gCollectibles.count, sizeof(s32), 1, f);
    for (s32 i = 0; i < gCollectibles.count; ++i) {
        const Collectible& c = gCollectibles.data[i];
        fwrite(&c.type, sizeof(c.type), 1, f);
        fwrite(&c.x, sizeof(c.x), 1, f);
        fwrite(&c.y, sizeof(c.y), 1, f);
        u8 active = c.active ? 1 : 0;
        fwrite(&active, sizeof(u8), 1, f);
    }
    return true;
}
static bool LoadCollectibles(FILE* f) {
    s32 count = 0;
    if (fread(&count, sizeof(s32), 1, f) != 1) return false;
    Col_Free(&gCollectibles); Col_Init(&gCollectibles);
    for (s32 i = 0; i < count; ++i) {
        CollectibleType t; s32 x, y; u8 active;
        if (fread(&t, sizeof(t), 1, f) != 1) return false;
        if (fread(&x, sizeof(x), 1, f) != 1) return false;
        if (fread(&y, sizeof(y), 1, f) != 1) return false;
        if (fread(&active, sizeof(u8), 1, f) != 1) return false;
        s32 idx = Col_Add(&gCollectibles, t, x, y);
        if (idx < 0) return false;
        gCollectibles.data[idx].active = (active != 0);
    }
    return true;
}
static bool SaveToFile(const char* path) {
    FILE* f = nullptr;
#if defined(_MSC_VER)
    if (fopen_s(&f, path, "wb") != 0 || !f) return false;
#else
    f = std::fopen(path, "wb");
    if (!f) return false;
#endif

    SaveGame sg{};
    sg.hdr.magic = 0x4D4D5A31u; // 'MMZ1'
    sg.hdr.version = 1u;
    sg.roomId = 1;

    sg.width = gGrid.width; sg.height = gGrid.height; sg.gateOpen = gGrid.gateOpen ? 1 : 0;

    const Actor& p = gActors.data[gPlayer.index];
    sg.playerIndex = gPlayer.index; sg.px = p.x; sg.py = p.y;
    sg.health = gPlayer.health;
    sg.immSteps = gPlayer.buffs.immunitySteps;
    sg.freezeTurns = gPlayer.buffs.freezeEnemyTurns;

    sg.won = gWon ? 1 : 0; sg.lost = gLost ? 1 : 0; sg.paused = gPaused ? 1 : 0;

    // header & meta
    fwrite(&sg, sizeof(sg), 1, f);

    // tiles
    s32 tileCount = gGrid.width * gGrid.height;
    fwrite(gGrid.tiles, sizeof(Tile), (size_t)tileCount, f);

    // actors & collectibles
    SaveActors(f);
    SaveCollectibles(f);

    std::fclose(f);
    return true;
}
static bool LoadFromFile(const char* path) {
    FILE* f = nullptr;
#if defined(_MSC_VER)
    if (fopen_s(&f, path, "rb") != 0 || !f) return false;
#else
    f = std::fopen(path, "rb");
    if (!f) return false;
#endif

    SaveGame sg{};
    if (fread(&sg, sizeof(sg), 1, f) != 1) { std::fclose(f); return false; }
    if (sg.hdr.magic != 0x4D4D5A31u || sg.hdr.version != 1u) { std::fclose(f); return false; }

    // rebuild grid
    FreeGrid();
    gGrid.width = sg.width; gGrid.height = sg.height; gGrid.gateOpen = (sg.gateOpen != 0);
    gGrid.tiles = static_cast<Tile*>(malloc(sizeof(Tile) * (size_t)(gGrid.width * gGrid.height)));
    if (!gGrid.tiles) { std::fclose(f); return false; }
    s32 tileCount = gGrid.width * gGrid.height;
    if (fread(gGrid.tiles, sizeof(Tile), (size_t)tileCount, f) != (size_t)tileCount) { std::fclose(f); return false; }

    // actors & collectibles
    if (!LoadActors(f)) { std::fclose(f); return false; }
    if (!LoadCollectibles(f)) { std::fclose(f); return false; }

    std::fclose(f);

    // restore player state
    gPlayer.index = sg.playerIndex;
    if (gPlayer.index >= 0 && gPlayer.index < gActors.count) {
        gActors.data[gPlayer.index].x = sg.px;
        gActors.data[gPlayer.index].y = sg.py;
    }
    gPlayer.health = sg.health;
    gPlayer.buffs.immunitySteps = sg.immSteps;
    gPlayer.buffs.freezeEnemyTurns = sg.freezeTurns;

    gWon = (sg.won != 0);
    gLost = (sg.lost != 0);
    gPaused = (sg.paused != 0);

    // make sure meshes match the new grid size
    const DrawSpace ds = MakeDrawSpace();
    BuildStaticGeometry(ds);
    return true;
}

// -----------------------------------------------------------------------------
// Input helpers
// -----------------------------------------------------------------------------
static bool RepeatTick(bool pressed, bool held, double now,
    double& firstTime, double& lastTime,
    double firstDelay, double repeatPeriod) {
    if (pressed) { firstTime = lastTime = now; return true; }
    if (!held) { firstTime = lastTime = -1.0; return false; }
    if (firstTime < 0.0) { firstTime = lastTime = now; return true; }
    if ((now - firstTime) >= firstDelay && (now - lastTime) >= repeatPeriod) {
        lastTime = now; return true;
    }
    return false;
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
    if (playerIdx < 0) return;

    gPlayer.index = playerIdx;
    gPlayer.buffs.immunitySteps = 0;
    gPlayer.buffs.freezeEnemyTurns = 0;
    gPlayer.health = kDefaultHealth;

    // Enemies (show different types)
    Actors_Add(&gActors, AT_MUMMY_WHITE, 9, 2);
    Actors_Add(&gActors, AT_MUMMY_RED, 9, 6);
    Actors_Add(&gActors, AT_SCORPION, 8, 3);

    // Collectibles
    Col_Add(&gCollectibles, CT_GEM, 2, 2);
    Col_Add(&gCollectibles, CT_ANKH, 4, 1);
    Col_Add(&gCollectibles, CT_SANDGLASS, 7, 1);
    Col_Add(&gCollectibles, CT_HEART, 3, 2); // health pickup

    // Flags
    gWon = gLost = gPaused = false;

    // Build graphics geometry
    const DrawSpace ds = MakeDrawSpace();
    BuildStaticGeometry(ds);

    // Load font (Roboto)
    EnsureGameFontLoaded();
}

void GS_PlayUpdate(void) {
    static double leftFirst = -1.0, leftLast = -1.0;
    static double rightFirst = -1.0, rightLast = -1.0;
    static double upFirst = -1.0, upLast = -1.0;
    static double downFirst = -1.0, downLast = -1.0;
    const double firstDelay = 0.25;
    const double repeatPeriod = 0.10;
    const double now = AEGetTime(nullptr);

    // Toggle pause
    if (AEInputCheckTriggered(AEVK_P)) {
        if (!gWon && !gLost) gPaused = !gPaused;
    }
    // Quick save/load allowed even while paused
    if (AEInputCheckTriggered(AEVK_F5)) { SaveToFile("savegame.dat"); }
    if (AEInputCheckTriggered(AEVK_F9)) { if (LoadFromFile("savegame.dat")) return; }

    if (gPaused) {
        return; // freeze gameplay while paused
    }

    // Win/Lose handling
    if (gWon) {
        if (AEInputCheckTriggered(AEVK_R)) {
            GS_PlayFree(); GS_PlayInit();
        }
        return;
    }
    if (gLost) {
        if (AEInputCheckTriggered(AEVK_R)) {
            GS_PlayFree(); GS_PlayInit();
        }
        return;
    }

    // Manual restart
    if (AEInputCheckTriggered(AEVK_R)) {
        GS_PlayFree(); GS_PlayInit(); return;
    }

    // Input states
    bool leftPressed = AEInputCheckTriggered(AEVK_LEFT);
    bool rightPressed = AEInputCheckTriggered(AEVK_RIGHT);
    bool upPressed = AEInputCheckTriggered(AEVK_UP);
    bool downPressed = AEInputCheckTriggered(AEVK_DOWN);

    bool leftHeld = AEInputCheckCurr(AEVK_LEFT);
    bool rightHeld = AEInputCheckCurr(AEVK_RIGHT);
    bool upHeld = AEInputCheckCurr(AEVK_UP);
    bool downHeld = AEInputCheckCurr(AEVK_DOWN);

    bool waitPressed = AEInputCheckTriggered(AEVK_SPACE);

    // Determine movement
    s32 dx = 0, dy = 0; bool acted = false;
    if (RepeatTick(leftPressed, leftHeld, now, leftFirst, leftLast, firstDelay, repeatPeriod)) { dx = -1; acted = true; }
    else if (RepeatTick(rightPressed, rightHeld, now, rightFirst, rightLast, firstDelay, repeatPeriod)) { dx = 1; acted = true; }
    else if (RepeatTick(upPressed, upHeld, now, upFirst, upLast, firstDelay, repeatPeriod)) { dy = -1; acted = true; }
    else if (RepeatTick(downPressed, downHeld, now, downFirst, downLast, firstDelay, repeatPeriod)) { dy = 1; acted = true; }

    // Execute player action
    auto PlayerTryMoveOrWait = [&](s32 mdx, s32 mdy, bool waitOnly) {
        Actor* playerActor = &gActors.data[gPlayer.index];
        if (!waitOnly && (mdx != 0 || mdy != 0)) {
            s32 newX = playerActor->x + mdx;
            s32 newY = playerActor->y + mdy;
            if (Grid_In(&gGrid, newX, newY) && !Grid_Blocking(&gGrid, newX, newY)) {
                playerActor->x = newX;
                playerActor->y = newY;
                // Key
                if (gGrid.tiles[Grid_Idx(&gGrid, newX, newY)] == TILE_KEY) {
                    Grid_ToggleGate(&gGrid, newX, newY);
                    gGrid.tiles[Grid_Idx(&gGrid, newX, newY)] = TILE_FLOOR;
                }
                // Collectibles
                s32 collectibleIdx = Col_FindAt(&gCollectibles, newX, newY);
                if (collectibleIdx >= 0) {
                    Col_ApplyAndConsume(&gCollectibles, collectibleIdx, &gPlayer);
                }
                // Exit
                if (gGrid.tiles[Grid_Idx(&gGrid, newX, newY)] == TILE_EXIT) {
                    gWon = true; return;
                }
            }
        }
        // Enemy turn if not won
        if (!gWon) {
            bool playerCaught = false;
            ResolveEnemyTurns(&gGrid, &gActors, &gPlayer, &playerCaught);
            if (playerCaught) {
                // lose a health if available; otherwise lose game
                if (gPlayer.health > 0 && gPlayer.buffs.immunitySteps == 0) {
                    gPlayer.health -= 1;
                    // small brief immunity after taking a hit
                    gPlayer.buffs.immunitySteps = 1;
                }
                else {
                    gLost = true;
                }
            }
        }
        };

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

    // Board layers
    DrawMesh(M_FloorGrid);
    DrawMesh(M_Walls);
    DrawMesh(M_Exits);
    DrawMesh(M_Keys);
    DrawMesh(M_Save);

    // Gate (open or closed)
    if (M_GateC && M_GateO) {
        DrawMesh(gGrid.gateOpen ? M_GateO : M_GateC);
    }

    // Collectibles
    for (s32 i = 0; i < gCollectibles.count; ++i) {
        const Collectible& col = gCollectibles.data[i];
        if (!col.active) continue;
        float cx = 0.0f, cy = 0.0f; CellCenterF(ds, col.x, col.y, cx, cy);
        AEGfxVertexList* mesh = nullptr;
        switch (col.type) {
        case CT_GEM:       mesh = M_Gem;       break;
        case CT_ANKH:      mesh = M_Ankh;      break;
        case CT_SANDGLASS: mesh = M_Sandglass; break;
        case CT_HEART:     mesh = M_Heart;     break;
        default: break;
        }
        if (mesh) DrawActorAt(mesh, cx, cy);
    }

    // Actors
    for (s32 i = 0; i < gActors.count; ++i) {
        const Actor& actor = gActors.data[i];
        if (!actor.alive) continue;
        float cx = 0.0f, cy = 0.0f; CellCenterF(ds, actor.x, actor.y, cx, cy);
        AEGfxVertexList* mesh = nullptr;
        if (i == gPlayer.index)                   mesh = M_Player;
        else if (actor.type == AT_MUMMY_WHITE)    mesh = M_MW;
        else if (actor.type == AT_MUMMY_RED)      mesh = M_MR;
        else if (actor.type == AT_SCORPION)       mesh = M_SC;
        if (mesh) DrawActorAt(mesh, cx, cy);
    }

    // --- Overlays (clear tint + centered text) ---
    auto DrawOverlayWithText = [&](float r, float g, float b, float a, const char* line1, const char* line2) {
        // 1) Full-screen tint
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        float minX = AEGfxGetWinMinX(), maxX = AEGfxGetWinMaxX();
        float minY = AEGfxGetWinMinY(), maxY = AEGfxGetWinMaxY();

        AEGfxMeshStart();
        u32 col = ARGB((u8)(a * 255), (u8)(r * 255), (u8)(g * 255), (u8)(b * 255));
        AddWorldQuad((minX + maxX) * 0.5f, (minY + maxY) * 0.5f,
            (maxX - minX) * 0.5f, (maxY - minY) * 0.5f, col);
        AEGfxVertexList* m = AEGfxMeshEnd();
        AEGfxMeshDraw(m, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(m);

        // 2) Big, centered text (printing requires BLEND mode)
        if (gFontId != -1) {
            const float titleScale = 1.1f;   // title
            const float infoScale = 0.75f;  // subtitle

            // Title (white)
            AEGfxPrint(gFontId, line1,
                0.0f, 0.15f,
                titleScale,
                1.0f, 1.0f, 1.0f, 1.0f);
            // Subtitle (light gray)
            AEGfxPrint(gFontId, line2,
                0.0f, -0.05f,
                infoScale,
                0.9f, 0.9f, 0.9f, 1.0f);
        }

        AEGfxSetBlendMode(AE_GFX_BM_NONE);
        };

    if (gPaused) {
        DrawOverlayWithText(0.05f, 0.05f, 0.05f, 0.80f,
            "Paused",
            "P: Resume   F5: Save   F9: Load   R: Restart");
    }
    if (gWon) {
        DrawOverlayWithText(0.05f, 0.60f, 0.10f, 0.80f,
            "You Win!",
            "R: Restart   F5: Save   F9: Load");
    }
    if (gLost) {
        DrawOverlayWithText(0.75f, 0.10f, 0.10f, 0.80f,
            "You Lose!",
            "R: Restart   F5: Save   F9: Load");
    }

    AEGfxEnd();
}

void GS_PlayFree(void) {
    FreeAllMeshes();
    Col_Free(&gCollectibles);
    Actors_Free(&gActors);
    FreeGrid();
    // Destroy font if loaded
    if (gFontId != -1) {
        AEGfxDestroyFont(gFontId);
        gFontId = -1;
    }
    gWon = gLost = gPaused = false;
}