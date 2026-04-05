/* Start Header ***************************************************************
\file       Instructions.cpp
\coders     Jasmine , Sharon , San
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#include "pch.h"
#include "Instructions.h"
#include "main.h"
#include "gamestatemanager.h"
#include <iostream>
#include <fstream>

namespace
{
    //                                                                --- VARIABLES DECLARATION START HERE ---

    struct InstructionsLine
    {
        const char* text;
        float x;     // x position in NDC
        float y;     // y position in NDC
        float scale; // scaling of the text
    };

    InstructionsLine Instructions[] =
    {
        {"INSTRUCTIONS",             -0.17f,  0.76f, 1.55f},
        {"Welcome to Mummy Game!",   -0.23f,  0.61f, 1.00f},
        {"By Intro Lab",             -0.08f,  0.52f, 0.78f},

        {"GOAL",                     -0.05f,  0.31f, 0.98f},
        {"Avoid enemies and mummies",-0.18f,  0.20f, 0.72f},
        {"Use powerups to survive",  -0.16f,  0.12f, 0.72f},
        {" ",     -0.14f,  0.04f, 0.72f},

        {"[FEATURES]",               -0.10f, -0.10f, 0.98f},
        {"Easy / Medium / Hard",     -0.16f, -0.20f, 0.72f},
        {"Level Editor Mode",        -0.14f, -0.28f, 0.72f},

        {"[CONTROLS]",               -0.27f, -0.50f, 0.95f},
        {"W A S D  - Move",          -0.27f, -0.60f, 0.72f},
        {"B        - Back",          -0.27f, -0.68f, 0.72f},
        {"ESC      - Quit",          -0.27f, -0.76f, 0.72f},

        {"[POWERUPS]",                0.10f, -0.50f, 0.95f},
        {"Freeze enemies",            0.18f, -0.60f, 0.64f}, // index 15
        {"Immunity",                  0.18f, -0.70f, 0.64f}  // index 16
    };

    int InstructionsCount = sizeof(Instructions) / sizeof(Instructions[0]);

    AEGfxTexture* wallimage = nullptr;
    AEGfxTexture* wasd = nullptr;
    AEGfxTexture* pwr1 = nullptr;
    AEGfxTexture* pwr2 = nullptr;

    // Back button texture
    AEGfxTexture* backButtonTex = nullptr;

    //                                                                --- VARIABLES DECLARATION END HERE ---

    // ----------------------------------------------------------------------------
    // DrawInstructionsText
    // Draws one line of instruction text using the given position and scale.
    // ----------------------------------------------------------------------------
    void DrawInstructionsText(const char* text, float x, float y, float scale)
    {
        char strBuffer[256];

        // Clear buffer first
        memset(strBuffer, 0, sizeof(strBuffer));

        // Copy text into the buffer
        sprintf_s(strBuffer, "%s", text);

        AEGfxPrint(fontId, strBuffer, x, y, scale, 1.0f, 1.0f, 1.0f, 1.0f); // white
    }

    // ----------------------------------------------------------------------------
    // DrawInstructionImage
    // Draws one textured image used on the instructions page, such as the
    // powerup icons or control image.
    // ----------------------------------------------------------------------------
    void DrawInstructionImage(AEGfxTexture* texture, float posX, float posY, float sizeX, float sizeY)
    {
        if (!texture) return; // Safety check

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

// ----------------------------------------------------------------------------
// Instructions_Load
// Loads all textures used by the Instructions page and creates the mesh used
// for drawing images on screen.
// ----------------------------------------------------------------------------
void Instructions_Load()
{

    wallimage = AEGfxTextureLoad("Assets/Bigwall.png");
    wasd = AEGfxTextureLoad("Assets/WASD.png");
    pwr1 = AEGfxTextureLoad("Assets/Freeze.png");
    pwr2 = AEGfxTextureLoad("Assets/Immune.png");
    backButtonTex = AEGfxTextureLoad("Assets/Back.png");

    pMesh = CreateSquareMesh();
}

// ----------------------------------------------------------------------------
// Instructions_Initialize
// Sets up the Instructions state when it starts.
// ----------------------------------------------------------------------------
void Instructions_Initialize()
{
}

// ----------------------------------------------------------------------------
// Instructions_Update
// Updates input handling for the Instructions page.
//
// Behaviour:
// 1. Clicking the back button returns to the main menu
// 2. Pressing B returns to the main menu
// 3. Pressing ESCAPE or closing the window quits the game
// ----------------------------------------------------------------------------
void Instructions_Update()
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
}

// ----------------------------------------------------------------------------
// Instructions_Draw
// Renders the full Instructions page each frame, including:
// 1. Background image
// 2. WASD image
// 3. Powerup icons
// 4. All instruction text lines
// 5. Back button
// 6. F5 / F9 helper text beside the powerups
// ----------------------------------------------------------------------------
void Instructions_Draw()
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

    // === WASD IMAGE ===
    if (wasd)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(wasd, 0, 0);

        AEMtx33 wasdScale, wasdTrans, wasdTransform;

        // Size of the WASD image
        AEMtx33Scale(&wasdScale, 220.0f, 200.0f);

        // Position on screen
        AEMtx33Trans(&wasdTrans, -360.0f, -270.0f);

        AEMtx33Concat(&wasdTransform, &wasdTrans, &wasdScale);
        AEGfxSetTransform(wasdTransform.m);

        AEGfxSetTransparency(1.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Reset rendering mode back to colour mode
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // === POWERUP IMAGES ===
    DrawInstructionImage(pwr1, 90.0f, -255.0f, 42.0f, 42.0f);
    DrawInstructionImage(pwr2, 90.0f, -300.0f, 42.0f, 42.0f);

    // === INSTRUCTIONS TEXT ===
    for (int i = 0; i < InstructionsCount; ++i)
    {
        DrawInstructionsText(
            Instructions[i].text,
            Instructions[i].x,
            Instructions[i].y,
            Instructions[i].scale); // loops through the array of InstructionsLine structs
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

    // === F5 / F9 HINTS ===
    {
        // "Freeze enemies" at index 15, "Immunity" at index 16
        const char* freeze_label = Instructions[15].text;
        const char* immunity_label = Instructions[16].text;

        float freeze_label_x = Instructions[15].x;
        float freeze_label_y = Instructions[15].y;
        float immunity_label_y = Instructions[16].y;
        float label_scale = Instructions[15].scale;

        const char* f5_hint = "   (F5 - Save level)";
        const char* f9_hint = "   (F9 - Load level)";
        float hint_scale = 0.55f;

        float freeze_w, freeze_h, immunity_w, immunity_h;
        AEGfxGetPrintSize(fontId, freeze_label, label_scale, &freeze_w, &freeze_h);
        AEGfxGetPrintSize(fontId, immunity_label, label_scale, &immunity_w, &immunity_h);

        float f5_w, f5_h, f9_w, f9_h;
        AEGfxGetPrintSize(fontId, f5_hint, hint_scale, &f5_w, &f5_h);
        AEGfxGetPrintSize(fontId, f9_hint, hint_scale, &f9_w, &f9_h);

        float gap = 0.02f; // Small gap in NDC

        // Compute X position for both hints based on the "Freeze enemies" label
        float hint_x = freeze_label_x + freeze_w + gap;

        // Y positions centered with their respective labels
        float f5_hint_y = freeze_label_y + (freeze_h - f5_h) * 0.5f;
        float f9_hint_y = immunity_label_y + (immunity_h - f9_h) * 0.5f;

        // Draw both hints starting at the same X position
        AEGfxPrint(fontId, f5_hint, hint_x, f5_hint_y, hint_scale, 1, 1, 1, 1);
        AEGfxPrint(fontId, f9_hint, hint_x, f9_hint_y, hint_scale, 1, 1, 1, 1);
    }
}

// ----------------------------------------------------------------------------
// Instructions_Free
// Cleans up runtime data for the Instructions page while keeping long-lived
// resources managed separately in Unload.
// ----------------------------------------------------------------------------
void Instructions_Free()
{
}

// ----------------------------------------------------------------------------
// Instructions_Unload
// Unloads all textures used by the Instructions page when exiting the state.
// ----------------------------------------------------------------------------
void Instructions_Unload()
{
    if (wallimage) { AEGfxTextureUnload(wallimage); wallimage = nullptr; }
    if (wasd) { AEGfxTextureUnload(wasd); wasd = nullptr; }
    if (pwr1) { AEGfxTextureUnload(pwr1); pwr1 = nullptr; }
    if (pwr2) { AEGfxTextureUnload(pwr2); pwr2 = nullptr; }
    if (backButtonTex) { AEGfxTextureUnload(backButtonTex); backButtonTex = nullptr; }

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}