// WinPage.cpp
// Shown when the player reaches the exit portal (win condition).
// Displayed as an overlay from within Level1/2/3_Draw() OR as a standalone
// GSM state (GS_WIN) -- both paths use these same draw/update functions.
#include "pch.h"
#include "WinPage.h"
#include "GameStateManager.h"
#include "Level1.h"
#include "Main.h"
#include "leveleditor.hpp" // for CreateSquareMesh and pMesh

// ----------------------------------------------------------------------------
// WinPage_Load
// Creates the shared square mesh used to draw the green background overlay.
// ----------------------------------------------------------------------------
void WinPage_Load() { pMesh = CreateSquareMesh(); }

// WinPage_Initialize -- no state to set up.
void WinPage_Initialize() {}

// ----------------------------------------------------------------------------
// WinPage_Update
// Handles player input while the Win screen is showing.
//
//   ENTER  : return to Level Select page (LEVELPAGE)
//   R      : replay the level that was just won (next = previous, which holds
//             the winning level's state ID set before transitioning to GS_WIN)
//   Q / close window : quit the game
// ----------------------------------------------------------------------------
void WinPage_Update()
{
    if (AEInputCheckReleased(AEVK_RETURN)) next = LEVELPAGE;
    if (AEInputCheckReleased(AEVK_R)) { next = previous; } // previous = the level that triggered GS_WIN
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) next = GS_QUIT;
}

// ----------------------------------------------------------------------------
// WinPage_Draw
// Renders the Win overlay:
//   1. Dark green background color.
//   2. Full-screen semi-transparent green rectangle (1600x900) over the game.
//   3. "YOU ESCAPED!" title text (large, white, centered).
//   4. Instruction text showing ENTER / R / Q key bindings (smaller, white).
// Called directly from Level1/2/3_Draw() when the win flag is active,
// and also by the GS_WIN state path in Main.cpp.
// ----------------------------------------------------------------------------
void WinPage_Draw()
{
    AEGfxSetBackgroundColor(0.22f, 0.14f, 0.09f);

    // Draw full-screen green tint rectangle over the game world
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 0.25f, 0.0f, 1.0f); // dark green fill
    AEGfxSetColorToAdd(0.08f, 0.05f, 0.02f, 1.0f);

    AEMtx33 scale, trans, mat;
    AEMtx33Scale(&scale, 1600.0f, 900.0f); // full screen
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&mat, &trans, &scale);
    AEGfxSetTransform(mat.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Print title and instructions in white
    AEGfxPrint(fontId, "YOU ESCAPED!", -0.30f, 0.12f, 2.0f, 0.85f, 0.68f, 0.15f, 1.0f);
    AEGfxPrint(fontId, "[ENTER] Level Select", -0.18f, -0.02f, 0.75f, 1.0f, 0.95f, 0.82f, 1.0f);
    AEGfxPrint(fontId, "[R] Restart", -0.08f, -0.11f, 0.75f, 1.0f, 0.95f, 0.82f, 1.0f);
    AEGfxPrint(fontId, "[Q] Quit", -0.06f, -0.20f, 0.75f, 1.0f, 0.95f, 0.82f, 1.0f);
}

// WinPage_Free -- nothing to clean up.
void WinPage_Free() {}

// ----------------------------------------------------------------------------
// WinPage_Unload
// Frees the shared mesh (only when WinPage is used as a standalone GSM state).
// ----------------------------------------------------------------------------
void WinPage_Unload() { if (pMesh) { AEGfxMeshFree(pMesh); pMesh = nullptr; } }