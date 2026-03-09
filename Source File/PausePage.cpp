// PausePage.cpp
#include "pch.h"
#include "PausePage.h"
#include "GameStateManager.h"
#include "Main.h"

void PausePage_Load() {} // nothing to load -ths
void PausePage_Initialize() {} // -ths

void PausePage_Update()
{
    // Resume on P or ESC; return to previous gameplay state -ths
    if (AEInputCheckReleased(AEVK_P) || AEInputCheckReleased(AEVK_ESCAPE))
    {
        next = previous; // resume last state (e.g., GS_LEVEL1) -ths
    }
    // Quit on Q -ths
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist())
        next = GS_QUIT; // -ths
} // -ths

void PausePage_Draw()
{
    // simple dim overlay -ths
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f); // -ths
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);

    AEMtx33 scale, trans, mat;
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&mat, &trans, &scale);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.5f);
    AEGfxSetTransform(mat.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // text -ths
    AEGfxPrint(fontId, "PAUSED", -0.12f, 0.05f, 2.0f, 1, 1, 1, 1);
    AEGfxPrint(fontId, "Press P to Resume | ESC to Resume | Q to Quit", -0.45f, -0.10f, 1.0f, 1, 1, 1, 1);
} // -ths

void PausePage_Free() {} // -ths
void PausePage_Unload() {} // -ths