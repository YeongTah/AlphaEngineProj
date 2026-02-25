// ---------------------------------------------------------------------------
// includes
#include "pch.h"
#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include <iostream>
#include <fstream>


#include "GameStateManager.h"
#include "System.h"


// ---------------------------------------------------------------------------
// main

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialization of your own variables go here

    // Initialize system components
    System_Initialize();

    // Initialize game state manager
    GSM_Initialize(MAINMENUSTATE);

    // Using custom window procedure
    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);

    // Changing the window title
    AESysSetWindowTitle("My New Demo2!");

    // reset the system modules
    //

    printf("Hello World\n");


    // Game State Manager Loop
    while (current != GS_QUIT)
    {
        AESysReset();

        // Your own update logic goes here

        // If current game state is not equal to restart (game loop)
        if (current != GS_RESTART)
        {
            // Call "Update" for current level


            GSM_Update();  // This handles state switching and function pointer setup
            fpLoad(); // Load current level
        }
        else
        {
            // Set next game state to be equal to previous game state
            next = previous;
            // set current game state to be equal to previous game state
            current = previous;
        }

        // Call "init" for current game state
        fpInitialize();

        // While next game state is equal to current game state
        while (next == current)
        {
            // Informing the system about the loop's start
            AESysFrameStart();

            // Update input status by calling input function
            std::cout << "Input\n"; // Debug purposes

            // Call update for current level
            fpUpdate();

            // Call "draw" for current level
            fpDraw();

            // Informing the system about the loop's end
            AESysFrameEnd();

        }

        // Call "free" for current level
        fpFree();

        // If next game state is not equal to restart
        if (next != GS_RESTART)
        {
            // Call "unload" for current level
            fpUnload();
        }

        // Set previous game state to be equal to current game state
        previous = current;
        // Set current game state to be equal to next game state
        current = next;

        // Your own rendering logic goes here

    }

    // IF THERE IS NO OTHER MANAGERS BUILT, CAN REMOVE THIS AS IT DOES THE SAME AS AESysExit
    // Terminate system components
    System_Exit();

    // free the system
    AESysExit();
}