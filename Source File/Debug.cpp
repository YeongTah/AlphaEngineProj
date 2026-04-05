/* Start Header ***************************************************************
\file       Debug.cpp
\coders     Lai Yeong Tah
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#include "pch.h"
#include "Debug.h"
#include "GridUtils.h"      // GRID_ROWS, GRID_COLS, GRID_TILE_SIZE, GridToWorldCenter
#include "leveleditor.hpp"  // level[][], pMesh, fontId
#include <cstdio>           // std::snprintf

// ============================================================
// Externs shared across all level files
// ============================================================
extern int   level[18][32];        // shared tile grid
extern void  GridToWorldCenter(int row, int col, float& outX, float& outY);
extern void  WorldToGrid(float worldX, float worldY, int& outRow, int& outCol);

// pMesh and fontId are declared in leveleditor.cpp / Main.cpp -- already extern
// via leveleditor.hpp.  No additional extern needed here.

// ============================================================
// Module-local state
// ============================================================
static bool s_debugActive = false;  // toggled by F1

// ============================================================
// Internal helper: DrawDebugOutline
// Draws a hollow coloured rectangle at world position (cx, cy)
// with dimensions (w x h) using four thin filled edge quads.
// r, g, b in [0..1]; thickness in world units.
// ============================================================
static void DrawDebugOutline(float cx, float cy, float w, float h,
    float r, float g, float b, float thickness = 2.0f)
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(r, g, b, 1.0f);

    AEMtx33 s, t, m;
    float hw = w * 0.5f;
    float hh = h * 0.5f;

    // Top edge
    AEMtx33Scale(&s, w, thickness);
    AEMtx33Trans(&t, cx, cy + hh - thickness * 0.5f);
    AEMtx33Concat(&m, &t, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Bottom edge
    AEMtx33Scale(&s, w, thickness);
    AEMtx33Trans(&t, cx, cy - hh + thickness * 0.5f);
    AEMtx33Concat(&m, &t, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Left edge
    AEMtx33Scale(&s, thickness, h);
    AEMtx33Trans(&t, cx - hw + thickness * 0.5f, cy);
    AEMtx33Concat(&m, &t, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Right edge
    AEMtx33Scale(&s, thickness, h);
    AEMtx33Trans(&t, cx + hw - thickness * 0.5f, cy);
    AEMtx33Concat(&m, &t, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
}

// ============================================================
// Public: Debug_HandleToggle
// Call once per frame inside Level_Update() (when not paused /
// in overlay).  Pressing F1 flips the debug state.
// ============================================================
void Debug_HandleToggle()
{
    if (AEInputCheckReleased(AEVK_F1))
        s_debugActive = !s_debugActive;
}

// ============================================================
// Public: Debug_IsActive
// ============================================================
bool Debug_IsActive()
{
    return s_debugActive;
}

// ============================================================
// Public: Debug_DrawOverlay
// Main entry point. Call at the very end of Level_Draw().
// ============================================================
void Debug_DrawOverlay(const DebugEntityInfo& info)
{
    if (!s_debugActive) return;

    // =======================
    // 1. Grid tile outlines
    // =======================
    for (int row = 0; row < GRID_ROWS; ++row)
    {
        for (int col = 0; col < GRID_COLS; ++col)
        {
            float wx, wy;
            GridToWorldCenter(row, col, wx, wy);

            int tile = level[row][col];
            if (tile == 0 || tile == 4)
            {
                // Walkable / coin tile: thin black outline
                DrawDebugOutline(wx, wy, info.tileSize, info.tileSize,
                    0.0f, 0.0f, 0.0f, 1.0f);
            }
            else if (tile == 1)
            {
                // Wall: thin black outline
                DrawDebugOutline(wx, wy, info.tileSize, info.tileSize,
                    1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    }

    // =======================
    // 2. Entity hitbox outlines (drawn back-to-front so the
    //    player outline is always visible on top)
    // =======================

    // Exit portal  (green)
    DrawDebugOutline(info.exitX, info.exitY, info.exitSize, info.exitSize,
        0.0f, 1.0f, 0.0f, 2.0f);

    // Legacy coin  (blue) -- only when on-screen
    if (info.coinX < 1000.0f)
        DrawDebugOutline(info.coinX, info.coinY, info.coinSize, info.coinSize,
            0.2f, 0.4f, 1.0f, 2.0f);

    // Power-up pickup  (purple)
    if (info.powerupActive)
        DrawDebugOutline(info.powerupX, info.powerupY,
            info.powerupSize, info.powerupSize,
            0.8f, 0.2f, 1.0f, 2.0f);

    // Treasure box  (yellow)
    if (info.treasureBoxActive)
        DrawDebugOutline(info.treasureBoxX, info.treasureBoxY,
            info.treasureBoxSize, info.treasureBoxSize,
            1.0f, 1.0f, 0.0f, 2.0f);

    // Box mummies  (orange, slightly thicker)
    for (int i = 0; i < info.boxMummyCount; ++i)
        DrawDebugOutline(info.boxMummyX[i], info.boxMummyY[i],
            info.boxMummySize[i], info.boxMummySize[i],
            1.0f, 0.5f, 0.0f, 3.0f);

    // Spider  (red -- Level 2 only)
    if (info.hasScorpion)
        DrawDebugOutline(info.scorpionX, info.scorpionY,
            info.scorpionSize, info.scorpionSize,
            1.0f, 0.0f, 0.0f, 3.0f);

    // Mummy 3  (red -- Level 3 only)
    if (info.hasMummy3)
        DrawDebugOutline(info.mummy3X, info.mummy3Y,
            info.mummy3Size, info.mummy3Size,
            1.0f, 0.0f, 0.0f, 3.0f);

    // Mummy 2  (red -- Level 2 & 3)
    if (info.hasMummy2)
        DrawDebugOutline(info.mummy2X, info.mummy2Y,
            info.mummy2Size, info.mummy2Size,
            1.0f, 0.0f, 0.0f, 3.0f);

    // Mummy 1 / main mummy  (red)
    if (info.hasMummy1)
        DrawDebugOutline(info.mummy1X, info.mummy1Y,
            info.mummy1Size, info.mummy1Size,
            1.0f, 0.0f, 0.0f, 3.0f);

    // Player  (cyan, thickest -- always on top)
    DrawDebugOutline(info.playerX, info.playerY,
        info.playerSize, info.playerSize,
        0.0f, 1.0f, 1.0f, 4.0f);

    // =======================
    // 3. Runtime variable HUD (stacked text lines, lower-left)
    // =======================
    // Restore texture mode so AEGfxPrint renders correctly
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    char buf[160];
    const float x0 = -0.98f; // left NDC edge
    const float y0 = 0.65f; // top of debug HUD block (below existing coin/powerup HUD)
    const float lineH = 0.09f; // NDC line spacing
    const float sc = 0.70f; // text scale

    // Player world pos + grid cell
    int pr, pc;
    WorldToGrid(info.playerX, info.playerY, pr, pc);
    std::snprintf(buf, sizeof(buf),
        "[DBG] Player  world=(%.0f,%.0f)  grid=(%d,%d)",
        info.playerX, info.playerY, pr, pc);
    AEGfxPrint(fontId, buf, x0, y0, sc, 1.0f, 1.0f, 1.0f, 1.0f);  // white

    // Mummy 1
    if (info.hasMummy1)
    {
        int mr, mc;
        WorldToGrid(info.mummy1X, info.mummy1Y, mr, mc);
        std::snprintf(buf, sizeof(buf),
            "[DBG] Mummy1  world=(%.0f,%.0f)  grid=(%d,%d)",
            info.mummy1X, info.mummy1Y, mr, mc);
        AEGfxPrint(fontId, buf, x0, y0 - lineH, sc, 1.0f, 1.0f, 1.0f, 1.0f);  // white
    }

    // Mummy 2 (Level 2 & 3)
    if (info.hasMummy2)
    {
        int mr, mc;
        WorldToGrid(info.mummy2X, info.mummy2Y, mr, mc);
        std::snprintf(buf, sizeof(buf),
            "[DBG] Mummy2  world=(%.0f,%.0f)  grid=(%d,%d)",
            info.mummy2X, info.mummy2Y, mr, mc);
        AEGfxPrint(fontId, buf, x0, y0 - lineH * 2, sc, 1.0f, 1.0f, 1.0f, 1.0f);  // white
    }

    // Mummy 3 (Level 3 only)
    if (info.hasMummy3)
    {
        int mr, mc;
        WorldToGrid(info.mummy3X, info.mummy3Y, mr, mc);
        std::snprintf(buf, sizeof(buf),
            "[DBG] Mummy3  world=(%.0f,%.0f)  grid=(%d,%d)",
            info.mummy3X, info.mummy3Y, mr, mc);
        AEGfxPrint(fontId, buf, x0, y0 - lineH * 3, sc, 1.0f, 1.0f, 1.0f, 1.0f);  // white
    }

    // Counters line
    std::snprintf(buf, sizeof(buf),
        "[DBG] Coins=%d  Turns=%d  BoxMummies=%d",
        info.coinCounter, info.turnCounter, info.boxMummyCount);
    AEGfxPrint(fontId, buf, x0, y0 - lineH * 4, sc, 1.0f, 1.0f, 1.0f, 1.0f);  // white

    // Powerup states
    std::snprintf(buf, sizeof(buf),
        "[DBG] Inv=%d(%dfrm)  Frz=%d(%dfrm)  Spd=%d(%dtrn)",
        (int)info.invincibleActive, info.invFrames,
        (int)info.freezeActive, info.freezeFrames,
        (int)info.speedActive, info.speedTurns);
    AEGfxPrint(fontId, buf, x0, y0 - lineH * 5, sc, 1.0f, 1.0f, 1.0f, 1.0f);  // white

    // Treasure box
    std::snprintf(buf, sizeof(buf),
        "[DBG] TreasureBox=%s  world=(%.0f,%.0f)",
        info.treasureBoxActive ? "ACTIVE" : "GONE",
        info.treasureBoxX, info.treasureBoxY);
    AEGfxPrint(fontId, buf, x0, y0 - lineH * 6, sc, 1.0f, 1.0f, 1.0f, 1.0f);  // white

    // Flow flags
    std::snprintf(buf, sizeof(buf),
        "[DBG] Paused=%d  Win=%d  Lose=%d  PopupFrames=%d",
        (int)info.isPaused, (int)info.isWin,
        (int)info.isLose, info.popupFrames);
    AEGfxPrint(fontId, buf, x0, y0 - lineH * 7, sc, 1.0f, 1.0f, 1.0f, 1.0f);  // white

    // Legend / toggle reminder
    AEGfxPrint(fontId,
        "[F1] Debug ON  |  Cyan=Player  Red=Mummies/Spider  "
        "Orange=BoxMummy  Yellow=Chest  Green=Exit",
        x0, y0 - lineH * 8, sc, 1.0f, 1.0f, 1.0f, 1.0f);  // white
}