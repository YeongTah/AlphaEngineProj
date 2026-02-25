/* Start Header ************************************************************************/
/*!
\file   Level1.cpp
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

#include "Level1.h"
#include "gamestatemanager.h"
#include <iostream>
#include <fstream>

int level1_counter = 0;

//----------------------------------------------------------------------------
// Loads Level 1 resources and reads the level counter from a text file
//---------------------------------------------------------------------------
void Level1_Load()
{
    std::cout << "Level1:Load\n"; // Print onto standard output stream

    // Read file to load level 1 counters
    std::ifstream file1("Level1_Counter.txt");
    if (file1.is_open())
    {
        file1 >> level1_counter; // Process counter
    }

}

//----------------------------------------------------------------------------
// Sets up the initial state and prepares it for gameplay
// ---------------------------------------------------------------------------
void Level1_Initialize()
{
	std::cout << "Level1:Initialize\n"; // Print onto standard output stream
}

//----------------------------------------------------------------------------
// Updates game logic and state each frame during gameplay
// ---------------------------------------------------------------------------
void Level1_Update()
{
	std::cout << "Level1:Update\n"; // Print onto standard output stream
    
    level1_counter--; // Decrement counter for level
    
    if (level1_counter == 0)
    {
        // Level 1 completed
        next = GS_LEVEL2;
    }
}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
// ---------------------------------------------------------------------------
void Level1_Draw()
{
    //====== INCLUDE RENDERING HERE ======//
    // Set the background to white.
    AEGfxSetBackgroundColor(255.0f, 255.0f, 255.0f); 
    
    std::cout << "Level1:Draw\n"; // Print onto standard output stream
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
// ---------------------------------------------------------------------------
void Level1_Free()
{
	std::cout << "Level1:Free\n"; // Print onto standard output stream
}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
// ---------------------------------------------------------------------------
void Level1_Unload()
{
	std::cout << "Level1:Unload\n"; // Print onto standard output stream
}