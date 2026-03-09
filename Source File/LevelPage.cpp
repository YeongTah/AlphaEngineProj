#include "pch.h"
#include "LevelPage.h"
#include "main.h"
#include "gamestatemanager.h"
#include <iostream>
#include <fstream>

// Macro: returns the number of elements in a stack-allocated array
#define array_count(a) (sizeof(a)/sizeof(*a))

// ---- UI layout variables ----
// Positions are in world space (center of each button rectangle).
float select_y;             // Shared Y position for all 3 difficulty buttons
float easy_x;               // X center of the "Level 1 / Easy" button
float medium_x;             // X center of the "Level 2 / Normal" button
float hard_x;               // X center of the "Level 3 / Hard" button
float backbutton_x, backbutton_y; // X, Y center of the Back button (top-left area)

AEGfxTexture* backButton = nullptr; // Texture loaded from Assets/Backbutton.png


// ----------------------------------------------------------------------------
// LevelPage_Load
// Called once when entering the Level Select page.
// Loads the back button texture and creates the shared square mesh.
// ----------------------------------------------------------------------------
void LevelPage_Load()
{
    std::cout << "LevelPage:Load\n";

    backButton = AEGfxTextureLoad("Assets/Backbutton.png"); // back arrow texture
    pMesh = CreateSquareMesh();                        // unit square mesh
}

// ----------------------------------------------------------------------------
// LevelPage_Initialize
// Sets the initial positions for all buttons on the Level Select page.
// The 3 difficulty buttons are horizontally spread across the screen center
// (Y = 0), and the back button is placed in the upper-left corner.
// ----------------------------------------------------------------------------
void LevelPage_Initialize()
{
    std::cout << "LevelPage:Initialize\n";

    select_y = 0.0f;  // All 3 level buttons share this Y
    easy_x = -400.0f;  // Left side   -- Level 1 (Easy)
    medium_x = 0.0f;  // Center      -- Level 2 (Normal)
    hard_x = 400.0f;  // Right side  -- Level 3 (Hard)
    backbutton_x = -750.0f; // Far left
    backbutton_y = 400.0f; // Near top
}

// ----------------------------------------------------------------------------
// LevelPage_Update
// Handles input for the Level Select page each frame.
//
// Click detection:
//   - "Easy"   button (easy_x,   select_y, 200x400): sets next = GS_LEVEL1
//   - "Normal" button (medium_x, select_y, 200x400): sets next = GS_LEVEL2
//   - "Hard"   button (hard_x,   select_y, 200x400): sets next = GS_LEVEL3
//   - "Back"   button OR B key OR ESCAPE: returns to MAINMENUSTATE
//   - Q key or window close: quit game
//
// IsAreaClicked() takes the button's world-space center and half-extents and
// compares them to the current mouse position in world space (via TransformScreentoWorld).
// ----------------------------------------------------------------------------
void LevelPage_Update()
{
    // Convert mouse screen position to world coordinates for click testing
    s32 mouseX, mouseY;
    TransformScreentoWorld(mouseX, mouseY);

    // AEInputCheckCurr keeps the left-button state updated for IsAreaClicked to work reliably
    AEInputCheckCurr(AEVK_LBUTTON);

    // --- Level 1 (Easy) button ---
    if (AEInputCheckReleased(AEVK_LBUTTON) && IsAreaClicked(easy_x, select_y,
        200.0f, 400.0f, mouseX, mouseY))
    {
        next = GS_LEVEL1;
        std::cout << "lvl 1 Left click released\n";
    }

    // --- Level 2 (Normal) button ---
    if (AEInputCheckReleased(AEVK_LBUTTON) && IsAreaClicked(medium_x, select_y,
        200.0f, 400.0f, mouseX, mouseY))
    {
        next = GS_LEVEL2;
        std::cout << "lvl 2 Left click released\n";
    }

    // --- Level 3 (Hard) button ---
    if (AEInputCheckReleased(AEVK_LBUTTON) && IsAreaClicked(hard_x, select_y,
        200.0f, 400.0f, mouseX, mouseY))
    {
        next = GS_LEVEL3;
        std::cout << "lvl 3 Left click released\n";
    }

    // --- Back button: B key, ESCAPE, or clicking the back button texture ---
    if (AEInputCheckReleased(AEVK_B) || (AEInputCheckReleased(AEVK_ESCAPE)) ||
        (AEInputCheckReleased(AEVK_LBUTTON) && IsAreaClicked(backbutton_x, backbutton_y,
            50.0f, 50.0f, mouseX, mouseY)))
    {
        next = MAINMENUSTATE;
        std::cout << "Back key released\n";
    }

    // --- Quit ---
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
        std::cout << "Q key released\n";
    }
}

