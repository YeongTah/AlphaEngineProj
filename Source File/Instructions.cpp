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
        {"Freeze enemies",          0.18f, -0.60f, 0.64f},   // index 15
        {"Immunity",                0.18f, -0.70f, 0.64f}    // index 16
    };

    int InstructionsCount = sizeof(Instructions) / sizeof(Instructions[0]);
    AEGfxTexture* wallimage = nullptr;
    AEGfxTexture* wasd = nullptr;
    AEGfxTexture* pwr1 = nullptr;
    AEGfxTexture* pwr2 = nullptr;

    // ADDED: back button texture (same as LevelPage)                   -ths
    AEGfxTexture* backButtonTex = nullptr;                              // -ths

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

    //function to help draw the poweruups (added null check)             -ths
    void DrawInstructionImage(AEGfxTexture* texture, float posX, float posY, float sizeX, float sizeY)
    {
        if (!texture) return; // safety check                           -ths

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

    // ADDED: load back button texture                                 -ths
    backButtonTex = AEGfxTextureLoad("Assets/Back.png");               // -ths

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

    // null check for wallimage                                        -ths
    if (wallimage)
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

    //adding the wasd ----
    if (wasd)                                                           // -ths
    {
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
    }
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

    // ==================================================================
    // ADDED: F5 / F9 hints – both aligned to the same X coordinate   -ths
    // ==================================================================
    {
        // "Freeze enemies" at index 15, "Immunity" at index 16        -ths
        const char* freeze_label = Instructions[15].text;   // "Freeze enemies"
        const char* immunity_label = Instructions[16].text; // "Immunity"
        float freeze_label_x = Instructions[15].x;   // 0.18f
        float freeze_label_y = Instructions[15].y;   // -0.60f
        float immunity_label_y = Instructions[16].y; // -0.70f
        float label_scale = Instructions[15].scale;  // 0.64f

        const char* f5_hint = "   (F5 - Save level)";
        const char* f9_hint = "   (F9 - Load level)";
        float hint_scale = 0.55f;

        float freeze_w, freeze_h, immunity_w, immunity_h;
        AEGfxGetPrintSize(fontId, freeze_label, label_scale, &freeze_w, &freeze_h);
        AEGfxGetPrintSize(fontId, immunity_label, label_scale, &immunity_w, &immunity_h);

        float f5_w, f5_h, f9_w, f9_h;
        AEGfxGetPrintSize(fontId, f5_hint, hint_scale, &f5_w, &f5_h);
        AEGfxGetPrintSize(fontId, f9_hint, hint_scale, &f9_w, &f9_h);

        float gap = 0.02f; // small gap in NDC
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
    if (wallimage) AEGfxTextureUnload(wallimage);
    if (wasd) AEGfxTextureUnload(wasd);
    if (pwr1) AEGfxTextureUnload(pwr1);
    if (pwr2) AEGfxTextureUnload(pwr2);
    if (backButtonTex) AEGfxTextureUnload(backButtonTex);               // -ths
}