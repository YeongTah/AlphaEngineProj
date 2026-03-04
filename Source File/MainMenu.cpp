
#include "pch.h"

#include "MainMenu.h"
#include "gamestatemanager.h"
#include "Main.h"
#include <iostream>
#include <fstream>

//																--- Variables declaration start here ---

int menuSelection = 0;
// Useful macro to count number of items in an array
#define array_count(a) (sizeof(a)/sizeof(*a))
//static AEGfxVertexList* pMesh = nullptr; // Pointer for the square mesh     
//yt test there change to static 

float button_x;
float playbutton_y;
float instructbutton_y;
float creditbutton_y;
float exitbutton_y;
s32 mouseX;
s32 mouseY;

//																--- Variables declaration end here ---

//----------------------------------------------------------------------------
// Loads Main Menu
//---------------------------------------------------------------------------
void MainMenu_Load()
{
    std::cout << "MainMenu:Load\n"; // Debug purposes
    // Load menu assets

        //																Create a unit square mesh (centered at 0,0)
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
void MainMenu_Initialize()
{
    menuSelection = 0; // Reset to "Play"
    std::cout << "MainMenu:Initialize\n"; // Debug purposes

    button_x = 0.0f;
    playbutton_y = 100.0f;
    instructbutton_y = -25.0f;
    creditbutton_y = -150.0f;
    exitbutton_y = -275.0f;

}

//----------------------------------------------------------------------------
// Updates main menu navigation
// ---------------------------------------------------------------------------
void MainMenu_Update()
{

    //std::cout << "MainMenu:Update\n"; // Debug purposes yt 25-2 comment up first, my computer cannot stand D:

    // call mouse position
    AEInputGetCursorPosition(&mouseX, &mouseY);
    std::cout << "mouse X pos:" << mouseX << "\n"; // Debug purposes 
    std::cout << "mouse Y pos:" << mouseY << "\n"; // Debug purposes 

    //                      ===== IGNORE THESE COMMENTED OUT SECTION, NEED FIX THIS BEFORE BEING ABLE TO CLICK THE BOXES =====
    //void TransformsScreentoWorld(s32 &mouseX, s32 &mouseY);
        // do translation from screen to world (use function that returns the window width and height 	AEGfxGetWindowWidth (), 	AEGfxGetWindowHeight ()
        //AEMtx33Trans(&mouseX, AEGfxGetWindowWidth()/2, AEGfxGetWindowHeight()/2);

        //^ need to ownself calculate the translation not use the function

    //        // Setting of camera view - Sharon
    //    float camera_target = player_pos_y - 690.0f; // Camera will always move to this coordinate
    //// Checks magnitude
    //if (fabsf(camera_y - camera_target) > 0.1f)
    //{
    //    // Camera exists in cartesian coordinates - y upwards, x right
    //    // Game exists in screen coordinates - y downwards, x right
    //    // This negative flips the coordinate system
    //    if (camera_y < camera_target)
    //    {
    //        camera_y += camera_speed * dt;
    //        if (camera_y > camera_target) camera_y = camera_target;
    //    }
    //    else if (camera_y > camera_target)
    //    {
    //        camera_y -= camera_speed * dt;
    //        if (camera_y < camera_target) camera_y = camera_target;
    //    }
    //}
    //else
    //{
    //    camera_y = camera_target;
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
    //std::cout << "MainMenu:Draw\n"; // Debug purposes  yt 25-2 comment up first, my computer cannot stand D:

    //                                      CREATING SHAPES FOR EACH BUTTONS
    // Array for buttons
    AEMtx33 buttons[4] = { 0 };

    //                                      SET SIZES AND POSITIONS OF BUTTON

    // "Play" button
    AEMtx33 button_scale;
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33 button_tran;
    AEMtx33Trans(&button_tran, button_x, playbutton_y);
    AEMtx33Concat(&buttons[0], &button_tran, &button_scale);

    // "Instructions" button
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, button_x, instructbutton_y);
    AEMtx33Concat(&buttons[1], &button_tran, &button_scale);

    // "Credits" button
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, button_x, creditbutton_y);
    AEMtx33Concat(&buttons[2], &button_tran, &button_scale);

    // "Exit button"
    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, button_x, exitbutton_y);
    AEMtx33Concat(&buttons[3], &button_tran, &button_scale);
    
    //                                      START OF RENDERING HERE
    
    // Set the background to black.
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Tell the engine to get ready to draw something with texture.
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Set the the color to multiply to white, so that the sprite can 
    // display the full range of colors (default is black).
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.0f);

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
            AEGfxSetColorToAdd(1.0f, 0.0f, 0.0f, 1.0f); // Orange
        }

        if (2 == i) { // "Credits" 
            AEGfxSetColorToAdd(0.0f, 0.5f, 0.0f, 1.0f); // Green
        }

        if (3 == i) { // "Exit" 
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.5f, 1.0f); // Blue
        }

        // Tell Alpha Engine to use the matrix in 'transform' to apply onto all
        // the vertices of the mesh that we are about to choose to draw in the next line.
        AEGfxSetTransform(buttons[i].m);

        // Tell Alpha Engine to draw the mesh with the above settings.
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

        // Draw text
        AEGfxPrint(fontId, "PLAY", -0.05f, 0.20f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxPrint(fontId, "INSTRUCTIONS", -0.13f, -0.08f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxPrint(fontId, "CREDITS", -0.081f, -0.36f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxPrint(fontId, "EXIT", -0.045f, -0.63f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

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

    // Unload font
    AEGfxDestroyFont(fontId);

    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}