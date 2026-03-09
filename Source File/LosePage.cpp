// LosePage.cpp
// Shown when the player is caught by a mummy.
// Displayed as an overlay from within Level1/2/3_Draw() (not a separate GSM state).
// Also reachable as a standalone GSM state (GS_LOSE) if needed in the future.
#include "pch.h"
#include "LosePage.h"
#include "GameStateManager.h"
#include "Main.h"
#include "leveleditor.hpp" // for CreateSquareMesh and pMesh

// ----------------------------------------------------------------------------
// LosePage_Load
// Creates the shared square mesh used to draw the red background overlay.
// ----------------------------------------------------------------------------
void LosePage_Load() { pMesh = CreateSquareMesh(); }

// LosePage_Initialize -- no state to set up.
void LosePage_Initialize() {}

// ----------------------------------------------------------------------------
// LosePage_Update
// Handles player input while the Lose screen is showing.
//
// ENTER : go to Level Select page (LEVELPAGE)
// R     : restart the level that was lost (next = previous, which holds
//         the losing level's state ID, set by the level before transitioning)
// Q / close window : quit the game
// ----------------------------------------------------------------------------
void LosePage_Update()
{
    if (AEInputCheckReleased(AEVK_RETURN)) next = LEVELPAGE;
    if (AEInputCheckReleased(AEVK_R)) { next = previous; } // previous = the level that triggered the lose
    if (AEInputCheckReleased(AEVK_Q) ||
        0 == AESysDoesWindowExist()) next = GS_QUIT;
}

// ----------------------------------------------------------------------------
// LosePage_Draw
// Renders the Lose overlay:
// 1. Dark red background color.
// 2. Full-screen semi-transparent red rectangle (1600x900) to cover the game.
// 3. "CAUGHT BY THE MUMMY!" title text (large, white, centered).
// 4. Instruction text showing ENTER / R / Q key bindings (smaller, white).
// Called directly from Level1/2/3_Draw() when the lose flag is active,
// so no separate GSM state transition is needed for the overlay use case.
// ----------------------------------------------------------------------------
void LosePage_Draw()
{
    AEGfxSetBackgroundColor(0.18f, 0.05f, 0.05f);

    // Draw full-screen red tint rectangle over the game world
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.5f, 0.0f, 0.0f, 1.0f); // dark red fill
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEMtx33 scale, trans, mat;
    AEMtx33Scale(&scale, 1600.0f, 900.0f); // full screen
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&mat, &trans, &scale);
    AEGfxSetTransform(mat.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // ============================
    // ADDED: Centered text helpers
    // ============================
    const float titleScale = 1.8f;
    const float itemScale = 0.75f;

    float w = 0.0f, h = 0.0f;

    // Title
    const char* t0 = "CAUGHT BY THE MUMMY!";
    AEGfxGetPrintSize(fontId, t0, titleScale, &w, &h);      // ADDED
    float x0 = -0.5f * w;                                    // ADDED (center)
    AEGfxPrint(fontId, t0, x0, 0.12f, titleScale, 0.65f, 0.12f, 0.12f, 1.0f);

    // Options
    const char* t1 = "[ENTER] Level Select";
    AEGfxGetPrintSize(fontId, t1, itemScale, &w, &h);        // ADDED
    float x1 = -0.5f * w;                                    // ADDED
    AEGfxPrint(fontId, t1, x1, -0.02f, itemScale, 1.0f, 0.95f, 0.82f, 1.0f);

    const char* t2 = "[R] Restart";
    AEGfxGetPrintSize(fontId, t2, itemScale, &w, &h);        // ADDED
    float x2 = -0.5f * w;                                    // ADDED
    AEGfxPrint(fontId, t2, x2, -0.11f, itemScale, 1.0f, 0.95f, 0.82f, 1.0f);

    const char* t3 = "[Q] Quit";
    AEGfxGetPrintSize(fontId, t3, itemScale, &w, &h);        // ADDED
    float x3 = -0.5f * w;                                    // ADDED
    AEGfxPrint(fontId, t3, x3, -0.20f, itemScale, 1.0f, 0.95f, 0.82f, 1.0f);
}

// LosePage_Free -- nothing to clean up.
void LosePage_Free() {}

// ----------------------------------------------------------------------------
// LosePage_Unload
// Frees the shared mesh (only when LosePage is used as a standalone GSM state).
// When used as an overlay inside a level, the level's own Unload handles pMesh.
// ----------------------------------------------------------------------------
void LosePage_Unload() { if (pMesh) { AEGfxMeshFree(pMesh); pMesh = nullptr; } }