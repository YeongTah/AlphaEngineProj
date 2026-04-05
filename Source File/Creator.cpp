/* Start Header ***************************************************************
\file       Creator.cpp
\coders     Jasmine
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#include "pch.h"
#include "leveleditor.hpp"
#include "Confirmation.h"
#include "Creator.h"
#include "main.h"
#include "gamestatemanager.h"

// ----------------------------------------------------------------------------
// Creator_Load
// Loads the textures and mesh needed for the Creator state.
// Also reads the level file and writes it back once during load.
// ----------------------------------------------------------------------------
void Creator_Load()
{
    gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");
    pMesh = CreateSquareMesh();

    readfile();
    print_file();
}

// ----------------------------------------------------------------------------
// Creator_Initialize
// Sets up the Creator state when it starts.
// ----------------------------------------------------------------------------
void Creator_Initialize()
{
}

// ----------------------------------------------------------------------------
// Creator_Update
// Updates input handling for the Creator state.
//
// Behaviour:
// 1. Pressing B returns to the main menu
// 2. Pressing ESCAPE or closing the window quits the game
// ----------------------------------------------------------------------------
void Creator_Update()
{
    // Move back to main menu upon triggering "B"
    if (AEInputCheckReleased(AEVK_B))
    {
        next = MAINMENUSTATE;
       
    }

    // Quit game when ESC is hit or when the window is closed
    if (AEInputCheckReleased(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
    {
        Confirmation_Level(CREATOR, GS_QUIT, "Are you sure you want to quit?");
        next = CONFIRM;
    }
}

// ----------------------------------------------------------------------------
// Creator_Draw
// Draws the Creator state each frame by setting the background colour and
// calling the level editor drawing / update logic.
// ----------------------------------------------------------------------------
void Creator_Draw()
{
    AEGfxSetBackgroundColor(0.30f, 0.22f, 0.12f);
    generateLevel();
}

// ----------------------------------------------------------------------------
// Creator_Free
// Cleans up runtime data for the Creator state
// ----------------------------------------------------------------------------
void Creator_Free()
{
}

// ----------------------------------------------------------------------------
// Creator_Unload
// Unloads resources used by the Creator state
// ----------------------------------------------------------------------------
void Creator_Unload()
{
    Editor_Unload();

    if (gDesertBlockTex)
    {
        AEGfxTextureUnload(gDesertBlockTex);
        gDesertBlockTex = nullptr;
    }

    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}