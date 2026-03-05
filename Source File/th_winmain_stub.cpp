// th_winmain_stub.cpp - Windows entry point for Mummy Maze
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include "AEEngine.h"
#include "gs_play.h"

#ifdef UNICODE
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
#endif
{
    // Initialize random seed
    srand(static_cast<unsigned int>(time(nullptr)));

    // Game configuration
    const s32  createConsole = 1;     // Show console for debugging
    const bool enableVSync = true;  // Enable vertical sync
    const s32  windowWidth = 800;
    const s32  windowHeight = 600;
    const u32  maxFrameRate = 60;

    // Initialize Alpha Engine
    if (!AESysInit(hInstance, nCmdShow, windowWidth, windowHeight,
        createConsole, maxFrameRate, enableVSync, nullptr)) {
        return 1;
    }

    AESysSetWindowTitle("Mummy Maze - Simple Version");
    AEGfxSetVSync(1);

    GS_PlayLoad();
    GS_PlayInit();

    while (AESysDoesWindowExist()) {
        AESysFrameStart();
        if (AEInputCheckTriggered(AEVK_ESCAPE)) break; // quit
        GS_PlayUpdate();
        GS_PlayDraw();
        AESysFrameEnd();
    }

    GS_PlayFree();
    GS_PlayUnload();
    AESysExit();
    return 0;
}
