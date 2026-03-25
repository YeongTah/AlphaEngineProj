// PausePage.cpp
#include "pch.h"
#include "PausePage.h"
#include "GameStateManager.h"
#include "Main.h"
#include "leveleditor.hpp"  // for print_file + readfile
#include "GridUtils.h"      // for world coordinate transforms
#include <cstring>          // for strlen -ths
#include "Confirmation.h"   // for Confirmation_Level() helper to show confirmation popups on important actions
static AEAudio sfxButton;
static AEAudioGroup pauseGroup;

// ----------------------------------------------------------------------------
// Load  -  create the shared mesh (still needed for the dim overlay rectangle)
// ----------------------------------------------------------------------------
void PausePage_Load()
{
    pMesh = CreateSquareMesh();
    pauseGroup = AEAudioCreateGroup();
    sfxButton = AEAudioLoadSound("Assets/audio/button.wav");
}

void PausePage_Initialize() {}

// ----------------------------------------------------------------------------
// Update - handle input while paused
// ----------------------------------------------------------------------------
void PausePage_Update()
{
    // Resume on P (no confirmation)
    if (AEInputCheckReleased(AEVK_P))
    {
        next = previous;
        return;
    }

    // Restart level on R (with confirmation)
    if (AEInputCheckReleased(AEVK_R))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, pauseGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_PAUSE, GS_RESTART, "Are you sure you want to restart?");
        next = CONFIRM;
        return;
    }

    // Level Select on B (with confirmation)
    if (AEInputCheckReleased(AEVK_B))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, pauseGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_PAUSE, LEVELPAGE, "Are you sure you want to go to Level Select?");
        next = CONFIRM;
        return;
    }

    // Quit on ESC (with confirmation)
    if (AEInputCheckReleased(AEVK_ESCAPE))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, pauseGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_PAUSE, GS_QUIT, "Are you sure you want to quit the game?");
        next = CONFIRM;
        return;
    }

    // Save on F5 (no confirmation)
    if (AEInputCheckReleased(AEVK_F5))
    {
        print_file();
    }

    // Load on F9 (no confirmation)
    if (AEInputCheckReleased(AEVK_F9))
    {
        readfile();
        next = previous;  // resume after load
    }
}

// ----------------------------------------------------------------------------
// Draw - render the pause overlay with text instructions only
// ----------------------------------------------------------------------------
void PausePage_Draw()
{
    // Dim the background
    AEGfxSetBackgroundColor(0, 0, 0);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);

    AEMtx33 scale, trans, mat;
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0, 0);
    AEMtx33Concat(&mat, &trans, &scale);

    AEGfxSetColorToMultiply(0, 0, 0, 0.5f);
    AEGfxSetTransform(mat.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // --- Title "PAUSED" ---
    const char* title = "PAUSED";
    float w, h;
    const float titleScale = 2.0f;
    AEGfxGetPrintSize(fontId, title, titleScale, &w, &h);
    AEGfxPrint(fontId, title, -0.5f * w, 0.20f, titleScale, 1, 1, 1, 1); // white

    // --- Instruction lines (centered, beige color) ---
    const float lineScale = 1.0f;
    const float beigeR = 1.0f, beigeG = 0.95f, beigeB = 0.82f; // matches LosePage

    // Resume
    const char* t1 = "Press P to Resume";
    AEGfxGetPrintSize(fontId, t1, lineScale, &w, &h);
    AEGfxPrint(fontId, t1, -0.5f * w, 0.02f, lineScale, beigeR, beigeG, beigeB, 1.0f);

    // Level Select
    const char* t2 = "B to Level Select";
    AEGfxGetPrintSize(fontId, t2, lineScale, &w, &h);
    AEGfxPrint(fontId, t2, -0.5f * w, -0.08f, lineScale, beigeR, beigeG, beigeB, 1.0f);

    // Restart
    const char* t3 = "[R] Restart Level";
    AEGfxGetPrintSize(fontId, t3, lineScale, &w, &h);
    AEGfxPrint(fontId, t3, -0.5f * w, -0.18f, lineScale, beigeR, beigeG, beigeB, 1.0f);

    // Quit
    const char* t4 = "ESC to Quit";
    AEGfxGetPrintSize(fontId, t4, lineScale, &w, &h);
    AEGfxPrint(fontId, t4, -0.5f * w, -0.28f, lineScale, beigeR, beigeG, beigeB, 1.0f);

    // Save / Load prompts (same style, placed below)
    const char* t5 = "[F5] Save";
    AEGfxGetPrintSize(fontId, t5, lineScale, &w, &h);
    AEGfxPrint(fontId, t5, -0.5f * w, -0.38f, lineScale, beigeR, beigeG, beigeB, 1.0f);

    const char* t6 = "[F9] Load";
    AEGfxGetPrintSize(fontId, t6, lineScale, &w, &h);
    AEGfxPrint(fontId, t6, -0.5f * w, -0.48f, lineScale, beigeR, beigeG, beigeB, 1.0f);
}

void PausePage_Free() {}

// ----------------------------------------------------------------------------
// Unload  -  free the shared mesh
// ----------------------------------------------------------------------------
void PausePage_Unload()
{
    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
    if (AEAudioIsValidAudio(sfxButton))
        AEAudioUnloadAudio(sfxButton);
    AEAudioUnloadAudioGroup(pauseGroup);
}