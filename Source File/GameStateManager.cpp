/* Start Header ***************************************************************
/*!
\file GameStateManager.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par sharonjooai.lim@digipen.edu
\date January, 26, 2026
\brief This file implements the game state transitions through initializing and updating.
Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
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
#include "System.h"
#include <iostream>

// 3 variables for the states
int current = 0, previous = 0, next = 0;
FP fpLoad = nullptr, fpInitialize = nullptr, fpUpdate = nullptr, fpDraw = nullptr, fpFree = nullptr, fpUnload = nullptr;

//----------------------------------------------------------------------------
// Initializes the game state manager with the starting state and prints to
// standard output stream
// ---------------------------------------------------------------------------
void GSM_Initialize(int startingState)
{
    // Set all states to initial value
    current = previous = next = startingState;
    // Print onto standard output stream
    std::cout << "GSM:Initialize\n";
}

//----------------------------------------------------------------------------
// Updates game state manager by handling state transitions, calls appropriate
// level functions, and manages the game loop for the current state
// ---------------------------------------------------------------------------
void GSM_Update()
{
    // Print onto standard output stream
    std::cout << "GSM:Update\n";

    // Set up 6 function pointers to currently selected stage
    switch (current) // Based on current state
    {
    case MAINMENUSTATE: // For Main Menu
        fpLoad = MainMenu_Load;      // Point to Main Menu Load
        fpInitialize = MainMenu_Initialize; // Point to Main Menu Initialise
        fpUpdate = MainMenu_Update;  // Point to Main Menu Update
        fpDraw = MainMenu_Draw;      // Point to Main Menu Draw
        fpFree = MainMenu_Free;      // Point to Main Menu Free
        fpUnload = MainMenu_Unload;  // Point to Main Menu Unload
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

    case GS_LEVEL1: // For level 1
        fpLoad = Level1_Load;        // Point to Level 1 Load
        fpInitialize = Level1_Initialize; // Point to Level 1 Initialize
        fpUpdate = Level1_Update;    // Point to Level 1 Update
        fpDraw = Level1_Draw;        // Point to Level 1 Draw
        fpFree = Level1_Free;        // Point to Level 1 Free
        fpUnload = Level1_Unload;    // Point to Level 1 Unload
        break;

    case GS_LEVEL2: // For level 2
        fpLoad = Level2_Load;        // Point to Level 2 Load
        fpInitialize = Level2_Initialize; // Point to Level 2 Initialize
        fpUpdate = Level2_Update;    // Point to Level 2 Update
        fpDraw = Level2_Draw;        // Point to Level 2 Draw
        fpFree = Level2_Free;        // Point to Level 2 Free
        fpUnload = Level2_Unload;    // Point to Level 2 Unload
        break;

    case GS_WIN: // Add  to trigger the win state
        fpLoad = WinPage_Load;
        fpInitialize = WinPage_Initialize;
        fpUpdate = WinPage_Update;
        fpDraw = WinPage_Draw;
        fpFree = WinPage_Free;
        fpUnload = WinPage_Unload;
        break;

    case GS_RESTART: // For restart
        /* Main.cpp syncs current = next (= previous) BEFORE calling GSM_Update again.
           By the time we get here, current already equals the level to restart.
           Nothing to do - the level's own case above will set the function pointers. */
        break;

    case GS_QUIT: // For exiting
        System_Exit();
        break;

    default:
        break;
    }

    /* ADDITIONAL NOTES (no code changes needed) -ths
       ------------------------------------------------
       � Pause/Win/Lose overlays are rendered and controlled from
         Level1_Update()/Level1_Draw() (see comments tagged -ths there),
         so GSM does not need extra states for them. -ths
       � If in the future you want dedicated Pause/Win/Lose pages,
         we can add GS_PAUSE / GS_WIN / GS_LOSE to GameStateList.h
         and mirror the Level1 wiring above with new pages. -ths
       ------------------------------------------------ -ths
    */
}