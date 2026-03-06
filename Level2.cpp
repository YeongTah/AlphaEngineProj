/* Start Header ************************************************************************/
/*!
\file   Level2.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  This file defines the function Load, Initialize, Update, Draw, Free, Unload
        to produce the level in the game and manage their own counters loaded from text
        files.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"

#include "Level2.h"
#include "gamestatemanager.h"
#include <iostream>
#include <fstream>


int level2_counter = 0;
int live2_counter = 0;

//----------------------------------------------------------------------------
// Loads Level 2 resources and reads the level counter from a text file
// ---------------------------------------------------------------------------
void Level2_Load()
{
    std::cout << "Level2:Load\n"; // Print onto standard output stream

    // Read file to load level 2 lives
    std::ifstream file3("Level2_Lives.txt");
    if (file3.is_open())
    {
        file3 >> live2_counter; // Process counter
    }
}

//----------------------------------------------------------------------------
// Sets up the initial state and prepares it for gameplay
// ---------------------------------------------------------------------------
void Level2_Initialize()
{
    std::cout << "Level2:Initialize\n"; // Print onto standard output stream

    // Read file to load level 2 counters
    std::ifstream file2("Level2_Counter.txt");
    if (file2.is_open())
    {
        file2 >> level2_counter; // Process counter
    }
}

//----------------------------------------------------------------------------
// Updates game logic and state each frame during gameplay
// ---------------------------------------------------------------------------
void Level2_Update()
{
    std::cout << "Level2:Update\n"; // Print onto standard output stream
    
    level2_counter--; // Decrement counter for level

    if (level2_counter <= 0)
    {
        live2_counter--; // Decrement life
        // Level 2 iteration completed
        if (live2_counter > 0)
        {
            next = GS_RESTART; // Will restart Level 2
        }
        else
        {
            next = MAINMENUSTATE; // No lives left, quit, sharon 4/3: go to main menu instead of quitting for the time being so can debug the game properly
        }
    }
}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
// ---------------------------------------------------------------------------
void Level2_Draw()
{
    std::cout << "Level2:Draw\n"; // Print onto standard output stream
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
// ---------------------------------------------------------------------------
void Level2_Free()
{
    std::cout << "Level2:Free\n"; // Print onto standard output stream
}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
// ---------------------------------------------------------------------------
void Level2_Unload()
{
    std::cout << "Level2:Unload\n"; // Print onto standard output stream
}