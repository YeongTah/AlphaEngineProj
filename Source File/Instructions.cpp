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
        {"Welcome to Mummy Game!", -0.25f,  0.61f, 1.10f},
        {"By Intro Lab", -0.10f,  0.51f, 0.85f},

        {"Goal", -0.25f,  0.31f, 1.00f},
        {"Collect all coins. Avoid enemies and mummies.",  -0.25f,  0.20f, 0.78f},
        {"Survive to win.",  -0.25f,  0.11f, 0.78f},
        //{"Survive to win.",  -0.20f, 0.02f, 0.78f},

        {"Features",  -0.25f, -0.10f, 1.00f},
        {"Easy / Medium / Hard Levels",  -0.25f, -0.21f, 0.78f},
        {"Level Editor to customise",  -0.25f, -0.30f, 0.74f},

        {"[Controls]",  -0.25f, -0.49f, 1.00f},
        {"W A S D: Move",  -0.25f, -0.59f, 0.82f},
        {"ESC / B: Menu",  -0.25f, -0.70f, 0.78f},
        {"Q: Quit",  -0.25f, -0.79f, 0.78f},
    };

    int InstructionsCount = sizeof(Instructions) / sizeof(Instructions[0]);
    AEGfxTexture* wallimage = nullptr;
    AEGfxTexture* wasd = nullptr;


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
    wasd = AEGfxTextureLoad("Assets/WASD.png"); // floor tile texture

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

    //adding the wasd ----
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(wasd, 0, 0);

    AEMtx33 wasdScale, wasdTrans, wasdTransform;

    // size of the WASD image
    AEMtx33Scale(&wasdScale, 220.0f, 200.0f);

    // position on screen
    // move x more right/left, y more up/down
    AEMtx33Trans(&wasdTrans, 200.0f, -270.0f);

    AEMtx33Concat(&wasdTransform, &wasdTrans, &wasdScale);
    AEGfxSetTransform(wasdTransform.m);

    AEGfxSetTransparency(1.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
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