// LosePage.cpp
#include "pch.h"
#include "LosePage.h"
#include "GameStateManager.h"
#include "Main.h"

#include "leveleditor.hpp" // for CreateSquareMesh and pMesh

void LosePage_Load() { pMesh = CreateSquareMesh(); }
void LosePage_Initialize() {}

void LosePage_Update()
{
    // Enter: Level Select, R: restart the level that was just lost, Q: quit
    if (AEInputCheckReleased(AEVK_RETURN)) next = LEVELPAGE;
    if (AEInputCheckReleased(AEVK_R)) { next = previous; } // previous holds the level that triggered the lose
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) next = GS_QUIT;
}

void LosePage_Draw()
{
    AEGfxSetBackgroundColor(0.15f, 0, 0);

    // Draw a full-screen red rectangle to completely cover the game beneath
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.5f, 0.0f, 0.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEMtx33 scale, trans, mat;
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&mat, &trans, &scale);
    AEGfxSetTransform(mat.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    AEGfxPrint(fontId, "CAUGHT BY THE MUMMY!", -0.33f, 0.10f, 2.0f, 1, 1, 1, 1);
    AEGfxPrint(fontId, "ENTER: Level Select  |  R: Restart  |  Q: Quit", -0.48f, -0.05f, 1.0f, 1, 1, 1, 1);
}

void LosePage_Free() {}
void LosePage_Unload() { if (pMesh) { AEGfxMeshFree(pMesh); pMesh = nullptr; } }