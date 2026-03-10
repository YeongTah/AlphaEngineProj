#include "pch.h"
#include "leveleditor.hpp"
#include "Creator.h"
#include "main.h"
#include "gamestatemanager.h"
#include <iostream>
#include <fstream>

namespace
{
    struct CreditLine
    {
        const char* text; // The text 
        float scale;      // The size of the text 
    };

    CreditLine Credits[] =
    {
        {"CREDITS", 1.6f},
        {"Mummy Maze", 1.3f},
        {"BY THE INTRO LAB", 1.0f},

        {"OUR TEAM", 1.15f},
        {"Sharon", 0.85f},
        {"Jasmine", 0.85f},
        {"Yeong", 0.85f},
        {"San", 0.85f},

        {"DIRECTOR AND PROGRAMMER", 1.05f},
        {"Sharon", 0.85f},
        {"Jasmine", 0.85f},
        {"Yeong", 0.85f},
        {"San", 0.85f},

        {"PRODUCER AND DESIGN LEAD", 1.05f},
        {"Sharon", 0.85f},
        {"Jasmine", 0.85f},

        {"TECHNICAL LEAD", 1.05f},
        {"Yeong", 0.85f},
        {"San", 0.85f},

        {"ART LEAD", 1.05f},
        {"Jasmine", 0.85f},

        {"AUDIO LEAD", 1.05f},
        {"Sharon", 0.85f},

        {"GRAPHICS PROGRAMMER", 1.05f},
        {"Sharon", 0.85f},
        {"Jasmine", 0.85f},
        {"Yeong", 0.85f},
        {"San", 0.85f},

        {"LEVEL DESIGNER", 1.05f},
        {"Jasmine", 0.85f},

        {"ENVIRONMENT ARTIST AND UI", 1.05f},
        {"Sharon", 0.85f},
        {"Jasmine", 0.85f},
        {"Yeong", 0.85f},
        {"San", 0.85f},

        {"COMPOSER", 1.05f},
        {"Sharon", 0.85f},
        {"Jasmine", 0.85f},
        {"Yeong", 0.85f},
        {"San", 0.85f},

        {"SPECIAL THANKS", 1.15f},
        {"PROF. GERALD", 0.85f},
        {"DR. SOROOR", 0.85f},
        {"PROF. TOMMY", 0.85f},
        {"TO THE FELLOW TA'S", 0.85f},

        {"CREATED AT", 1.05f},
        {"DIGIPEN INSTITUTE OF TECHNOLOGY SINGAPORE", 0.80f},

        {"TOOLS USED", 1.05f},
        {"KENNEY", 0.85f},
        {"ASEPRITE", 0.85f},
        {"WWW.DIGIPEN.EDU", 0.85f},

        {"PRESS ESC TO RETURN TO MAIN MENU", 0.75f},
        {"PRESS Q TO QUIT", 0.75f}
    };

    int CreditCount = sizeof(Credits) / sizeof(Credits[0]);
    float CreditOffsetY = 0.5f;
    float CreditSpacing = 0.12f;
    float AutoScrollSpeed = 0.0025f;
    float ManualScrollSpeed = 0.01f;

    AEGfxTexture* wallimage = nullptr; //jas added

    // text = the string to display
    // x = horizontal print position
    // y = vertical print position
    // scale = size of the text

    void DrawCreditText(const char* text, float x, float y, float scale)
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
// Loads Main Menu
//----------------------------------------------------------------------------
void Credit_Load()
{
    std::cout << "Credit:Load\n";
    wallimage = AEGfxTextureLoad("Assets/wall.png"); // floor tile texture
    pMesh = CreateSquareMesh();
}

//----------------------------------------------------------------------------
// Sets up the initial state
//----------------------------------------------------------------------------
void Credit_Initialize()
{
    std::cout << "Credit:Initialize\n";

    //starting Y pos
    CreditOffsetY = 0.5f;
}

//----------------------------------------------------------------------------
// Updates Level Selection Page navigation
//----------------------------------------------------------------------------
void Credit_Update()
{
    std::cout << "Credit:Update\n";

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

    // UP key is pressed
    if (AEInputCheckCurr(AEVK_UP))
    {
        CreditOffsetY += ManualScrollSpeed;
    }
    // DOWN key is pressed
    else if (AEInputCheckCurr(AEVK_DOWN))
    {
        CreditOffsetY -= ManualScrollSpeed;
    }
    //scrolls 
    else
    {
        CreditOffsetY += AutoScrollSpeed;
    }

    //calculate the total height of all credit lines
    float totalHeight = static_cast<float>(CreditCount) * CreditSpacing;

    //resets
    if (CreditOffsetY < -totalHeight)
    {
        CreditOffsetY = 1.0f;
    }
}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
//----------------------------------------------------------------------------
void Credit_Draw()
{
    std::cout << "Credit:Draw\n";

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

    //loop through credits array
    for (int i = 0; i < CreditCount; ++i)
    {
        float y = CreditOffsetY - static_cast<float>(i) * CreditSpacing;

        if (y < -1.3f || y > 1.3f) continue;

        DrawCreditText(Credits[i].text, -0.35f, y, Credits[i].scale);
    }
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
//----------------------------------------------------------------------------
void Credit_Free()
{
    std::cout << "Credit:Free\n";
}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
//----------------------------------------------------------------------------
void Credit_Unload()
{
    std::cout << "Credit:Unload\n";
    AEGfxTextureUnload(wallimage);
}