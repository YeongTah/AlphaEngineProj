#include "pch.h"
#include "Instructions.h"
#include "main.h"
#include "gamestatemanager.h"
#include <iostream>
#include <fstream>

namespace
{

    struct InstructionsLine
    {
        const char* text;
        float x; //x pos
        float y; //y pos
        float scale; //scaling of the text
    };

    InstructionsLine Instructions[] =
    {
        {"INSTRUCTIONS", -0.18f,  0.75f, 1.55f},
        {"Welcome to Mummy Game!", -0.20f,  0.61f, 1.10f},
        {"Created by Intro Lab", -0.16f,  0.51f, 0.85f},

        {"Objective", -0.40f,  0.31f, 1.00f},
        {"Avoid the enemies and collect all the coins.", -0.40f,  0.20f, 0.78f},
        {"Watch out for mummies chasing you through the level.", -0.40f,  0.11f, 0.78f},
        {"Plan your moves carefully to survive and win.", -0.40f, 0.02f, 0.78f},

        {"Game Features", -0.40f, -0.13f, 1.00f},
        {"Three difficulty levels: Easy, Medium, and Hard.", -0.40f, -0.24f, 0.78f},
        {"Use the Level Editor to create and customize your own levels.", -0.40f, -0.33f, 0.74f},

        {"[Controls]",                       -0.40f, -0.49f, 1.00f},
        {"Move: W A S D",                  -0.40f, -0.59f, 0.82f},
        {"ESC or B: Return to Main Menu",  -0.40f, -0.70f, 0.78f},
        {"Q: Quit Game",                   -0.40f, -0.79f, 0.78f}
    };

    int InstructionsCount = sizeof(Instructions) / sizeof(Instructions[0]);
    AEGfxTexture* wallimage = nullptr;

    // text = the string to display
    // x = horizontal print position
    // y = vertical print position
    // scale = size of the text

    void DrawInstructionsText(const char* text, float x, float y, float scale)
    {
        char strBuffer[256];

        //clear buffer first
        memset(strBuffer, 0, sizeof(strBuffer));

        //copy text into the buffer
        sprintf_s(strBuffer, "%s", text);

        AEGfxPrint(fontId, strBuffer, x, y, scale, 1.0f, 1.0f, 1.0f, 1.0f); //white
    }
}

//----------------------------------------------------------------------------
// Loads Instructions
//----------------------------------------------------------------------------
void Instructions_Load()
{
    std::cout << "Instructions:Load\n";
    wallimage = AEGfxTextureLoad("Assets/Bigwall.png"); // floor tile texture
    pMesh = CreateSquareMesh();
}

//----------------------------------------------------------------------------
// Sets up the initial state
//----------------------------------------------------------------------------
void Instructions_Initialize()
{
    std::cout << "Instructions:Initialize\n";

}

//----------------------------------------------------------------------------
// Updates Level Selection Page navigation
//----------------------------------------------------------------------------
void Instructions_Update()
{
    std::cout << "Instructions:Update\n";

    // Move back to main menu upon triggering "B"
    if (AEInputCheckReleased(AEVK_B) || AEInputCheckReleased(AEVK_ESCAPE))
    {
        next = MAINMENUSTATE;
        std::cout << "Back key Released" << '\n';
    }

    // Quit game when Q is hit or when the window is closed
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
        std::cout << "Q key Released" << '\n';
    }

}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
//----------------------------------------------------------------------------
void Instructions_Draw()
{
    std::cout << "Instructions:Draw\n";

    //adding of the main page image--
    AEMtx33 scale, trans, transform;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(wallimage, 0, 0);

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

    //rendering mode to colour mode
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    for (int i = 0; i < InstructionsCount; ++i)
    {
        DrawInstructionsText(
            Instructions[i].text, Instructions[i].x, Instructions[i].y, Instructions[i].scale); //loops throught the array of InstructionsLine structs
    }
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
//----------------------------------------------------------------------------
void Instructions_Free()
{
    std::cout << "Instructions:Free\n";
}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
//----------------------------------------------------------------------------
void Instructions_Unload()
{
    std::cout << "Instructions:Unload\n";
    AEGfxTextureUnload(wallimage);
}