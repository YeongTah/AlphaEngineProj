
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
void Credit_Load()
{
    std::cout << "Credit:Load\n"; // Debug purposes
    gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    pMesh = CreateSquareMesh();



    readfile();
    print_file();

}

//----------------------------------------------------------------------------
// Sets up the initial state
// ---------------------------------------------------------------------------
void Credit_Initialize()
{
    std::cout << "Credit:Initialize\n"; // Debug purposes

}

//----------------------------------------------------------------------------
// Updates Level Selection Page navigation
// ---------------------------------------------------------------------------
void Credit_Update()
{

    std::cout << "Credit:Update\n"; // Debug purposes

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
void Credit_Draw()
{
    std::cout << "Credit:Draw\n"; // Debug purposes
    AEGfxSetBackgroundColor(0.30f, 0.22f, 0.12f);
    generateLevel();
    
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
// ---------------------------------------------------------------------------
void Credit_Free()
{
    std::cout << "Credit:Free\n"; // Debug purposes

}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
// ---------------------------------------------------------------------------
void Credit_Unload()
{
    std::cout << "Credit:Unload\n"; // Debug purposes

}