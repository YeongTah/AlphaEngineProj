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
//   ENTER  : go to Level Select page (LEVELPAGE)
//   R      : restart the level that was lost (next = previous, which holds
//             the losing level's state ID, set by the level before transitioning)
//   Q / close window : quit the game
// ----------------------------------------------------------------------------
void LosePage_Update()
{
    if (AEInputCheckReleased(AEVK_RETURN)) next = LEVELPAGE;
    if (AEInputCheckReleased(AEVK_R)) { next = previous; } // previous = the level that triggered the lose
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) next = GS_QUIT;
}

// ----------------------------------------------------------------------------
// LosePage_Draw
// Renders the Lose overlay:
//   1. Dark red background color.
//   2. Full-screen semi-transparent red rectangle (1600x900) to cover the game.
//   3. "CAUGHT BY THE MUMMY!" title text (large, white, centered).
//   4. Instruction text showing ENTER / R / Q key bindings (smaller, white).
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

    // Print title and instructions in white
    AEGfxPrint(fontId, "CAUGHT BY THE MUMMY!", -0.40f, 0.12f, 1.8f, 0.65f, 0.12f, 0.12f, 1.0f);

    // options
    AEGfxPrint(fontId, "[ENTER] Level Select", -0.18f, -0.02f, 0.75f, 1.0f, 0.95f, 0.82f, 1.0f);
    AEGfxPrint(fontId, "[R] Restart", -0.08f, -0.11f, 0.75f, 1.0f, 0.95f, 0.82f, 1.0f);
    AEGfxPrint(fontId, "[Q] Quit", -0.06f, -0.20f, 0.75f, 1.0f, 0.95f, 0.82f, 1.0f);
}

// LosePage_Free -- nothing to clean up.
void LosePage_Free() {}

// ----------------------------------------------------------------------------
// LosePage_Unload
// Frees the shared mesh (only when LosePage is used as a standalone GSM state).
// When used as an overlay inside a level, the level's own Unload handles pMesh.
// ----------------------------------------------------------------------------
void LosePage_Unload() { if (pMesh) { AEGfxMeshFree(pMesh); pMesh = nullptr; } }