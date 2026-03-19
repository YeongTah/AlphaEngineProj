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
#include "IntroLogo.h"
#include "MainMenu.h"
#include "LevelPage.h"
#include "Creator.h"
#include "Credits.h"
#include "Instructions.h"
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "PausePage.h"   // added for GS_PAUSE -ths
#include "System.h"
#include <iostream>

// Tracks which level sent the player to the Win Page -ths
int gLastLevelPlayed = 1;   // default = Level 1

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
//   CREATOR       -> Creator_*    functions
//   CREDIT        -> Credit_*     functions
//   INSTRUCTIONS  -> Instructions_* functions
//   GS_LEVEL1     -> Level1_*    functions
//   GS_LEVEL2     -> Level2_*    functions
//   GS_LEVEL3     -> Level3_*    functions
//   GS_WIN        -> WinPage_*   functions
//   GS_PAUSE      -> PausePage_* functions   <-- added -ths
//   GS_RESTART    -> No-op; Main.cpp resolves state before this call
//   GS_QUIT       -> System_Exit()
// ----------------------------------------------------------------------------
void GSM_Update()
{
    std::cout << "GSM:Update\n";

    switch (current)
    {
    case INTROSTATE: // For Intro screen
        fpLoad = Intro_Load;
        fpInitialize = Intro_Initialize;
        fpUpdate = Intro_Update;
        fpDraw = Intro_Draw;
        fpFree = Intro_Free;
        fpUnload = Intro_Unload;
        break;

    case MAINMENUSTATE: // For Main Menu
        fpLoad = MainMenu_Load;
        fpInitialize = MainMenu_Initialize;
        fpUpdate = MainMenu_Update;
        fpDraw = MainMenu_Draw;
        fpFree = MainMenu_Free;
        fpUnload = MainMenu_Unload;
        break;

    case LEVELPAGE: // For Level Selection Page
        fpLoad = LevelPage_Load;
        fpInitialize = LevelPage_Initialize;
        fpUpdate = LevelPage_Update;
        fpDraw = LevelPage_Draw;
        fpFree = LevelPage_Free;
        fpUnload = LevelPage_Unload;
        break;

    case CREDIT: // Credit Page
        fpLoad = Credit_Load;
        fpInitialize = Credit_Initialize;
        fpUpdate = Credit_Update;
        fpDraw = Credit_Draw;
        fpFree = Credit_Free;
        fpUnload = Credit_Unload;
        break;

    case INSTRUCTIONS: // Instructions Page
        fpLoad = Instructions_Load;
        fpInitialize = Instructions_Initialize;
        fpUpdate = Instructions_Update;
        fpDraw = Instructions_Draw;
        fpFree = Instructions_Free;
        fpUnload = Instructions_Unload;
        break;

    case CREATOR: // Creator Page
        fpLoad = Creator_Load;
        fpInitialize = Creator_Initialize;
        fpUpdate = Creator_Update;
        fpDraw = Creator_Draw;
        fpFree = Creator_Free;
        fpUnload = Creator_Unload;
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
        fpLoad = WinPage_Load;
        fpInitialize = WinPage_Initialize;
        fpUpdate = WinPage_Update;
        fpDraw = WinPage_Draw;
        fpFree = WinPage_Free;
        fpUnload = WinPage_Unload;
        break;

        // ============================================================================
        // NEW PAUSE STATE (PausePage) -ths
        // ============================================================================
    case GS_PAUSE:
        fpLoad = PausePage_Load;       // -ths
        fpInitialize = PausePage_Initialize; // -ths
        fpUpdate = PausePage_Update;     // -ths
        fpDraw = PausePage_Draw;       // -ths
        fpFree = PausePage_Free;       // -ths
        fpUnload = PausePage_Unload;     // -ths
        break;

    case GS_RESTART:
        // Main.cpp resolves GS_RESTART to the actual level before calling here.
        break;

    case GS_QUIT:
        System_Exit(); // cleanup
        break;

    default:
        break;
    }
}