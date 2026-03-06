// WinPage.cpp
#include "pch.h"
#include "WinPage.h"
#include "GameStateManager.h"
#include "Level1.h"
#include "Main.h"

void WinPage_Load() {}
void WinPage_Initialize() {}

void WinPage_Update()
{
    // Enter: back to Level Select, R: restart Level1, Q: quit -ths
    if (AEInputCheckReleased(AEVK_RETURN)) next = LEVELPAGE; // -ths
    if (AEInputCheckReleased(AEVK_R)) { previous = GS_LEVEL1; next = GS_RESTART; } // -ths
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) next = GS_QUIT; // -ths
}

void WinPage_Draw()
{
    AEGfxSetBackgroundColor(0, 0.2f, 0); // green tone -ths
    AEGfxPrint(fontId, "YOU ESCAPED!", -0.18f, 0.10f, 2.0f, 1, 1, 1, 1); // -ths
    AEGfxPrint(fontId, "ENTER: Level Select  |  R: Restart  |  Q: Quit", -0.48f, -0.05f, 1.0f, 1, 1, 1, 1); // -ths
}

void WinPage_Free() {}
void WinPage_Unload() {}