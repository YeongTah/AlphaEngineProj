#include "pch.h"
#include "MainMenu.h"
#include "gamestatemanager.h"
#include "Main.h"
#include "Level1.h"
#include <iostream>
#include <fstream>
#include "Confirmation.h"

// --- NEW AUDIO VARIABLES --- // -ths
static AEAudio bgmMainMenu;     // -ths
static AEAudio sfxButtonClick;  // -ths

//                                                                --- Variables declaration start here ---
#define array_count(a) (sizeof(a)/sizeof(*a))

float button_x;
float playbutton_y;
float instructbutton_y;
float creditbutton_y;
float exitbutton_y;
float createbutton_x, createbutton_y;
AEGfxTexture* mainpage = nullptr;
//                                                                --- Variables declaration end here ---

//----------------------------------------------------------------------------
// Loads Main Menu
//----------------------------------------------------------------------------
void MainMenu_Load()
{
    std::cout << "MainMenu:Load\n";

    mainpage = AEGfxTextureLoad("Assets/MainPage.png");

    // --- AUDIO LOAD --- // -ths
    bgmMainMenu = AEAudioLoadMusic("Assets/audio/mainmenu.wav"); // -ths
    sfxButtonClick = AEAudioLoadSound("Assets/audio/button.wav");   // -ths

    // Play menu BGM (looping) - only if valid // -ths
    if (AEAudioIsValidAudio(bgmMainMenu))  // -ths
        AEAudioPlay(bgmMainMenu, AEAudioCreateGroup(), 1.0f, 1.0f, -1); // loop forever // -ths

    pMesh = CreateSquareMesh();
}

//----------------------------------------------------------------------------
// Sets up the initial state
//----------------------------------------------------------------------------
void MainMenu_Initialize()
{
    std::cout << "MainMenu:Initialize\n";

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
//----------------------------------------------------------------------------
void MainMenu_Update()
{
    s32 mouseX, mouseY;
    TransformScreentoWorld(mouseX, mouseY);

    // CLICK: PLAY
    if (AEInputCheckReleased(AEVK_LBUTTON) &&
        IsAreaClicked(button_x, playbutton_y, 300.0f, 90.0f, mouseX, mouseY))
    {
        // PLAY BUTTON SFX // -ths
        if (AEAudioIsValidAudio(sfxButtonClick)) // -ths
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0); // -ths

        next = LEVELPAGE;
        std::cout << "Left click released\n";
        std::cout << "next state: " << next << "\n";
    }

    // CLICK: INSTRUCTIONS
    if (AEInputCheckReleased(AEVK_LBUTTON) &&
        IsAreaClicked(button_x, instructbutton_y, 300.0f, 90.0f, mouseX, mouseY))
    {
        // BUTTON CLICK SOUND // -ths
        if (AEAudioIsValidAudio(sfxButtonClick)) // -ths
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0); // -ths

        next = INSTRUCTIONS;
    }

    // CLICK: CREDITS
    if (AEInputCheckReleased(AEVK_LBUTTON) &&
        IsAreaClicked(button_x, creditbutton_y, 300.0f, 90.0f, mouseX, mouseY))
    {
        // BUTTON CLICK SOUND // -ths
        if (AEAudioIsValidAudio(sfxButtonClick)) // -ths
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0); // -ths

        next = CREDIT;
    }

    // Inside MainMenu_Update(), replace the exit block

// CLICK: EXIT
    if (AEInputCheckReleased(AEVK_ESCAPE) ||
        (AEInputCheckReleased(AEVK_LBUTTON) &&
            IsAreaClicked(button_x, exitbutton_y, 300.0f, 90.0f, mouseX, mouseY)))
    {
        // BUTTON CLICK SOUND
        if (AEAudioIsValidAudio(sfxButtonClick))
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0);

        // Show confirmation before quitting                      // -ths
        Confirmation_Level(MAINMENUSTATE, GS_QUIT, "Are you sure you want to quit?");
        next = CONFIRM;
    }

    // CLICK: CREATOR
    if (AEInputCheckReleased(AEVK_LBUTTON) &&
            IsAreaClicked(createbutton_x, createbutton_y, 150.0f, 65.0f, mouseX, mouseY))
    {
        // BUTTON CLICK SOUND // -ths
        if (AEAudioIsValidAudio(sfxButtonClick)) // -ths
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0); // -ths

        next = CREATOR;
    }
}

