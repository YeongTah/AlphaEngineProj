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
        {"INSTRUCTIONS",           -0.17f,  0.76f, 1.55f},
        {"Welcome to Mummy Game!", -0.23f,  0.61f, 1.00f},
        {"By Intro Lab",           -0.08f,  0.52f, 0.78f},

        {"GOAL",                    -0.05f,  0.31f, 0.98f},
        {"Avoid enemies and mummies",-0.18f, 0.20f, 0.72f},
        {"Use powerups to survive", -0.16f,  0.12f, 0.72f},
        {"Reach the end to win",    -0.14f,  0.04f, 0.72f},

        {"[FEATURES]",                -0.10f, -0.10f, 0.98f},
        {"Easy / Medium / Hard",    -0.16f, -0.20f, 0.72f},
        {"Level Editor Mode",       -0.14f, -0.28f, 0.72f},

        {"[CONTROLS]",               -0.27f, -0.50f, 0.95f},
        {"W A S D  - Move",        -0.27f, -0.60f, 0.72f},
        {"B        - Back",        -0.27f, -0.68f, 0.72f},
        {"ESC      - Quit",        -0.27f, -0.76f, 0.72f},

        {"[POWERUPS]",                0.10f, -0.50f, 0.95f},
        {"Freeze enemies",  0.18f, -0.60f, 0.64f},
        {"Immunity",      0.18f, -0.70f, 0.64f},
    };

    int InstructionsCount = sizeof(Instructions) / sizeof(Instructions[0]);
    AEGfxTexture* wallimage = nullptr;
    AEGfxTexture* wasd = nullptr;
    AEGfxTexture* pwr1 = nullptr;
    AEGfxTexture* pwr2 = nullptr;


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

    //function to help draw the poweruups
    void DrawInstructionImage(AEGfxTexture* texture, float posX, float posY, float sizeX, float sizeY)
    {
        AEMtx33 scale, trans, transform;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(texture, 0, 0);

        AEMtx33Scale(&scale, sizeX, sizeY);
        AEMtx33Trans(&trans, posX, posY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxSetTransparency(1.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
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
    pwr1 = AEGfxTextureLoad("Assets/Freeze.png"); // freeze
    pwr2 = AEGfxTextureLoad("Assets/Immune.png"); // Immune

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
    if (AEInputCheckReleased(AEVK_B))
    {
        next = MAINMENUSTATE;
        std::cout << "Back key Released" << '\n';
    }

    // Quit game when Q is hit or when the window is closed
    if (AEInputCheckReleased(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
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
    AEMtx33Trans(&wasdTrans, -360.0f, -270.0f);

    AEMtx33Concat(&wasdTransform, &wasdTrans, &wasdScale);
    AEGfxSetTransform(wasdTransform.m);

    AEGfxSetTransparency(1.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    //rendering mode to colour mode
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    //adding the powerups--
    DrawInstructionImage(pwr1, 90.0f, -255.0f, 42.0f, 42.0f); //jas
    DrawInstructionImage(pwr2, 90.0f, -300.0f, 42.0f, 42.0f); //jas

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