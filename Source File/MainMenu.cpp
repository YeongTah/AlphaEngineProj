
#include "pch.h"

#include "MainMenu.h"
#include "gamestatemanager.h"
#include "Main.h"
#include <iostream>
#include <fstream>

//																--- Variables declaration start here ---

// Useful macro to count number of items in an array
#define array_count(a) (sizeof(a)/sizeof(*a))

// button variables
float button_x;         // x coordinate for all buttons
float playbutton_y;     // y coordinate for play button
float instructbutton_y; // y coordinate for instructions button
float creditbutton_y;   // y coordinate for credits button
float exitbutton_y;     // y coordinate for exit button
float createbutton_x, createbutton_y; // x and y coordinate for creator button
AEGfxTexture* mainpage = nullptr; //jas added

//																--- Variables declaration end here ---

//----------------------------------------------------------------------------
// Loads Main Menu
//---------------------------------------------------------------------------
void MainMenu_Load()
{
    std::cout << "MainMenu:Load\n"; // Debug purposes
    mainpage = AEGfxTextureLoad("Assets/MainPage.png"); // floor tile texture
    // Load menu assets

    pMesh = CreateSquareMesh();

}

//----------------------------------------------------------------------------
// Sets up the initial state
// ---------------------------------------------------------------------------
void MainMenu_Initialize()
{
    std::cout << "MainMenu:Initialize\n"; // Debug purposes

    // Initialise button positions 
    button_x = 0.0f;
    playbutton_y = 100.0f;
    instructbutton_y = -25.0f;
    creditbutton_y = -150.0f;
    exitbutton_y = -275.0f;
    createbutton_x = 680.0f;
    createbutton_y = -370.0f;

}

//----------------------------------------------------------------------------
// Updates main menu navigation
// ---------------------------------------------------------------------------
void MainMenu_Update()
{

    //std::cout << "MainMenu:Update\n"; // Debug purposes yt 25-2 comment up first, my computer cannot stand D:

    // Get mouse position in world coordinates
    s32 mouseX, mouseY;
    TransformScreentoWorld(mouseX, mouseY);

    // Move to next page
    // Move to level 1 when click on play button
    if (AEInputCheckReleased(AEVK_LBUTTON) && IsAreaClicked(button_x, playbutton_y,
        300.0f, 90.0f, mouseX, mouseY))
    {
        next = LEVELPAGE;
        std::cout << "Left click released\n"; // Debug purposes
        std::cout << "next state: " << next << "\n"; // Debug purposes
    }

                                              // --- ONLY UNCOMMENT THE BELOW IF DONE WITH INSTRUCTIONS AND CREDITS PAGE ---
    // Move to instructions page when click on instructions button
    if (AEInputCheckReleased(AEVK_LBUTTON) && IsAreaClicked(button_x, instructbutton_y,
        300.0f, 90.0f, mouseX, mouseY))
    {
        next = INSTRUCTIONS;
    }

    // Move to credits page when click on credits button
    if (AEInputCheckReleased(AEVK_LBUTTON) && IsAreaClicked(button_x, creditbutton_y,
        300.0f, 90.0f, mouseX, mouseY))
    {
        next = CREDIT;
    }

    // Quit game when Q is hit or when the window is closed
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist() ||
        (AEInputCheckReleased(AEVK_LBUTTON) && IsAreaClicked(button_x, exitbutton_y,
            300.0f, 90.0f, mouseX, mouseY)))
    {
        std::cout << "Q key Released" << '\n'; // Debug purposes
        next = GS_QUIT;
    }

    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist() ||
        (AEInputCheckReleased(AEVK_LBUTTON) && IsAreaClicked(createbutton_x, createbutton_y,
            150.0f, 65.0f, mouseX, mouseY)))
    {
        next = CREATOR;
    }

}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
// ---------------------------------------------------------------------------
void MainMenu_Draw()
{

    //adding of the main page image--
    AEMtx33 scale, trans, transform;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(mainpage, 0, 0);

    // size
    AEMtx33Scale(&scale, 1600.0f, 900.0f);

    //center of screen
    AEMtx33Trans(&trans, 0.0f, 0.0f);

    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);

    AEGfxSetTransparency(1.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES); // main menu image


    //std::cout << "MainMenu:Draw\n"; // Debug purposes  yt 25-2 comment up first, my computer cannot stand D:

    //                                      CREATING SHAPES FOR EACH BUTTONS
    // Array for buttons
    AEMtx33 buttons[5] = { 0 };

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

    // "Creator button"
    AEMtx33Scale(&button_scale, 150.0f, 65.0f);
    AEMtx33Trans(&button_tran, createbutton_x, createbutton_y);
    AEMtx33Concat(&buttons[4], &button_tran, &button_scale);
    
    //                                      START OF RENDERING HERE
    
    // Set the background to Sand.
    AEGfxSetBackgroundColor(0.84f, 0.76f, 0.58f);

    // Tell the engine to get ready to draw something with texture.
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    //// Set the the color to multiply to black (default so can render any colour in)
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.0f);

    // Set blend mode to AE_GFX_BM_BLEND
    // This will allow transparency.
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // For each transform in the array...
    for (int i = 0; i < array_count(buttons); ++i) {

        if (0 == i) { // "Play" 
            AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f); // Red
        }

        if (1 == i) { // "Instruction" 
            AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f);  // Orange
        }

        if (2 == i) { // "Credits" 
            AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f);  // Green
        }

        if (3 == i) { // "Exit" 
            AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f);  // Blue
        }

        if (4 == i) { // "Creator" 
            AEGfxSetColorToAdd(0.37f, 0.14f, 0.14f, 1.0f); // Grey
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
        AEGfxPrint(fontId, "Creator", 0.8f, -0.84f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
        //AEGfxPrint(fontId, "Mummy Game", -0.33f, 0.58f, 2.8f, 0.30f, 0.18f, 0.08f, 1.0f);

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

    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}