//----------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------
void MainMenu_Draw()
{
    // (unchanged draw code)
    // no audio changes needed here -ths

    AEMtx33 scale, trans, transform;
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(mainpage, 0, 0);

    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);

    AEGfxSetTransparency(1.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Button rendering (unchanged)
    AEMtx33 buttons[5] = { 0 };
    AEMtx33 button_scale, button_tran;

    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, button_x, playbutton_y);
    AEMtx33Concat(&buttons[0], &button_tran, &button_scale);

    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, button_x, instructbutton_y);
    AEMtx33Concat(&buttons[1], &button_tran, &button_scale);

    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, button_x, creditbutton_y);
    AEMtx33Concat(&buttons[2], &button_tran, &button_scale);

    AEMtx33Scale(&button_scale, 300.f, 90.f);
    AEMtx33Trans(&button_tran, button_x, exitbutton_y);
    AEMtx33Concat(&buttons[3], &button_tran, &button_scale);

    AEMtx33Scale(&button_scale, 150.f, 65.f);
    AEMtx33Trans(&button_tran, createbutton_x, createbutton_y);
    AEMtx33Concat(&buttons[4], &button_tran, &button_scale);

    //AEGfxSetBackgroundColor(0.84f, 0.76f, 0.58f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    //for (int i = 0; i < array_count(buttons); ++i)
    //{
    //    AEGfxSetTransform(buttons[i].m);
    //    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    //}

    for (int i = 0; i < array_count(buttons); ++i) // added the colour - jas for colour
    {
        //AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        if (i == 0 || i == 1 || i == 2 || i == 3)
            AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f);
        else
            AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f);

        AEGfxSetTransform(buttons[i].m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // ================ PRINTING OF BUTTON TEXT ================

    // Convery button world positions to normalised device coordinates
    const float titleScale = 1.2f;
    float btnCenterX_NDC = button_x / 800.0f;
    float playY_NDC = playbutton_y / 450.0f;
    float instructY_NDC = instructbutton_y / 450.0f;
    float creditY_NDC = creditbutton_y / 450.0f;
    float exitY_NDC = exitbutton_y / 450.0f;
    // Specific to creator button
    float createX_NDC = createbutton_x / 800.0f;
    float createY_NDC = createbutton_y / 450.0f;

    // --- Play ---
    const char* t0 = "PLAY";
    float w0, h0;
    AEGfxGetPrintSize(fontId, t0, titleScale, &w0, &h0);
    float playX_NDC = btnCenterX_NDC - w0 * 0.5f;     // Center horizontally
    float playY_NDC_centered = playY_NDC - h0 * 0.5f; // Center vertically
    AEGfxPrint(fontId, t0, playX_NDC, playY_NDC_centered, titleScale, 1.0f, 1.0f, 1.0f, 1.0f);

    // --- INSTRUCTIONS ---
    const char* t1 = "INSTRUCTIONS";
    float w1, h1;
    AEGfxGetPrintSize(fontId, t1, titleScale, &w1, &h1);
    float instructX_NDC = btnCenterX_NDC - w1 * 0.5f;
    float instructY_NDC_centered = instructY_NDC - h1 * 0.5f;
    AEGfxPrint(fontId, t1, instructX_NDC, instructY_NDC_centered, titleScale, 1.0f, 1.0f, 1.0f, 1.0f);

    // --- CREDITS ---
    const char* t2 = "CREDITS";
    float w2, h2;
    AEGfxGetPrintSize(fontId, t2, titleScale, &w2, &h2);
    float creditX_NDC = btnCenterX_NDC - w2 * 0.5f;
    float creditY_NDC_centered = creditY_NDC - h2 * 0.5f;
    AEGfxPrint(fontId, t2, creditX_NDC, creditY_NDC_centered, titleScale, 1.0f, 1.0f, 1.0f, 1.0f);

    // --- EXIT ---
    const char* t3 = "EXIT";
    float w3, h3;
    AEGfxGetPrintSize(fontId, t3, titleScale, &w3, &h3);
    float exitX_NDC = btnCenterX_NDC - w3 * 0.5f;
    float exitY_NDC_centered = exitY_NDC - h3 * 0.5f;
    AEGfxPrint(fontId, t3, exitX_NDC, exitY_NDC_centered, titleScale, 1.0f, 1.0f, 1.0f, 1.0f);

    // --- CREATOR ---
    const float createScale = 0.8f;
    const char* t4 = "Creator";
    float w4, h4;
    AEGfxGetPrintSize(fontId, t4, createScale, &w4, &h4);
    float createX_NDC_centered = createX_NDC - w4 * 0.5f;
    float createY_NDC_centered = createY_NDC - h4 * 0.5f;
    AEGfxPrint(fontId, t4, createX_NDC_centered, createY_NDC_centered, createScale, 1.0f, 1.0f, 1.0f, 1.0f);
}

//----------------------------------------------------------------------------
// Free
//----------------------------------------------------------------------------
void MainMenu_Free()
{
    std::cout << "MainMenu:Free\n";
}

//----------------------------------------------------------------------------
// Unload
//----------------------------------------------------------------------------
void MainMenu_Unload()
{
    std::cout << "MainMenu:Unload\n";

    // --- AUDIO UNLOAD --- // -ths
    if (AEAudioIsValidAudio(bgmMainMenu))     AEAudioUnloadAudio(bgmMainMenu);     // -ths
    if (AEAudioIsValidAudio(sfxButtonClick))  AEAudioUnloadAudio(sfxButtonClick);  // -ths

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}
