/* Start Header ************************************************************************/
/*!
\file   IntroLogo.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  This file defines the intro sequence (Digipen logo and game title).
        Extended with reliable transition handling. -ths
Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"

#include "IntroLogo.h"
#include "gamestatemanager.h"
#include "Main.h"
#include <iostream>
#include <fstream>

//                                                                --- VARIABLES DECLARATION START HERE ---
AEGfxTexture* DPLogo = nullptr;      // Digipen logo texture

static int page_index = 0;                // 0 = Digipen Logo, 1 = Game Logo text
static const float PAGE_MIN_TIME = 0.0f;  // prevent instantaneous skip 
static const float PAGE_AUTO_TIME = 3.0f; // auto-advance after 3 seconds
static float page_timer = 0.0f;           // time accumulator for current page

// Small delay after pressing skip to prevent accidental double‑skip
static const float SKIP_COOLDOWN = 0.2f;
static float skip_timer = 0.0f;

//                                                                --- VARIABLES DECLARATION END HERE ---

//----------------------------------------------------------------------------
// Intro_load
// Loads Intro Screen
//---------------------------------------------------------------------------
void Intro_Load()
{
    std::cout << "Intro:Load\n";

    DPLogo = AEGfxTextureLoad("Assets/DigipenLogo.png");

    pMesh = CreateSquareMesh();
}

//----------------------------------------------------------------------------
// Intro_Intialize
// Sets up the initial state
//---------------------------------------------------------------------------
void Intro_Initialize()
{
    std::cout << "Intro:Initialize\n";

    page_index = 0;
    page_timer = 0.0f;
    skip_timer = 0.0f;   // -ths
}

//----------------------------------------------------------------------------
// Intro_Update
// Updates intro screen navigation
//---------------------------------------------------------------------------
void Intro_Update()
{
    // Add delta time to timers
    float dt = (float)AEFrameRateControllerGetFrameTime();
    page_timer += dt;
    skip_timer += dt;   // -ths

    // --- Handle advance to next page / main menu ---
    bool advanceRequested = false;

    // Auto‑advance if page timer exceeds limit
    if (page_timer >= PAGE_AUTO_TIME)
    {
        advanceRequested = true;
    }

    // Only allow skip if we've met the minimum time requirement for the current page
    bool canSkip = false;
    // Can only skip after PAGE_MIN_TIME(2.0f seconds)
    canSkip = (page_timer >= PAGE_MIN_TIME);

    // Manual skip (spacebar or left mouse button) with cooldown -ths
    if (canSkip && skip_timer >= SKIP_COOLDOWN)
    {
        if (AEInputCheckReleased(AEVK_SPACE) || AEInputCheckReleased(AEVK_LBUTTON))
        {
            advanceRequested = true;
            skip_timer = 0.0f;   // reset cooldown
        }
    }

    if (advanceRequested)
    {
        std::cout << "Intro: advance requested, page_index=" << page_index << "\n";

        if (page_index == 0)
        {
            // Move to game title page
            page_index = 1;
            page_timer = 0.0f;          // reset timer for next page
        }
        else if (page_index == 1)
        {
            // Finished intro, go to main menu
            next = MAINMENUSTATE;
            std::cout << "Intro: transitioning to MAINMENU\n";
        }
    }

    // Quit game when ESC is hit or window closed
    if (AEInputCheckReleased(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
    {
        std::cout << "Intro: Q key released, quitting\n";
        next = GS_QUIT;
    }
}

//----------------------------------------------------------------------------
// Intro_Draw
// Renders or draws the visual representation each frame
//---------------------------------------------------------------------------
void Intro_Draw()
{
    // Set background to black
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    AEMtx33 scale, trans, transform;

    if (page_index == 0)
    {
        // --- Draw Digipen logo (full screen texture) ---
        if (DPLogo)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxTextureSet(DPLogo, 0, 0);

            AEMtx33Scale(&scale, 1600.0f, 900.0f);
            AEMtx33Trans(&trans, 0.0f, 0.0f);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
    else if (page_index == 1)
    {
        // --- Draw game title text ---
        // Clear screen to black (already set)
        // Draw "MUMMY GAME" in large white letters
        const char* title = "MUMMY GAME";
        float w, h;
        const float scaleTitle = 4.0f;
        AEGfxGetPrintSize(fontId, title, scaleTitle, &w, &h);
        AEGfxPrint(fontId, title, -0.5f * w, 0.0f, scaleTitle, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Draw skip instruction at top

    const float titleScale = 0.8f;

    float w = 0.0f, h = 0.0f;

    const char* t0 = "Press SPACE or click to skip";
    AEGfxGetPrintSize(fontId, t0, titleScale, &w, &h);
    float x0 = -0.5f * w;
    AEGfxPrint(fontId, t0, x0, 0.9f, titleScale, 1.0f, 1.0f, 1.0f, 0.5f);
}

//----------------------------------------------------------------------------
// Intro_Free
// Cleans up dynamic resources while keeping static data
//---------------------------------------------------------------------------
void Intro_Free() {}

//----------------------------------------------------------------------------
// Intro_Unload
// Unloads all resources completely when exiting the level
//---------------------------------------------------------------------------
void Intro_Unload()
{
    std::cout << "Intro:Unload\n";

    AEGfxTextureUnload(DPLogo);

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}