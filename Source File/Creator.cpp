
#include "pch.h"
#include "leveleditor.hpp"
#include "Creator.h"
#include "main.h"
#include "gamestatemanager.h"
#include <iostream>
#include <fstream>

//----------------------------------------------------------------------------
// Loads Main Menu
//---------------------------------------------------------------------------
void Creator_Load()
{
    std::cout << "Creator:Load\n"; // Debug purposes
    gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    pMesh = CreateSquareMesh();



    readfile();
    print_file();

}

//----------------------------------------------------------------------------
// Sets up the initial state
// ---------------------------------------------------------------------------
void Creator_Initialize()
{
    std::cout << "Creator:Initialize\n"; // Debug purposes

}

//----------------------------------------------------------------------------
// Updates Level Selection Page navigation
// ---------------------------------------------------------------------------
void Creator_Update()
{

    std::cout << "Creator:Update\n"; // Debug purposes

    // Move back to main menu upon triggering "B"
    if (AEInputCheckReleased(AEVK_B) || (AEInputCheckReleased(AEVK_ESCAPE)))
    {
        next = MAINMENUSTATE;
        std::cout << "Back key Released" << '\n'; // Debug purposes
    }

    // Quit game when Q is hit or when the window is closed
    if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
        std::cout << "Q key Released" << '\n'; // Debug purposes
    }
    

}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
// ---------------------------------------------------------------------------
void Creator_Draw()
{
    std::cout << "Creator:Draw\n"; // Debug purposes
    AEGfxSetBackgroundColor(0.30f, 0.22f, 0.12f);
    generateLevel();
    
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
// ---------------------------------------------------------------------------
void Creator_Free()
{
    std::cout << "Creator:Free\n"; // Debug purposes

}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
// ---------------------------------------------------------------------------
void Creator_Unload()
{
    std::cout << "Creator:Unload\n"; // Debug purposes

}