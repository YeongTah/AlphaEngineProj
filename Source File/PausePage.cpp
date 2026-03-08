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
    if (AEInputCheckReleased(AEVK_P) ||
        AEInputCheckReleased(AEVK_ESCAPE))
    {
        next = previous; // resume last state (e.g., GS_LEVEL1) -ths
    }
    // Quit on Q -ths
    if (AEInputCheckReleased(AEVK_Q) ||
        0 == AESysDoesWindowExist())
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

    // AEGfxPrint(fontId, "PAUSED", -0.12f, 0.05f, 2.0f, 1, 1, 1, 1);
    // AEGfxPrint(fontId, "Press P to Resume \n ESC to Resume \n Q to Quit", -0.45f, -0.10f, 1.0f, 1, 1, 1, 1);
    // ^ Original text draws (left-aligned and multi-line) kept here as reference.

    // ============================
    // ADDED: Centered text layout
    // Split multi-line text into separate centered lines using AEGfxGetPrintSize.
    // ============================
    const float titleScale = 2.0f;
    const float lineScale = 1.0f;

    float w = 0.0f, h = 0.0f;

    // Title line
    const char* t0 = "PAUSED";
    AEGfxGetPrintSize(fontId, t0, titleScale, &w, &h);     // ADDED
    float x0 = -0.5f * w;                                   // ADDED
    AEGfxPrint(fontId, t0, x0, 0.05f, titleScale, 1, 1, 1, 1);

    // Instruction lines
    const char* t1 = "Press P to Resume";
    AEGfxGetPrintSize(fontId, t1, lineScale, &w, &h);      // ADDED
    float x1 = -0.5f * w;                                   // ADDED
    AEGfxPrint(fontId, t1, x1, -0.10f, lineScale, 1, 1, 1, 1);

    const char* t2 = "ESC to Level Select";
    AEGfxGetPrintSize(fontId, t2, lineScale, &w, &h);      // ADDED
    float x2 = -0.5f * w;                                   // ADDED
    AEGfxPrint(fontId, t2, x2, -0.18f, lineScale, 1, 1, 1, 1);

    const char* t3 = "Q to Quit";
    AEGfxGetPrintSize(fontId, t3, lineScale, &w, &h);      // ADDED
    float x3 = -0.5f * w;                                   // ADDED
    AEGfxPrint(fontId, t3, x3, -0.26f, lineScale, 1, 1, 1, 1);
} // -ths

void PausePage_Free() {} // -ths
void PausePage_Unload() {} // -ths