// ----------------------------------------------------------------------------
// LevelPage_Draw
// Renders the Level Select page each frame.
//
// Drawing order:
//   1. Sets background to purple.
//   2. Draws the 3 level buttons as solid colored rectangles:
//        [0] Easy   = green
//        [1] Normal = blue
//        [2] Hard   = red
//   3. Draws the Back button as a texture (Backbutton.png).
//   4. Prints text labels ("LEVEL 1 / EASY", "LEVEL 2 / NORMAL", "LEVEL 3 / HARD")
//      centered on each button using AEGfxPrint with hardcoded NDC offsets.
// ----------------------------------------------------------------------------
void LevelPage_Draw()
{
    // Array of 4 transform matrices: indices 0-2 = difficulty buttons, 3 = back button
    AEMtx33 Selection[4] = { 0 };

    AEMtx33 button_scale, button_tran;

    // Build transform for Easy button (200x400 at easy_x, select_y)
    AEMtx33Scale(&button_scale, 200.f, 400.f);
    AEMtx33Trans(&button_tran, easy_x, select_y);
    AEMtx33Concat(&Selection[0], &button_tran, &button_scale);

    // Build transform for Normal button
    AEMtx33Scale(&button_scale, 200.f, 400.f);
    AEMtx33Trans(&button_tran, medium_x, select_y);
    AEMtx33Concat(&Selection[1], &button_tran, &button_scale);

    // Build transform for Hard button
    AEMtx33Scale(&button_scale, 200.f, 400.f);
    AEMtx33Trans(&button_tran, hard_x, select_y);
    AEMtx33Concat(&Selection[2], &button_tran, &button_scale);

    // Build transform for Back button (50x50 texture)
    AEMtx33Scale(&button_scale, 50.0f, 50.0f);
    AEMtx33Trans(&button_tran, backbutton_x, backbutton_y);
    AEMtx33Concat(&Selection[3], &button_tran, &button_scale);

    // Set background color (purple -- clamped to 0-1 range by engine)
    AEGfxSetBackgroundColor(0.50f, 0.44f, 0.30f);
    //AEGfxSetBackgroundColor(0.21f, 0.11f, 0.12f);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    for (int i = 0; i < array_count(Selection); ++i)
    {
        if (i == 0) AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f);  // Green = Easy
        if (i == 1) AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f);  // Blue = Normal
        if (i == 2) AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f); // Red   = Hard

        if (i == 3) // Back button: switch to texture mode for Backbutton.png
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // full white so texture colors show
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxTextureSet(backButton, 0, 0);
        }

        AEGfxSetTransform(Selection[i].m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- Text labels on each button (NDC coordinates hand-tuned) ---
    if (fontId >= 0)
    {
        AEGfxPrint(fontId, "LEVEL 1", -0.57f, 0.05f, 1.0f, 1, 1, 1, 1);
        AEGfxPrint(fontId, "EASY", -0.55f, -0.05f, 1.0f, 1, 1, 1, 1);

        AEGfxPrint(fontId, "LEVEL 2", -0.07f, 0.05f, 1.0f, 1, 1, 1, 1);
        AEGfxPrint(fontId, "NORMAL", -0.08f, -0.05f, 1.0f, 1, 1, 1, 1);

        AEGfxPrint(fontId, "LEVEL 3", 0.43f, 0.05f, 1.0f, 1, 1, 1, 1);
        AEGfxPrint(fontId, "HARD", 0.44f, -0.05f, 1.0f, 1, 1, 1, 1);
    }
}

// ----------------------------------------------------------------------------
// LevelPage_Free -- currently empty; no heap allocations to release here.
// ----------------------------------------------------------------------------
void LevelPage_Free()
{
    std::cout << "LevelPage:Free\n";
}

// ----------------------------------------------------------------------------
// LevelPage_Unload
// Unloads the back button texture and frees the shared mesh.
// ----------------------------------------------------------------------------
void LevelPage_Unload()
{
    std::cout << "LevelPage:Unload\n";

    AEGfxTextureUnload(backButton);

    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}