#include "pch.h"
#include "WinPage.h"
#include "gamestatemanager.h"
#include "Main.h"
#include <iostream>

// ======================================================
// AUDIO VARIABLES (WIN PAGE)                         -ths
// ======================================================
static AEAudio sfxWin;        // -ths
static AEAudio sfxButton;     // -ths
static AEAudioGroup winGroup; // -ths

// ======================================================
// UI LAYOUT VALUES (MATCHING YOUR SCREENSHOT)
// ======================================================
float kBtnW = 300.0f;
float kBtnH = 90.0f;

float kRetryX = 0.0f;
float kRetryY = -100.0f;

float kExitX = 0.0f;
float kExitY = -250.0f;

// Mesh for white rectangles
AEGfxVertexList* winMesh = nullptr; // -ths

// ======================================================
// LOAD
// ======================================================
void WinPage_Load()
{
    std::cout << "WinPage:Load\n";

    winGroup = AEAudioCreateGroup(); // -ths
    sfxWin = AEAudioLoadSound("Assets/audio/win.wav");       // -ths
    sfxButton = AEAudioLoadSound("Assets/audio/button.wav");   // -ths

    if (AEAudioIsValidAudio(sfxWin))
        AEAudioPlay(sfxWin, winGroup, 1.0f, 1.0f, 0); // -ths
}

// ======================================================
// INITIALIZE
// ======================================================
void WinPage_Initialize()
{
    std::cout << "WinPage:Initialize\n";
}

// ======================================================
// UPDATE
// ======================================================
void WinPage_Update()
{
    // ENTER -> Level Select -ths
    if (AEInputCheckReleased(AEVK_RETURN))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, winGroup, 1.0f, 1.0f, 0); // -ths

        next = LEVELPAGE; // match LosePage logic -ths
        return;
    }

    // R -> Restart Level -ths
    if (AEInputCheckReleased(AEVK_R))   // restart -ths
    {
        if (gLastLevelPlayed == 1) next = GS_LEVEL1;   // -ths
        else if (gLastLevelPlayed == 2) next = GS_LEVEL2; // -ths
        else if (gLastLevelPlayed == 3) next = GS_LEVEL3; // -ths

        return;
    }

    // Q -> Quit to Main Menu -ths
    if (AEInputCheckReleased(AEVK_Q))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, winGroup, 1.0f, 1.0f, 0); // -ths

        next = MAINMENUSTATE; // quit -ths
        return;
    }
}
// ======================================================
// DRAW  (MATCHES YOUR SCREENSHOT EXACTLY)
// ======================================================
void WinPage_Draw()
{
    // Match LosePage background color -ths
    AEGfxSetBackgroundColor(0.25f, 0.07f, 0.07f); // dark red -ths

    // Title text (similar to LosePage) -ths
    AEGfxPrint(fontId,
        "YOU ESCAPED!",
        -0.18f, 0.20f,   // centered position -ths
        1.5f,            // scale -ths
        1.0f, 0.3f, 0.3f, 1.0f); // red text -ths

    // Controls (match LosePage layout) -ths
    AEGfxPrint(fontId,
        "[ENTER] Level Select",
        -0.18f, -0.05f, 1.0f,
        0.95f, 0.92f, 0.62f, 1.0f);  // beige text -ths

    AEGfxPrint(fontId,
        "[R] Restart",
        -0.12f, -0.15f, 1.0f,
        0.95f, 0.92f, 0.62f, 1.0f);  // -ths

    AEGfxPrint(fontId,
        "[Q] Quit",
        -0.08f, -0.25f, 1.0f,
        0.95f, 0.92f, 0.62f, 1.0f);  // -ths
}


// ======================================================
// FREE
// ======================================================
void WinPage_Free()
{
    std::cout << "WinPage:Free\n";
}

// ======================================================
// UNLOAD
// ======================================================
void WinPage_Unload()
{
    if (AEAudioIsValidAudio(sfxWin))
        AEAudioUnloadAudio(sfxWin);     // -ths
    if (AEAudioIsValidAudio(sfxButton))
        AEAudioUnloadAudio(sfxButton);  // -ths

    AEAudioUnloadAudioGroup(winGroup);  // -ths
}