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

        {"PRESS B TO RETURN TO MAIN MENU", 0.75f},
        {"PRESS ESCAPE TO QUIT", 0.75f}
    };

    int CreditCount = sizeof(Credits) / sizeof(Credits[0]);
    float CreditOffsetY = 0.5f;
    float CreditSpacing = 0.12f;
    float AutoScrollSpeed = 0.0025f;
    float ManualScrollSpeed = 0.01f;
    float topLimit = 0.75f;
    float bottomLimit = -0.8f;

    AEGfxTexture* wallimage = nullptr; //jas added

    // ADDED: back button texture (same as LevelPage)                   -ths
    AEGfxTexture* backButtonTex = nullptr;                              // -ths

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
    //std::cout << "Credit:Load\n";
    wallimage = AEGfxTextureLoad("Assets/Bigwall.png"); // floor tile texture
    // ADDED: load back button texture                                 -ths
    backButtonTex = AEGfxTextureLoad("Assets/Back.png");               // -ths
    pMesh = CreateSquareMesh();
}

//----------------------------------------------------------------------------
// Sets up the initial state
//----------------------------------------------------------------------------
void Credit_Initialize()
{
    //std::cout << "Credit:Initialize\n";

    //starting Y pos
    //CreditOffsetY = 0.5f;
}

//----------------------------------------------------------------------------
// Updates Level Selection Page navigation
//----------------------------------------------------------------------------
void Credit_Update()
{
    //std::cout << "Credit:Update\n";

    // ==================================================================
    // ADDED: Back button click detection (top-left corner)             -ths
    // ==================================================================
    {
        s32 mouseX, mouseY;
        TransformScreentoWorld(mouseX, mouseY);
        static float back_x = -750.0f, back_y = 400.0f;                // -ths
        static float back_w = 50.0f, back_h = 50.0f;                   // -ths
        if (AEInputCheckReleased(AEVK_LBUTTON) &&
            IsAreaClicked(back_x, back_y, back_w, back_h, mouseX, mouseY))
        {
            next = MAINMENUSTATE;
            return;
        }
    }

    // Move back to main menu upon triggering "B"
    if (AEInputCheckReleased(AEVK_B))
    {
        next = MAINMENUSTATE;
        //std::cout << "Back key Released" << '\n';
    }

    // Quit game when ESC is hit or when the window is closed
    if (AEInputCheckReleased(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
        //std::cout << "Q key Released" << '\n';
    }

    // UP key is pressed
    if (AEInputCheckCurr(AEVK_W) || AEInputCheckCurr(AEVK_UP))
    {
        CreditOffsetY += ManualScrollSpeed;
    }
    // DOWN key is pressed
    else if (AEInputCheckCurr(AEVK_S) || AEInputCheckCurr(AEVK_DOWN))
    {
        CreditOffsetY -= ManualScrollSpeed;
    }
    // auto scroll upward
    else
    {
        CreditOffsetY += AutoScrollSpeed;
    }

    // last line position
    float lastLineY = CreditOffsetY - static_cast<float>(CreditCount - 1) * CreditSpacing;

    // loop credits back to bottom after the entire list leaves the top
    if (lastLineY > topLimit)
    {
        CreditOffsetY = bottomLimit;
    }
}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
//----------------------------------------------------------------------------
void Credit_Draw()
{
    //std::cout << "Credit:Draw\n";

    //adding of the main page image--
    AEMtx33 scale, trans, transform;

    if (wallimage)                                                      // -ths
    {
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
    }

    // Draw text copyright
    AEGfxPrint(fontId, "All content © 2026 DigiPen Institute of Technology Singapore. All Rights Reserved.", -0.52f, 0.90f, 0.70f,1.0f, 1.0f, 1.0f, 1.0f);

    //rendering mode to colour mode
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    //loop through credits array
    for (int i = 0; i < CreditCount; ++i)
    {
        float y = CreditOffsetY - static_cast<float>(i) * CreditSpacing;

        if (y < bottomLimit || y > topLimit) continue; //to adjust the text not to go over red border

        DrawCreditText(Credits[i].text, -0.35f, y, Credits[i].scale);
    }

    // ==================================================================
    // ADDED: Back button using texture (same as LevelPage)            -ths
    // ==================================================================
    if (backButtonTex)
    {
        static float back_x = -750.0f, back_y = 400.0f, back_w = 50.0f, back_h = 50.0f;
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(backButtonTex, 0, 0);
        AEMtx33 scaleBtn, transBtn, matBtn;
        AEMtx33Scale(&scaleBtn, back_w, back_h);
        AEMtx33Trans(&transBtn, back_x, back_y);
        AEMtx33Concat(&matBtn, &transBtn, &scaleBtn);
        AEGfxSetTransform(matBtn.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
//----------------------------------------------------------------------------
void Credit_Free()
{
    //std::cout << "Credit:Free\n";
}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
//----------------------------------------------------------------------------
void Credit_Unload()
{
    //std::cout << "Credit:Unload\n";
    if (wallimage)
    {
        AEGfxTextureUnload(wallimage);
        wallimage = nullptr;
    }
    // ADDED: unload back button texture                               -ths
    if (backButtonTex)
    {
        AEGfxTextureUnload(backButtonTex);
        backButtonTex = nullptr;
    }

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}