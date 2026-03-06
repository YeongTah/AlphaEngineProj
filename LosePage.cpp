/* Start Header ***************************************************************
/*!
\file LosePage.cpp
\author (added page controls & buttons) -ths
\brief Lose page shown when the player is caught. Original behavior retained,
       with Retry/Exit buttons added. -ths
*/
/* End Header *************************************************************** */

#include "pch.h"
#include "LosePage.h"
#include "GameStateManager.h"
#include "Main.h"
#include "Collision.h"   // IsAreaClicked -ths
#include "MouseCoor.h"   // TransformScreentoWorld -ths

// ---------------- UI placement (simple rectangles) ---------------- -ths
static float btnRetryX = -200.0f;  // center X of Retry button -ths
static float btnRetryY = -100.0f;  // center Y of Retry button -ths
static float btnExitX = 200.0f;  // center X of Exit button -ths
static float btnExitY = -100.0f;  // center Y of Exit button -ths
static float btnWidth = 280.0f;  // width of buttons -ths
static float btnHeight = 90.0f;  // height of buttons -ths

void LosePage_Load() { /* nothing to load; reuse pMesh/fontId -ths */ }
void LosePage_Initialize() { /* keep defaults -ths */ }

void LosePage_Update()
{
    // Mouse click → buttons -ths
    s32 mx, my; TransformScreentoWorld(mx, my); // screen→world -ths

    if (AEInputCheckReleased(AEVK_LBUTTON))
    {
        if (IsAreaClicked(btnRetryX, btnRetryY, btnWidth, btnHeight, (float)mx, (float)my))
        {
            previous = GS_LEVEL1;      // restart Level 1 -ths
            next = GS_RESTART;     // main loop handles reload -ths
            return;
        }
        if (IsAreaClicked(btnExitX, btnExitY, btnWidth, btnHeight, (float)mx, (float)my))
        {
            next = MAINMENUSTATE;      // exit to Main Menu -ths
            return;
        }
    }

    // Keyboard shortcuts (kept + aligned with buttons) -ths
    if (AEInputCheckReleased(AEVK_R)) { previous = GS_LEVEL1; next = GS_RESTART; return; } // -ths
    if (AEInputCheckReleased(AEVK_RETURN)) { next = MAINMENUSTATE; return; }                     // -ths
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) { next = GS_QUIT; return; }        // -ths
}

void LosePage_Draw()
{
    // Dark red background -ths
    AEGfxSetBackgroundColor(0.15f, 0.0f, 0.0f);

    // Title text -ths
    AEGfxPrint(fontId, "CAUGHT BY THE MUMMY!", -0.33f, 0.25f, 2.0f, 1, 1, 1, 1);

    // Draw buttons as colored rectangles using the shared square mesh -ths
    // Retry button (red accent) -ths
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.50f, 0.05f, 0.05f, 1.0f);

    AEMtx33 s, t, m;
    AEMtx33Scale(&s, btnWidth, btnHeight);
    AEMtx33Trans(&t, btnRetryX, btnRetryY);
    AEMtx33Concat(&m, &t, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxPrint(fontId, "RETRY", -0.12f, -0.13f, 1.2f, 1, 1, 1, 1);

    // Exit button (dark) -ths
    AEGfxSetColorToMultiply(0.10f, 0.10f, 0.10f, 1.0f);
    AEMtx33Scale(&s, btnWidth, btnHeight);
    AEMtx33Trans(&t, btnExitX, btnExitY);
    AEMtx33Concat(&m, &t, &s);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxPrint(fontId, "EXIT", 0.22f, -0.13f, 1.2f, 1, 1, 1, 1);

    // Helper text -ths
    AEGfxPrint(fontId, "R: Retry  |  ENTER: Main Menu  |  Q: Quit",
        -0.42f, -0.45f, 0.9f, 1, 1, 1, 1);
}

void LosePage_Free() { /* none -ths */ }
void LosePage_Unload() { /* none -ths */ }