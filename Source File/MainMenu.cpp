/* Start Header ***************************************************************
\file       Level1.cpp
\coders     Sharon, San
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#include "pch.h"
#include "MainMenu.h"
#include "gamestatemanager.h"
#include "Main.h"
#include "Level1.h"
#include "Confirmation.h"

//                                                                --- VARIABLES DECLARATION START HERE ---
// === Audio variables ===
static AEAudio bgmMainMenu;
static AEAudio sfxButtonClick;

// === button variables ===
#define array_count(a) (sizeof(a)/sizeof(*a))

float button_x;
float playbutton_y;
float instructbutton_y;
float creditbutton_y;
float exitbutton_y;
float createbutton_x, createbutton_y;
AEGfxTexture* mainpage = nullptr;

//                                                                --- VARIABLES DECLARATION END HERE ---

// ----------------------------------------------------------------------------
// MainMenu_Load
// Loads the main menu background texture and both audio assets (BGM and button
// click SFX) from Assets/. Starts the menu BGM immediately on loop. Creates
// the shared pMesh unit square used for rendering. Called once before the
// game loop begins.
//----------------------------------------------------------------------------
void MainMenu_Load()
{
    // Load Game Picture
    mainpage = AEGfxTextureLoad("Assets/MainPage.png");

    // Load Audio
    bgmMainMenu = AEAudioLoadMusic("Assets/audio/mainmenu.wav");
    sfxButtonClick = AEAudioLoadSound("Assets/audio/button.wav");

    // Play menu BGM (looping) - only if valid
    if (AEAudioIsValidAudio(bgmMainMenu))
        AEAudioPlay(bgmMainMenu, AEAudioCreateGroup(), 1.0f, 1.0f, -1); // loop forever

    pMesh = CreateSquareMesh();
}

//----------------------------------------------------------------------------
// MainMenu_Initialize
// Sets the world-space positions of all five buttons (Play, Instructions,
// Credits, Exit, Creator) on every entry into the main menu state. Called
// each time the GSM transitions into MAINMENUSTATE.
//----------------------------------------------------------------------------
void MainMenu_Initialize()
{

    button_x = 0.0f;
    playbutton_y = 100.0f;
    instructbutton_y = -25.0f;
    creditbutton_y = -150.0f;
    exitbutton_y = -275.0f;
    createbutton_x = 680.0f;
    createbutton_y = -370.0f;
}

//----------------------------------------------------------------------------
// MainMenu_Update
// Handles mouse click input for all five buttons each frame. Converts screen
// mouse coordinates to world space and checks each button's hit area using
// IsAreaClicked. Plays the button click SFX on any valid click. Navigates to
// LEVELPAGE, INSTRUCTIONS, CREDIT, CREATOR, or routes EXIT and ESC through
// the confirmation screen before quitting.
//----------------------------------------------------------------------------
void MainMenu_Update()
{
    s32 mouseX, mouseY;
    TransformScreentoWorld(mouseX, mouseY);

    // CLICK: PLAY
    if (AEInputCheckReleased(AEVK_LBUTTON) &&
        IsAreaClicked(button_x, playbutton_y, 300.0f, 90.0f, mouseX, mouseY))
    {
        // PLAY BUTTON SFX
        if (AEAudioIsValidAudio(sfxButtonClick))
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0);

        next = LEVELPAGE;
    }

    // CLICK: INSTRUCTIONS
    if (AEInputCheckReleased(AEVK_LBUTTON) &&
        IsAreaClicked(button_x, instructbutton_y, 300.0f, 90.0f, mouseX, mouseY))
    {
        // BUTTON CLICK SOUND
        if (AEAudioIsValidAudio(sfxButtonClick))
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0);

        next = INSTRUCTIONS;
    }

    // CLICK: CREDITS
    if (AEInputCheckReleased(AEVK_LBUTTON) &&
        IsAreaClicked(button_x, creditbutton_y, 300.0f, 90.0f, mouseX, mouseY))
    {
        // BUTTON CLICK SOUND 
        if (AEAudioIsValidAudio(sfxButtonClick)) 
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0); 

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

        // Show confirmation before quitting                      
        Confirmation_Level(MAINMENUSTATE, GS_QUIT, "Are you sure you want to quit?");
        next = CONFIRM;
    }

    // CLICK: CREATOR
    if (AEInputCheckReleased(AEVK_LBUTTON) &&
            IsAreaClicked(createbutton_x, createbutton_y, 150.0f, 65.0f, mouseX, mouseY))
    {
        // BUTTON CLICK SOUND 
        if (AEAudioIsValidAudio(sfxButtonClick)) 
            AEAudioPlay(sfxButtonClick, AEAudioCreateGroup(), 1.0f, 1.0f, 0); 

        next = CREATOR;
    }
}

//----------------------------------------------------------------------------
// MainMenu_Draw
// Renders the main menu each frame. Draws the full-screen background texture
// first, then overlays five colored button rectangles using AE_GFX_RM_COLOR.
// Converts each button's world-space position to NDC and center-aligns the
// button label text using AEGfxGetPrintSize. Called every frame by the GSM.
//----------------------------------------------------------------------------
void MainMenu_Draw()
{
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

    // === Button rendering ===
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

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // === Colours for buttons ===
    for (int i = 0; i < array_count(buttons); ++i) 
    {

        if (i == 0 || i == 1 || i == 2 || i == 3)
            AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f);
        else
            AEGfxSetColorToAdd(0.21f, 0.11f, 0.12f, 1.0f);

        AEGfxSetTransform(buttons[i].m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    //                                                                --- START OF PRINTING OF BUTTON TEXT ---

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

    // === PLAY ===
    const char* t0 = "PLAY";
    float w0, h0;
    AEGfxGetPrintSize(fontId, t0, titleScale, &w0, &h0);
    float playX_NDC = btnCenterX_NDC - w0 * 0.5f;     // Center horizontally
    float playY_NDC_centered = playY_NDC - h0 * 0.5f; // Center vertically
    AEGfxPrint(fontId, t0, playX_NDC, playY_NDC_centered, titleScale, 1.0f, 1.0f, 1.0f, 1.0f);

    // === INSTRUCTIONS ===
    const char* t1 = "INSTRUCTIONS";
    float w1, h1;
    AEGfxGetPrintSize(fontId, t1, titleScale, &w1, &h1);
    float instructX_NDC = btnCenterX_NDC - w1 * 0.5f;
    float instructY_NDC_centered = instructY_NDC - h1 * 0.5f;
    AEGfxPrint(fontId, t1, instructX_NDC, instructY_NDC_centered, titleScale, 1.0f, 1.0f, 1.0f, 1.0f);

    // === CREDITS ===
    const char* t2 = "CREDITS";
    float w2, h2;
    AEGfxGetPrintSize(fontId, t2, titleScale, &w2, &h2);
    float creditX_NDC = btnCenterX_NDC - w2 * 0.5f;
    float creditY_NDC_centered = creditY_NDC - h2 * 0.5f;
    AEGfxPrint(fontId, t2, creditX_NDC, creditY_NDC_centered, titleScale, 1.0f, 1.0f, 1.0f, 1.0f);

    // === EXIT ===
    const char* t3 = "EXIT";
    float w3, h3;
    AEGfxGetPrintSize(fontId, t3, titleScale, &w3, &h3);
    float exitX_NDC = btnCenterX_NDC - w3 * 0.5f;
    float exitY_NDC_centered = exitY_NDC - h3 * 0.5f;
    AEGfxPrint(fontId, t3, exitX_NDC, exitY_NDC_centered, titleScale, 1.0f, 1.0f, 1.0f, 1.0f);

    // === CREATOR ===
    const float createScale = 0.8f;
    const char* t4 = "Creator";
    float w4, h4;
    AEGfxGetPrintSize(fontId, t4, createScale, &w4, &h4);
    float createX_NDC_centered = createX_NDC - w4 * 0.5f;
    float createY_NDC_centered = createY_NDC - h4 * 0.5f;
    AEGfxPrint(fontId, t4, createX_NDC_centered, createY_NDC_centered, createScale, 1.0f, 1.0f, 1.0f, 1.0f);

    //                                                                --- END OF PRINTING OF BUTTON TEXT ---
}

//----------------------------------------------------------------------------
// MainMenu_Free
// Called after the game loop exits this state, before Unload.
// Currently empty as the Main Menu state has no heap allocations or runtime
// resources that need releasing between Free and Unload. Still needs to be
// here or there would be linker error / null function pointer crash
//----------------------------------------------------------------------------
void MainMenu_Free() {}

//----------------------------------------------------------------------------
// MainMenu_Unload
// Unloads both audio assets and frees the shared pMesh to prevent memory
// leaks. Sets pMesh to nullptr after freeing to prevent dangling references.
// Note: the main menu background texture is not explicitly unloaded here and
// should be added to prevent a GPU texture leak on exit.
//----------------------------------------------------------------------------
void MainMenu_Unload()
{

    // Unload Audio
    if (AEAudioIsValidAudio(bgmMainMenu))     AEAudioUnloadAudio(bgmMainMenu);     
    if (AEAudioIsValidAudio(sfxButtonClick))  AEAudioUnloadAudio(sfxButtonClick);  

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
    if (mainpage)
    {
        AEGfxTextureUnload(mainpage);
        mainpage = nullptr;
    }
}
