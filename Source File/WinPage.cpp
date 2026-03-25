// WinPage.cpp
#include "pch.h"
#include "WinPage.h"
#include "gamestatemanager.h"
#include "Main.h"
#include "Confirmation.h"
#include <iostream>

// ======================================================
// AUDIO VARIABLES (WIN PAGE)                         -ths
// ======================================================
static AEAudio sfxWin;        // -ths
static AEAudio sfxButton;     // -ths
static AEAudioGroup winGroup; // -ths

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
void WinPage_Initialize() {}

// ======================================================
// UPDATE
// ======================================================
void WinPage_Update()
{
    // ENTER -> Level Select (already with confirmation)
    if (AEInputCheckReleased(AEVK_RETURN))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, winGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_WIN, LEVELPAGE, "Are you sure you want to go to Level Select?");
        next = CONFIRM;
        return;
    }

    // R -> Restart Level (now with confirmation)
    if (AEInputCheckReleased(AEVK_R))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, winGroup, 1.0f, 1.0f, 0);
        int restartState = (gLastLevelPlayed == 1) ? GS_LEVEL1 :
            (gLastLevelPlayed == 2) ? GS_LEVEL2 : GS_LEVEL3;
        Confirmation_Level(GS_WIN, restartState, "Are you sure you want to restart the level?");
        next = CONFIRM;
        return;
    }

    // B -> Level Select (with confirmation)
    if (AEInputCheckReleased(AEVK_B))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, winGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_WIN, LEVELPAGE, "Are you sure you want to go to Level Select?");
        next = CONFIRM;
        return;
    }

    // Quit on ESC (no confirmation)
    if (AEInputCheckReleased(AEVK_ESCAPE))
        next = GS_QUIT;
}

// ======================================================
// DRAW  (centered text, matching LosePage style)
// ======================================================
void WinPage_Draw()
{
    // Match LosePage background color (dark red)
    AEGfxSetBackgroundColor(0.25f, 0.07f, 0.07f);

    float w, h;
    const float titleScale = 1.8f;
    const float lineScale = 0.75f;

    // --- Title "YOU ESCAPED!" (red, centered) ---
    const char* title = "YOU ESCAPED!";
    AEGfxGetPrintSize(fontId, title, titleScale, &w, &h);
    AEGfxPrint(fontId, title, -0.5f * w, 0.12f, titleScale, 0.65f, 0.12f, 0.12f, 1.0f); // dark red

    // --- Options (beige, centered) ---
    const char* t1 = "[ENTER] Level Select";
    AEGfxGetPrintSize(fontId, t1, lineScale, &w, &h);
    AEGfxPrint(fontId, t1, -0.5f * w, -0.02f, lineScale, 1.0f, 0.95f, 0.82f, 1.0f);

    const char* t2 = "[R] Restart";
    AEGfxGetPrintSize(fontId, t2, lineScale, &w, &h);
    AEGfxPrint(fontId, t2, -0.5f * w, -0.11f, lineScale, 1.0f, 0.95f, 0.82f, 1.0f);

    const char* t3 = "[B] Level Select";
    AEGfxGetPrintSize(fontId, t3, lineScale, &w, &h);
    AEGfxPrint(fontId, t3, -0.5f * w, -0.20f, lineScale, 1.0f, 0.95f, 0.82f, 1.0f);

    const char* t4 = "[ESCAPE] Quit";
    AEGfxGetPrintSize(fontId, t4, lineScale, &w, &h);
    AEGfxPrint(fontId, t4, -0.5f * w, -0.29f, lineScale, 1.0f, 0.95f, 0.82f, 1.0f);
}

// ======================================================
// FREE
// ======================================================
void WinPage_Free() {}

// ======================================================
// UNLOAD
// ======================================================
void WinPage_Unload()
{
    if (AEAudioIsValidAudio(sfxWin))
        AEAudioUnloadAudio(sfxWin);
    if (AEAudioIsValidAudio(sfxButton))
        AEAudioUnloadAudio(sfxButton);

    AEAudioUnloadAudioGroup(winGroup);
}