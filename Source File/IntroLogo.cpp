/* Start Header ***************************************************************
\file       IntroLogo.cpp
\coders     Sharon, San
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#include "pch.h"

#include "IntroLogo.h"
#include "gamestatemanager.h"
#include "Main.h"

//                                                                --- VARIABLES DECLARATION START HERE ---
AEGfxTexture* DPLogo = nullptr;      // Digipen logo texture

static int page_index = 0;                // 0 = Digipen Logo, 1 = Game Logo text
static const float PAGE_MIN_TIME = 2.0f;  // prevent instantaneous skip 
static const float PAGE_AUTO_TIME = 3.0f; // auto-advance after 3 seconds
static float page_timer = 0.0f;           // time accumulator for current page

// Small delay after pressing skip to prevent accidental double‑skip
static const float SKIP_COOLDOWN = 0.2f;
static float skip_timer = 0.0f;

//                                                                --- VARIABLES DECLARATION END HERE ---

//----------------------------------------------------------------------------
// Intro_load
// Loads the DigiPen logo texture from Assets/ and creates the shared pMesh
// unit square used for rendering. Called once before the game loop begins.
//---------------------------------------------------------------------------
void Intro_Load()
{

    DPLogo = AEGfxTextureLoad("Assets/DigipenLogo.png");

    pMesh = CreateSquareMesh();
}

//----------------------------------------------------------------------------
// Intro_Intialize
// Resets the intro state by setting page_index to 0 (DigiPen logo page) and
// zeroing both the page timer and skip cooldown timer. Called on every entry
// into the intro state to ensure a clean start.
//---------------------------------------------------------------------------
void Intro_Initialize()
{

    page_index = 0;
    page_timer = 0.0f;
    skip_timer = 0.0f;
}

//----------------------------------------------------------------------------
// Intro_Update
// Advances the intro sequence each frame. Accumulates delta time into
// page_timer and skip_timer. Auto-advances to the next page after
// PAGE_AUTO_TIME seconds, or immediately on spacebar or left mouse click once
// the skip cooldown has elapsed. Page 0 transitions to page 1 (game title),
// and page 1 transitions to MAINMENUSTATE. ESC or window close quits the game.
//---------------------------------------------------------------------------
void Intro_Update()
{
    // Add delta time to timers
    float dt = (float)AEFrameRateControllerGetFrameTime();
    page_timer += dt;
    skip_timer += dt;

    // Handle advance to next page / main menu
    bool advanceRequested = false;

    // Auto‑advance if page timer exceeds limit
    if (page_timer >= PAGE_AUTO_TIME)
    {
        advanceRequested = true;
    }

    // Only allow skip if we've met the minimum time requirement for the current page
    bool canSkip = false;
    // Can only skip after PAGE_MIN_TIME (2.0f seconds)
    canSkip = (page_timer >= PAGE_MIN_TIME);

    // Manual skip (spacebar or left mouse button) with cooldown
    if (canSkip && skip_timer >= SKIP_COOLDOWN)
    {
        if (AEInputCheckReleased(AEVK_SPACE) || AEInputCheckReleased(AEVK_LBUTTON))
        {
            advanceRequested = true;
            skip_timer = 0.0f; // reset cooldown
        }
    }

    if (advanceRequested)
    {

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
        }
    }

    // Quit game when ESC is hit or window closed
    if (AEInputCheckReleased(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
    }
}

//----------------------------------------------------------------------------
// Intro_Draw
// Renders the current intro page each frame against a black background.
// Page 0 displays the DigiPen logo as a full-screen texture.
// Page 1 displays the game title text centered on screen.
// Both pages display a semi-transparent skip instruction at the top of screen.
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
// Called after the game loop exits this state, before Unload.
// Currently empty as the intro state has no heap allocations or runtime
// resources that need releasing between Free and Unload. Still needs to be
// here or there would be linker error / null function pointer crash
//---------------------------------------------------------------------------
void Intro_Free() {}

//----------------------------------------------------------------------------
// Intro_Unload
// Unloads the DigiPen logo texture and frees the shared pMesh to prevent
// memory leaks. Sets pMesh to nullptr after freeing to prevent dangling
// references. Called when permanently leaving the intro state.
//---------------------------------------------------------------------------
void Intro_Unload()
{

    AEGfxTextureUnload(DPLogo);

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}