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
    const s32 createConsole = 1;      // Show console for debugging
    const bool enableVSync = true;    // Enable vertical sync
    const s32 windowWidth = 800;      // Window width
    const s32 windowHeight = 600;     // Window height
    const u32 maxFrameRate = 60;      // Maximum FPS

    // Initialize Alpha Engine
    if (!AESysInit(hInstance, nCmdShow, windowWidth, windowHeight,
        createConsole, maxFrameRate, enableVSync, nullptr)) {
        return 1; // Initialization failed
    }

    // Set window title
    AESysSetWindowTitle("Mummy Maze - Simple Version");

    // Enable VSync
    AEGfxSetVSync(1);

    // Initialize game state
    GS_PlayLoad();
    GS_PlayInit();

    // Main game loop
    while (AESysDoesWindowExist()) {
        // Start of frame
        AESysFrameStart();

        // Check for ESC key to exit
        if (AEInputCheckTriggered(AEVK_ESCAPE)) {
            break;
        }

        // Update game state
        GS_PlayUpdate();

        // Draw game state
        GS_PlayDraw();

        // End of frame
        AESysFrameEnd();
    }

    // Cleanup
    GS_PlayFree();
    GS_PlayUnload();
    AESysExit();

    return 0;
}