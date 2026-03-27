#include "pch.h"

#include "IntroLogo.h"
#include "gamestatemanager.h"
#include "Main.h"
#include <iostream>

// ------------------------------------------------------------
// Variables
// ------------------------------------------------------------
AEGfxTexture* Level1instruct = nullptr;
AEGfxTexture* Level2instruct = nullptr;
AEGfxTexture* Level3instruct = nullptr;

static int instructionLevel = 1;
static int instructionNextState = GS_LEVEL1;

static float instructionTimer = 0.0f;
static const float LEVEL_INSTRUCT_TIME = 2.5f;

// ------------------------------------------------------------
// Switch case for the levels to be loaded more easily
// ------------------------------------------------------------
static int GetLevelState(int level)
{
    switch (level)
    {
    case 1: return GS_LEVEL1;
    case 2: return GS_LEVEL2;
    case 3: return GS_LEVEL3;
    default: return GS_LEVEL1;
    }
}

// ------------------------------------------------------------
// Use this function for calling of which levels to load the images
// and the timer
// ------------------------------------------------------------
void LevelInstruct(int level)
{
    instructionLevel = level; //the number that you put
    instructionNextState = GetLevelState(level); //switch the level based on the number placed
    instructionTimer = 0.0f; //start the timer
}

// ------------------------------------------------------------
// Load
// ------------------------------------------------------------
void LevelInstruct_Load()
{

    Level1instruct = AEGfxTextureLoad("Assets/Level1Instructions.png");
    Level2instruct = AEGfxTextureLoad("Assets/Level2Instructions.png");
    Level3instruct = AEGfxTextureLoad("Assets/Level3Instruct.png");

    pMesh = CreateSquareMesh();
}

// ------------------------------------------------------------
// Initialize
// ------------------------------------------------------------
void LevelInstruct_Initialize()
{
    instructionTimer = 0.0f;
}

// ------------------------------------------------------------
// Update
// ------------------------------------------------------------
void LevelInstruct_Update()
{
    instructionTimer += (float)AEFrameRateControllerGetFrameTime();

    // auto continue after timer
    if (instructionTimer >= LEVEL_INSTRUCT_TIME)
    {
        next = instructionNextState;
        return;
    }

    // continue manually
    if ( AEInputCheckTriggered(AEVK_SPACE) || //if press space or left MSB
        AEInputCheckTriggered(AEVK_LBUTTON))
    {
        next = instructionNextState;
        return;
    }

    // quit
    if (AEInputCheckReleased(AEVK_ESCAPE) ||
        0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
        return;
    }
}

// ------------------------------------------------------------
// Draw
// ------------------------------------------------------------
void LevelInstruct_Draw()
{
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    AEGfxTexture* currentInstruction = nullptr;

    switch (instructionLevel)
    {
    case 1:
        currentInstruction = Level1instruct;
        break;
    case 2:
        currentInstruction = Level2instruct;
        break;
    case 3:
        currentInstruction = Level3instruct;
        break;
    default:
        currentInstruction = Level1instruct;
        break;
    }

    if (currentInstruction)
    {
        AEMtx33 scale, trans, transform;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(currentInstruction, 0, 0);

        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);

        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    AEGfxPrint(fontId, "Press Space to skip", -0.16f, -0.80f,
        0.7f, 1.0f, 1.0f, 1.0f, 1.0f);
}

// ------------------------------------------------------------
// Free
// ------------------------------------------------------------
void LevelInstruct_Free()
{
    std::cout << "LevelInstruct:Free\n";
}

// ------------------------------------------------------------
// Unload
// ------------------------------------------------------------
void LevelInstruct_Unload()
{
    std::cout << "LevelInstruct:Unload\n";

    if (Level1instruct)
    {
        AEGfxTextureUnload(Level1instruct);
        Level1instruct = nullptr;
    }

    if (Level2instruct)
    {
        AEGfxTextureUnload(Level2instruct);
        Level2instruct = nullptr;
    }

    if (Level3instruct)
    {
        AEGfxTextureUnload(Level3instruct);
        Level3instruct = nullptr;
    }

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}