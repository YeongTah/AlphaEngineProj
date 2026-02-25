
#include "pch.h"

#include "MainMenu.h"
#include "gamestatemanager.h"
#include <iostream>
#include <fstream>

int menuSelection = 0;
// Useful macro to count number of items in an array
#define array_count(a) (sizeof(a)/sizeof(*a))
AEGfxVertexList* pMesh = nullptr; // Pointer for the square mesh

//----------------------------------------------------------------------------
// Loads Main Menu
//---------------------------------------------------------------------------
void MainMenu_Load()
{
    std::cout << "MainMenu:Load\n"; // Debug purposes
    // Load menu assets
    // LoadTexture("menu_bg.png");
    // LoadTexture("menu_cursor.png");

}

//----------------------------------------------------------------------------
// Sets up the initial state
// ---------------------------------------------------------------------------
void MainMenu_Initialize()
{
    menuSelection = 0; // Reset to "Play"
    std::cout << "MainMenu:Initialize\n"; // Debug purposes
}

//----------------------------------------------------------------------------
// Updates main menu navigation
// ---------------------------------------------------------------------------
void MainMenu_Update()
{

    std::cout << "MainMenu:Update\n"; // Debug purposes

    // Handle menu navigation
    //// Press W or UP to go to above button (need the also ensure that it goes back to bottom button after the top of the selections)
    //if (AEInputCheckCurr(AEVK_W) || AEInputCheckCurr(AEVK_UP))
    //{
    //    menuSelection--;
    //    std::cout << "W pressed - moving selection up\n"; // Debug purposes
    //}

    //// Press S or Down to go to below button (need the also ensure that it goes back to top button after the end of the selections)
    //if (AEInputCheckCurr(AEVK_S) || AEInputCheckCurr(AEVK_DOWN))
    //{
    //    menuSelection++;
    //    std::cout << "S pressed - moving selection down\n"; // Debug purposes
    //}



    // Move to next page
    // Trigger right mouse to go to next state
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        //if (menuSelection == 0) 
            next = GS_LEVEL1;
       // else if (menuSelection == 3) next = GS_QUIT;
        std::cout << "Right click triggered\n"; // Debug purposes
    }

    // Quit game when ESCAPE is hit or when the window is closed
    if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
    {
        std::cout << "escape key triggered" << '\n'; // Debug purposes
        next = GS_QUIT;
    }

}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
// ---------------------------------------------------------------------------
void MainMenu_Draw()
{
    std::cout << "MainMenu:Draw\n"; // Debug purposes

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

    // Array for buttons
    AEMtx33 buttons[4] = { 0 };

    //                                      SET SIZES AND POSITIONS OF BUTTON
    
    // "Play" button
    AEMtx33 button_scale;
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33 button_tran;
    AEMtx33Trans(&button_tran, 0.f, 100.f);
    AEMtx33Concat(&buttons[0], &button_tran, &button_scale);

    // "Instructions" button
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, 0.f, -25.f);
    AEMtx33Concat(&buttons[1], &button_tran, &button_scale);

    // "Credits" button
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, 0.f, -150.f);
    AEMtx33Concat(&buttons[2], &button_tran, &button_scale);

    // "Exit button"
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, 0.f, -275.f);
    AEMtx33Concat(&buttons[3], &button_tran, &button_scale);
    
    //                                      START OF RENDERING HERE
    
    // Set the background to black.
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

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
    for (int i = 0; i < array_count(buttons); ++i) {

        if (0 == i) { // "Play" 
            AEGfxSetColorToAdd(0.5f, 0.0f, 0.0f, 1.0f); // Red
        }

        if (1 == i) { // "Instruction" 
            AEGfxSetColorToAdd(1.0f, 0.0f, 0.0f, 1.0f); // Red
        }

        if (2 == i) { // "Credits" 
            AEGfxSetColorToAdd(0.0f, 0.5f, 0.0f, 1.0f); 
        }

        if (3 == i) { // "Exit" 
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.5f, 1.0f); 
        }

        // Tell Alpha Engine to use the matrix in 'transform' to apply onto all
        // the vertices of the mesh that we are about to choose to draw in the next line.
        AEGfxSetTransform(buttons[i].m);

        // Tell Alpha Engine to draw the mesh with the above settings.
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    }
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
// ---------------------------------------------------------------------------
void MainMenu_Free()
{
    std::cout << "MainMenu:Free\n"; // Debug purposes
}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
// ---------------------------------------------------------------------------
void MainMenu_Unload()
{
    std::cout << "MainMenu:Unload\n"; // Debug purposes
}