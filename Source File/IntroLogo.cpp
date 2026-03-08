
#include "pch.h"

#include "IntroLogo.h"
#include "gamestatemanager.h"
#include "Main.h"
#include <iostream>
#include <fstream>

//																--- Variables declaration start here ---

AEGfxTexture* DPLogo = nullptr;
AEGfxTexture* GameLogo = nullptr;

static int page_index = 0;                // current slide index: 0 = Digipen Logo, 1 = Game Logo
static const float PAGE_AUTO_TIME = 2.5f; // auto-advance after 2.5s
static float page_timer = 0.0f;           // timer for image

//																--- Variables declaration end here ---

//----------------------------------------------------------------------------
// Loads Intro Screen
//---------------------------------------------------------------------------
void Intro_Load()
{
    std::cout << "Intro:Load\n"; // Debug purposes

    DPLogo = AEGfxTextureLoad("Assets/DigipenLogo.png");
    GameLogo = AEGfxTextureLoad("Assets/GameLogo.png");

    pMesh = CreateSquareMesh();

}

//----------------------------------------------------------------------------
// Sets up the initial state
// ---------------------------------------------------------------------------
void Intro_Initialize()
{
    std::cout << "Intro:Initialize\n"; // Debug purposes

    page_index = 0;
    page_timer = 0;

}

//----------------------------------------------------------------------------
// Updates intro screen navigation
// ---------------------------------------------------------------------------
void Intro_Update()
{

    //std::cout << "Intro:Update\n"; // Debug purposes yt 25-2 comment up first, my computer cannot stand D:

    // Add delta time to timer to ensure it increases in real time instead of per frame
    page_timer += AEFrameRateControllerGetFrameTime();

    //                                          === Handle appearance logic for digipen logo and game logo ===

    // Move to show game logo if...
    if ((page_timer >= PAGE_AUTO_TIME) ||      // if its more than auto advance time
        (AEInputCheckReleased(AEVK_LBUTTON) || // if click left mouse button
            AEInputCheckReleased(AEVK_SPACE))) // if click on space bar
    {
        std::cout << "next page released\n"; // Debug purposes
        if (page_index == 0) {
            page_index++; // go to game logo
            page_timer = 0.0f; // reset timer
        } else if (page_index == 1) {
            // finish showing intro, move to main menu
            next = MAINMENUSTATE;
        }
    }


    // Quit game when Q is hit or when the window is closed
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist())
    {
        std::cout << "Q key Released" << '\n'; // Debug purposes
        next = GS_QUIT;
    }

}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
// ---------------------------------------------------------------------------
void Intro_Draw()
{
    std::cout << "Intro:Draw\n"; // Debug purposes  yt 25-2 comment up first, my computer cannot stand D:

    //                                      SET SIZES AND POSITIONS OF LOGOS
    AEMtx33 scale, trans, transform;
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);

    //                                      START OF RENDERING HERE

    // Set the background to black.
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    // Render images (Texture Mode)
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    // Set the the color to multiply to white so it doesnt interfere with texture
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    // Set blend mode to AE_GFX_BM_BLEND
    // This will allow transparency.
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    
    // Draw logo based on the page index and if image exist
    if (page_index == 0) {
        if (DPLogo) {
            
            AEGfxTextureSet(DPLogo, 0, 0);
            AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
    else if (page_index == 1) {
        if (GameLogo) {
            AEGfxTextureSet(GameLogo, 0, 0);
            AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // Reset transform for text
    AEMtx33Identity(&transform);
    AEGfxSetTransform(transform.m);

    // Draw text for controls
    AEGfxPrint(fontId, "Press spacebar or click anywhere on the screen to skip", 0.5f, 0.8f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
// ---------------------------------------------------------------------------
void Intro_Free()
{
    std::cout << "Intro:Free\n"; // Debug purposes
}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
// ---------------------------------------------------------------------------
void Intro_Unload()
{
    std::cout << "Intro:Unload\n"; // Debug purposes

    AEGfxTextureUnload(DPLogo);
    AEGfxTextureUnload(GameLogo);

    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}