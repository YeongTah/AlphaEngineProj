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

// --- Variables declaration start here ---
AEGfxTexture* DPLogo = nullptr;      // Digipen logo texture
AEGfxTexture* GameLogo = nullptr;    // (unused, we print text instead)

static int page_index = 0;                // 0 = Digipen Logo, 1 = Game Logo text
static const float PAGE_AUTO_TIME = 2.5f; // auto-advance after 2.5 seconds
static float page_timer = 0.0f;           // time accumulator for current page

// Small delay after pressing skip to prevent accidental double‑skip -ths
static const float SKIP_COOLDOWN = 0.2f;
static float skip_timer = 0.0f;

// --- Variables declaration end here ---

//----------------------------------------------------------------------------
// Loads Intro Screen
//---------------------------------------------------------------------------
void Intro_Load()
{
    std::cout << "Intro:Load\n";

    DPLogo = AEGfxTextureLoad("Assets/DigipenLogo.png");
    // GameLogo texture is not used; we print text manually.

    pMesh = CreateSquareMesh();
}

//----------------------------------------------------------------------------
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

    // Manual skip (spacebar or left mouse button) with cooldown -ths
    if (skip_timer >= SKIP_COOLDOWN)
    {
        if (AEInputCheckTriggered(AEVK_SPACE) || AEInputCheckTriggered(AEVK_LBUTTON))
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

    // Quit game when Q is hit or window closed
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist())
    {
        std::cout << "Intro: Q key released, quitting\n";
        next = GS_QUIT;
    }
}

//----------------------------------------------------------------------------
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

    // Draw skip instruction at bottom
    AEGfxPrint(fontId, "Press SPACE or click to skip", -0.35f, -0.85f, 0.6f, 0.8f, 0.8f, 0.8f, 1.0f);
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data
//---------------------------------------------------------------------------
void Intro_Free()
{
    std::cout << "Intro:Free\n";
}

//----------------------------------------------------------------------------
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