
#include "pch.h"

#include "LevelPage.h"
#include "gamestatemanager.h"
#include <iostream>
#include <fstream>

// Useful macro to count number of items in an array
#define array_count(a) (sizeof(a)/sizeof(*a))

//----------------------------------------------------------------------------
// Loads Main Menu
//---------------------------------------------------------------------------
void LevelPage_Load()
{
    std::cout << "LevelPage:Load\n"; // Debug purposes
    // TO ADD: Load level selection options, not needed if just drawing the boxes

        //                                      CREATING SHAPES FOR EACH BUTTONS
    // Create a unit square mesh (centered at 0,0)
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0x00FFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0x00FFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0x00FFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0x00FFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0x00FFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0x00FFFFFF, 0.0f, 0.0f);
    pMesh = AEGfxMeshEnd();

}

//----------------------------------------------------------------------------
// Sets up the initial state
// ---------------------------------------------------------------------------
void LevelPage_Initialize()
{
    std::cout << "LevelPage:Initialize\n"; // Debug purposes
}

//----------------------------------------------------------------------------
// Updates Level Selection Page navigation
// ---------------------------------------------------------------------------
void LevelPage_Update()
{

    std::cout << "LevelPage:Update\n"; // Debug purposes

    // Handle level selection

    // If "easy" difficulty is selected, move to level 1
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        next = GS_LEVEL1;

        std::cout << "Left click triggered\n"; // Debug purposes
    }

    // If "normal" difficulty is selected, move to level 2
    if (AEInputCheckTriggered(AEVK_LBUTTON)) 
    {
        next = GS_LEVEL2;

        std::cout << "Left click triggered\n"; // Debug purposes
    }

    // If "hard" difficulty is selected, move to level 3
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        next = GS_LEVEL3;
        std::cout << "Left click triggered\n"; // Debug purposes
    }

    // Move back to main menu upon triggering "B"
    if (AEInputCheckTriggered(AEVK_B))
    {
        next = MAINMENUSTATE;
        std::cout << "B key triggered" << '\n'; // Debug purposes
    }

    // Quit game when ESCAPE is hit or when the window is closed
    if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
        std::cout << "escape key triggered" << '\n'; // Debug purposes
    }

}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
// ---------------------------------------------------------------------------
void LevelPage_Draw()
{
    std::cout << "LevelPage:Draw\n"; // Debug purposes

    // Array for level selection button
    AEMtx33 Selection[3] = { 0 };

    //                                      SET SIZES AND POSITIONS OF SELECTION

    // "Easy" difficulty button
    AEMtx33 button_scale;
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33 button_tran;
    AEMtx33Trans(&button_tran, -400.f, 250.f);
    AEMtx33Concat(&Selection[0], &button_tran, &button_scale);

    // "Normal" difficulty button
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, 0.f, 250.f);
    AEMtx33Concat(&Selection[1], &button_tran, &button_scale);

    // "Hard" difficulty button
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, 400.f, 250.f);
    AEMtx33Concat(&Selection[2], &button_tran, &button_scale);

    //                                      START OF RENDERING HERE

    // Set the background to purple.
    AEGfxSetBackgroundColor(0.0f, 150.0f,150.0f);

    // Tell the engine to get ready to draw something with texture.
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // Set the the color to multiply to white, so that the sprite can 
    // display the full range of colors (default is black).
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    // Set blend mode to AE_GFX_BM_BLEND
    // This will allow transparency.
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // Set the color to add to nothing, so that we don't alter the sprite's color
    AEGfxSetColorToAdd(255.0f, 0.0f, 0.0f, 0.0f);

    // For each transform in the array...
    for (int i = 0; i < array_count(Selection); ++i) {

        if (0 == i) { // "Easy" 
            AEGfxSetColorToAdd(0.0f, 0.5f, 0.0f, 1.0f); // Green
        }

        if (1 == i) { // "Normal" 
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.5f, 1.0f); // Blue
        }

        if (2 == i) { // "Hard" 
            AEGfxSetColorToAdd(0.5f, 0.0f, 0.0f, 1.0f); // Red
        }

        // Tell Alpha Engine to use the matrix in 'transform' to apply onto all
        // the vertices of the mesh that we are about to choose to draw in the next line.
        AEGfxSetTransform(Selection[i].m);

        // Tell Alpha Engine to draw the mesh with the above settings.
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    }
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
// ---------------------------------------------------------------------------
void LevelPage_Free()
{
    std::cout << "LevelPage:Free\n"; // Debug purposes
}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
// ---------------------------------------------------------------------------
void LevelPage_Unload()
{
    std::cout << "LevelPage:Unload\n"; // Debug purposes
}