#include "pch.h"
#include "leveleditor.hpp"
#include "Creator.h"
#include "main.h"
#include "gamestatemanager.h"

namespace
{
    //                                                                --- VARIABLES DECLARATION START HERE ---

    struct CreditLine
    {
        const char* text; // The text to display
        float scale;      // The size of the text
    };

    CreditLine Credits[] =
    {
        {"CREDITS", 1.6f},
        {"Mummy Game", 1.3f},
        {"BY THE INTRO LAB", 1.0f},

        {" "},

        {"OUR TEAM", 1.15f},
        {"Sharon Lim Joo Ai", 0.85f},
        {"Jasmine Dela Cruz Tan", 0.85f},
        {"Lai Yeong Tah", 0.85f},
        {"Thu Htoo San", 0.85f},

        {" "},

        {"DIRECTOR AND PROGRAMMER", 1.05f},
        {"Sharon Lim Joo Ai", 0.85f},
        {"Jasmine Dela Cruz Tan", 0.85f},
        {"Lai Yeong Tah", 0.85f},
        {"Thu Htoo San", 0.85f},

        {" "},

        {"PRODUCER AND DESIGN LEAD", 1.05f},
        {"Sharon Lim Joo Ai", 0.85f},
        {"Jasmine Dela Cruz Tan", 0.85f},

        {" "},

        {"TECHNICAL LEAD", 1.05f},
        {"Lai Yeong Tah", 0.85f},
        {"Thu Htoo San", 0.85f},

        {" "},

        {"ART LEAD", 1.05f},
        {"Jasmine Dela Cruz Tan", 0.85f},

        {" "},

        {"AUDIO LEAD", 1.05f},
        {"Thu Htoo San", 0.85f},

        {" "},

        {"GRAPHICS PROGRAMMER", 1.05f},
        {"Sharon Lim Joo Ai", 0.85f},
        {"Jasmine Dela Cruz Tan", 0.85f},
        {"Lai Yeong Tah", 0.85f},
        {"Thu Htoo San", 0.85f},

        {" "},

        {"LEVEL DESIGNER", 1.05f},
        {"Jasmine Dela Cruz Tan", 0.85f},

        {" "},

        {"ENVIRONMENT ARTIST AND UI", 1.05f},
        {"Sharon Lim Joo Ai", 0.85f},
        {"Jasmine Dela Cruz Tan", 0.85f},
        {"Lai Yeong Tah", 0.85f},
        {"Thu Htoo San", 0.85f},

        {" "},

        {"COMPOSER", 1.05f},
        {"Sharon Lim Joo Ai", 0.85f},
        {"Jasmine Dela Cruz Tan", 0.85f},
        {"Lai Yeong Tah", 0.85f},
        {"Thu Htoo San", 0.85f},

        {" "},

        {"INSTRUCTORS", 1.15f},

        {"DESIGN AND PRODUCTION", 1.05f},
        {"Gerald Wong Han Feng", 0.85f},
        {"Dr. Soroor Malekmohammadai Faradounbeh", 0.85f},
        {"Tommy Tan Chee Wei", 0.85f},

        {" "},

        {"ART", 1.05f},
        {"Gerald Wong Han Feng", 0.85f},
        {"Dr. Soroor Malekmohammadai Faradounbeh", 0.85f},
        {"Tommy Tan Chee Wei", 0.85f},

        {" "},

        {"PROGRAMMING", 1.05f},
        {"Gerald Wong Han Feng", 0.85f},
        {"Dr. Soroor Malekmohammadai Faradounbeh", 0.85f},
        {"Tommy Tan Chee Wei", 0.85f},

        {" "},

        {"AUDIO", 1.05f},
        {"Gerald Wong Han Feng", 0.85f},
        {"Dr. Soroor Malekmohammadai Faradounbeh", 0.85f},
        {"Tommy Tan Chee Wei", 0.85f},

        { " " },

        {"LAB MANAGEMENT AND IT", 1.05f},
        {"Gerald Wong Han Feng", 0.85f},
        {"Dr. Soroor Malekmohammadai Faradounbeh", 0.85f},
        {"Tommy Tan Chee Wei", 0.85f},

        { " " },

        {"CREATED AT", 1.05f},
        {"DIGIPEN INSTITUTE OF TECHNOLOGY SINGAPORE", 0.80f},

        { " " },

        {"PRESIDENT", 1.05f},
        {"Claude Comair", 0.80f},

        { " " },

        {"EXECUTIVES", 1.05f},
        {"CHU Jason Yeu Tat", 0.80f},
        {"Michael GATS", 0.80f},
        {"TAN Chek Ming", 0.80f},
        {"Prasanna Kumar GHALI", 0.80f},
        {"Mandy WONG", 0.80f},
        {"Johnny DEEK", 0.80f},

        { " " },

        {"TOOLS USED", 1.05f},
        {"Font from www.kenny.nl (CCO)", 0.85f},
        {"ASEPRITE", 0.85f},

        { " " },

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

    AEGfxTexture* wallimage = nullptr;
    AEGfxTexture* backButtonTex = nullptr;

    //                                                                --- VARIABLES DECLARATION END HERE ---

    // ----------------------------------------------------------------------------
    // DrawCreditText
    // Draws one line of credit text using the given position and scale.
    // ----------------------------------------------------------------------------
    void DrawCreditText(const char* text, float x, float y, float scale)
    {
        char strBuffer[256];

        // Clear buffer first
        memset(strBuffer, 0, sizeof(strBuffer));

        // Copy text into the buffer
        sprintf_s(strBuffer, "%s", text);

        AEGfxPrint(fontId, strBuffer, x, y, scale, 1.0f, 1.0f, 1.0f, 1.0f); // white
    }
}

// ----------------------------------------------------------------------------
// Credit_Load
// Loads all textures used by the Credits page and creates the mesh used for
// drawing images on screen.
// ----------------------------------------------------------------------------
void Credit_Load()
{
    wallimage = AEGfxTextureLoad("Assets/Bigwall.png");
    backButtonTex = AEGfxTextureLoad("Assets/Back.png");

    pMesh = CreateSquareMesh();
}

// ----------------------------------------------------------------------------
// Credit_Initialize
// Sets up the Credits state when it starts.
// ----------------------------------------------------------------------------
void Credit_Initialize()
{
    // Starting Y position
    // CreditOffsetY = 0.5f;
}

// ----------------------------------------------------------------------------
// Credit_Update
// Updates input handling and scrolling for the Credits page.
//
// Behaviour:
// 1. Clicking the back button returns to the main menu
// 2. Pressing B returns to the main menu
// 3. Pressing ESCAPE or closing the window quits the game
// 4. W / Up scrolls upward manually
// 5. S / Down scrolls downward manually
// 6. If no manual input is given, the credits auto-scroll upward
// ----------------------------------------------------------------------------
void Credit_Update()
{
    // === BACK BUTTON CLICK ===
    {
        s32 mouseX, mouseY;
        TransformScreentoWorld(mouseX, mouseY);

        static float back_x = -750.0f;
        static float back_y = 400.0f;
        static float back_w = 50.0f;
        static float back_h = 50.0f;

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
    }

    // Quit game when ESC is hit or when the window is closed
    if (AEInputCheckReleased(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
    }

    // === MANUAL / AUTO SCROLL ===

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
    // Auto scroll upward
    else
    {
        CreditOffsetY += AutoScrollSpeed;
    }

    // Last line position
    float lastLineY = CreditOffsetY - static_cast<float>(CreditCount - 1) * CreditSpacing;

    // Loop credits back to bottom after the entire list leaves the top
    if (lastLineY > topLimit)
    {
        CreditOffsetY = bottomLimit;
    }
}

// ----------------------------------------------------------------------------
// Credit_Draw
// Renders the full Credits page each frame, including:
// 1. Background image
// 2. Copyright text
// 3. Scrolling credits text
// 4. Back button
// ----------------------------------------------------------------------------
void Credit_Draw()
{
    AEMtx33 scale, trans, transform;

    // === BACKGROUND IMAGE ===
    if (wallimage)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(wallimage, 0, 0);

        // Size
        AEMtx33Scale(&scale, 1600.0f, 900.0f);

        // Center of screen
        AEMtx33Trans(&trans, 0.0f, 0.0f);

        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);

        AEGfxSetTransparency(1.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // === COPYRIGHT TEXT ===
    AEGfxPrint(fontId,
        "All content © 2026 DigiPen Institute of Technology Singapore. All Rights Reserved.",
        -0.52f, 0.90f, 0.70f, 1.0f, 1.0f, 1.0f, 1.0f);

    // Reset rendering mode back to colour mode
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // === SCROLLING CREDITS TEXT ===
    for (int i = 0; i < CreditCount; ++i)
    {
        float y = CreditOffsetY - static_cast<float>(i) * CreditSpacing;

        // Skip text outside visible bounds
        if (y < bottomLimit || y > topLimit)
            continue;

        DrawCreditText(Credits[i].text, -0.35f, y, Credits[i].scale);
    }

    // === BACK BUTTON ===
    if (backButtonTex)
    {
        static float back_x = -750.0f;
        static float back_y = 400.0f;
        static float back_w = 50.0f;
        static float back_h = 50.0f;

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

// ----------------------------------------------------------------------------
// Credit_Free
// Cleans up runtime data for the Credits page while keeping long-lived
// resources managed separately in Unload.
// ----------------------------------------------------------------------------
void Credit_Free()
{
}

// ----------------------------------------------------------------------------
// Credit_Unload
// Unloads all textures and frees the mesh used by the Credits page.
// ----------------------------------------------------------------------------
void Credit_Unload()
{
    if (wallimage)
    {
        AEGfxTextureUnload(wallimage);
        wallimage = nullptr;
    }

    if (backButtonTex)
    {
        AEGfxTextureUnload(backButtonTex);
        backButtonTex = nullptr;
    }

    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }

}