#include "pch.h"
#include "MainMenu.h"
#include "gamestatemanager.h"
#include "Main.h"
#include <iostream>
#include <fstream>

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

    // CLICK: EXIT
    if (AEInputCheckReleased(AEVK_Q) ||
        0 == AESysDoesWindowExist() ||
        (AEInputCheckReleased(AEVK_LBUTTON) &&
            IsAreaClicked(button_x, exitbutton_y, 300.0f, 90.0f, mouseX, mouseY)))
    {
        // BUTTON CLICK SOUND // -ths
        if (AEAudioIsValidAudio(sfxButtonClick)) // -ths
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0); // -ths

        next = GS_QUIT;
    }

    // CLICK: CREATOR
    if (AEInputCheckReleased(AEVK_Q) ||
        0 == AESysDoesWindowExist() ||
        (AEInputCheckReleased(AEVK_LBUTTON) &&
            IsAreaClicked(createbutton_x, createbutton_y, 150.0f, 65.0f, mouseX, mouseY)))
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

    AEGfxSetBackgroundColor(0.84f, 0.76f, 0.58f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    for (int i = 0; i < array_count(buttons); ++i)
    {
        AEGfxSetTransform(buttons[i].m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Button text (unchanged)
    AEGfxPrint(fontId, "PLAY", -0.05f, 0.20f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxPrint(fontId, "INSTRUCTIONS", -0.13f, -0.08f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxPrint(fontId, "CREDITS", -0.081f, -0.36f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxPrint(fontId, "EXIT", -0.045f, -0.63f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxPrint(fontId, "Creator", 0.8f, -0.84f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
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
