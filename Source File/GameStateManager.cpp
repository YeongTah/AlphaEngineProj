/* Start Header ***************************************************************
\file GameStateManager.cpp
\brief Manages game state transitions: initializes the starting state and
       updates the 6 function pointers (Load, Initialize, Update, Draw, Free,
       Unload) to match whichever game state is currently active.
       The main game loop in Main.cpp calls these pointers each frame.
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#include "pch.h"
#include "GameStateManager.h"
#include "MainMenu.h"
#include "LevelPage.h"
#include "Creator.h"
#include "Credits.h"
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "System.h"
#include <iostream>

// The three state variables used by the main loop in Main.cpp:
//   current  = state whose functions are currently executing
//   previous = state that was active before the current one
//              (used by LosePage/WinPage to know which level to restart)
//   next     = state to transition to after this frame's loop ends
int current = 0, previous = 0, next = 0;

// Six function pointers set by GSM_Update() each time 'current' changes.
// Main.cpp calls these in order: Load -> Initialize -> [Update/Draw loop] -> Free -> Unload
FP fpLoad = nullptr,
fpInitialize = nullptr,
fpUpdate = nullptr,
fpDraw = nullptr,
fpFree = nullptr,
fpUnload = nullptr;

// ----------------------------------------------------------------------------
// GSM_Initialize
// Sets current, previous, and next all to 'startingState' so the main loop
// begins in that state.  Call once before the main while loop in Main.cpp.
// ----------------------------------------------------------------------------
void GSM_Initialize(int startingState)
{
    current = previous = next = startingState;
    std::cout << "GSM:Initialize\n";
}

// ----------------------------------------------------------------------------
// GSM_Update
// Called once per state entry (BEFORE the inner game loop) to wire the 6
// function pointers to the correct functions for 'current'.
//
// State routing:
//   MAINMENUSTATE -> MainMenu_*  functions
//   LEVELPAGE     -> LevelPage_* functions
//   CREATOR       -> Credit_*    functions  (credits/creator page)
//   GS_LEVEL1     -> Level1_*    functions
//   GS_LEVEL2     -> Level2_*    functions
//   GS_LEVEL3     -> Level3_*    functions
//   GS_WIN        -> WinPage_*   functions  (win overlay page)
//   GS_RESTART    -> No-op; Main.cpp already resolved current = previous before calling here
//   GS_QUIT       -> Calls System_Exit() to clean up and terminate
// ----------------------------------------------------------------------------
void GSM_Update()
{
    std::cout << "GSM:Update\n";

    switch (current)
    {
    case MAINMENUSTATE:
        fpLoad = MainMenu_Load;
        fpInitialize = MainMenu_Initialize;
        fpUpdate = MainMenu_Update;
        fpDraw = MainMenu_Draw;
        fpFree = MainMenu_Free;
        fpUnload = MainMenu_Unload;
        break;

    case LEVELPAGE: // For Level Selection Page
        fpLoad = LevelPage_Load; // Point to Main Menu Load
        fpInitialize = LevelPage_Initialize; // Point to Main Menu Initialise
        fpUpdate = LevelPage_Update; // Point to Main Menu Update
        fpDraw = LevelPage_Draw; // Point to Main Menu Draw
        fpFree = LevelPage_Free; // Point to Main Menu Free
        fpUnload = LevelPage_Unload; // Point to Main Menu Unload
        break;
    case CREDIT: // For Credit Page
        fpLoad = Credit_Load; // Point to Credit Load
        fpInitialize = Credit_Initialize; // Point to Credit Initialise
        fpUpdate = Credit_Update; // Point to Credit Update
        fpDraw = Credit_Draw; // Point to Credit Draw
        fpFree = Credit_Free; // Point to Credit Free
        fpUnload = Credit_Unload; // Point to Credit Unload
        break;
    case CREATOR: // For Creator Page
        fpLoad = Creator_Load; // Point to Creator Load
        fpInitialize = Creator_Initialize; // Point to Creator Initialise
        fpUpdate = Creator_Update; // Point to Creator Update
        fpDraw = Creator_Draw; // Point to Creator Draw
        fpFree = Creator_Free; // Point to Creator Free
        fpUnload = Creator_Unload; // Point to Creator Unload
        break;
    case GS_LEVEL1:
        fpLoad = Level1_Load;
        fpInitialize = Level1_Initialize;
        fpUpdate = Level1_Update;
        fpDraw = Level1_Draw;
        fpFree = Level1_Free;
        fpUnload = Level1_Unload;
        break;

    case GS_LEVEL2:
        fpLoad = Level2_Load;
        fpInitialize = Level2_Initialize;
        fpUpdate = Level2_Update;
        fpDraw = Level2_Draw;
        fpFree = Level2_Free;
        fpUnload = Level2_Unload;
        break;

    case GS_LEVEL3:
        fpLoad = Level3_Load;
        fpInitialize = Level3_Initialize;
        fpUpdate = Level3_Update;
        fpDraw = Level3_Draw;
        fpFree = Level3_Free;
        fpUnload = Level3_Unload;
        break;

    case GS_WIN:
        // Win page -- shown after the player reaches the exit portal in any level
        fpLoad = WinPage_Load;
        fpInitialize = WinPage_Initialize;
        fpUpdate = WinPage_Update;
        fpDraw = WinPage_Draw;
        fpFree = WinPage_Free;
        fpUnload = WinPage_Unload;
        break;

    case GS_RESTART:
        // Main.cpp resolves GS_RESTART to the actual level (current = previous) BEFORE
        // calling GSM_Update, so by the time we reach this case 'current' is already the
        // level that should restart.  The level's own case above will set the pointers.
        break;

    case GS_QUIT:
        System_Exit(); // Perform engine/system cleanup then let the loop exit
        break;

    default:
        break;
    }
}