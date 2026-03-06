// LosePage.cpp
#include "pch.h"
#include "LosePage.h"
#include "GameStateManager.h"
#include "Main.h"

void LosePage_Load() {}
void LosePage_Initialize() {}

void LosePage_Update()
{
    // Enter: Level Select; R: Restart; Q: Quit -ths
    if (AEInputCheckReleased(AEVK_RETURN)) next = LEVELPAGE; // -ths
    if (AEInputCheckReleased(AEVK_R)) { previous = GS_LEVEL1; next = GS_RESTART; } // -ths
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) next = GS_QUIT; // -ths
}

void LosePage_Draw()
{
    AEGfxSetBackgroundColor(0.15f, 0, 0); // red tone -ths
    AEGfxPrint(fontId, "CAUGHT BY THE MUMMY!", -0.33f, 0.10f, 2.0f, 1, 1, 1, 1); // -ths
    AEGfxPrint(fontId, "ENTER: Level Select  |  R: Restart  |  Q: Quit", -0.48f, -0.05f, 1.0f, 1, 1, 1, 1); // -ths
}

void LosePage_Free() {}
void LosePage_Unload() {}