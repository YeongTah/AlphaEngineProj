/* Start Header ***************************************************************
\file       Debug.h
\coders     Lai Yeong Tah
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#pragma once

// ---- engine forward declarations (already available via pch.h / AEEngine.h) ----
// included here only for the struct definitions below; levels include pch.h first.

// ============================================================
// DebugEntityInfo
// Plain-old-data bundle that each level fills in and passes to
// Debug_DrawOverlay(). Every field has a safe default so levels
// that don't have a particular entity (e.g. no scorpion) just
// leave it at 0 / false and it won't be drawn.
// ============================================================
struct DebugEntityInfo
{
    // ---- Player ----
    float playerX = 0.f, playerY = 0.f, playerSize = 50.f;

    // ---- Main mummies (up to 3) ----
    float mummy1X = 0.f, mummy1Y = 0.f, mummy1Size = 50.f;
    bool  hasMummy1 = false;

    float mummy2X = 0.f, mummy2Y = 0.f, mummy2Size = 50.f;
    bool  hasMummy2 = false;

    float mummy3X = 0.f, mummy3Y = 0.f, mummy3Size = 50.f;
    bool  hasMummy3 = false;

    // ---- Level-2 scorpion (optional) ----
    float scorpionX = 0.f, scorpionY = 0.f, scorpionSize = 50.f;
    bool  hasScorpion = false;

    // ---- Box mummies spawned from treasure chests ----
    // Caller fills boxMummyX/Y/Size arrays and sets boxMummyCount.
    static const int MAX_BOX_MUMMIES = 8;
    float boxMummyX[MAX_BOX_MUMMIES] = {};
    float boxMummyY[MAX_BOX_MUMMIES] = {};
    float boxMummySize[MAX_BOX_MUMMIES] = {};
    int   boxMummyCount = 0;

    // ---- Treasure box ----
    float treasureBoxX = 0.f, treasureBoxY = 0.f, treasureBoxSize = 45.f;
    bool  treasureBoxActive = false;

    // ---- Exit portal ----
    float exitX = 0.f, exitY = 0.f, exitSize = 50.f;

    // ---- Legacy coin entity ----
    float coinX = 2000.f, coinY = 2000.f, coinSize = 40.f; // x>=1000 = off-screen / collected

    // ---- Power-up pickup ----
    float powerupX = 0.f, powerupY = 0.f, powerupSize = 30.f;
    bool  powerupActive = false;

    // ---- HUD counters ----
    int coinCounter = 0;
    int turnCounter = 0;

    // ---- Powerup states (shown in HUD text) ----
    bool invincibleActive = false;
    int  invFrames = 0;
    bool freezeActive = false;
    int  freezeFrames = 0;
    bool speedActive = false;
    int  speedTurns = 0;

    // ---- Overlay / flow flags ----
    bool isPaused = false;
    bool isWin = false;
    bool isLose = false;
    int  popupFrames = 0;

    // ---- Grid tile size (world units per cell) ----
    float tileSize = 50.f;
};

// ============================================================
// Public API
// ============================================================

// Call once per frame in Level_Update() to wire up the F1 toggle.
void Debug_HandleToggle();

// Returns true while the overlay is active (useful if callers want to skip
// expensive work while debug is off, though it's not required).
bool Debug_IsActive();

// Call at the very end of Level_Draw() (after DrawPauseButton / all HUD).
// Draws hitbox outlines + variable HUD when the overlay is on.
// Requires the shared pMesh and fontId to already be valid (they always are
// during Draw).
void Debug_DrawOverlay(const DebugEntityInfo& info);