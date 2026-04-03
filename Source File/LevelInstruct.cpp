#include "pch.h"
#include "IntroLogo.h"
#include "gamestatemanager.h"
#include "Main.h"

//                                                                --- VARIABLES DECLARATION START HERE ---
AEGfxTexture* Level1instruct = nullptr;
AEGfxTexture* Level2instruct = nullptr;
AEGfxTexture* Level3instruct = nullptr;

static int instructionLevel = 1;
static int instructionNextState = GS_LEVEL1;

static float instructionTimer = 0.0f;
static const float LEVEL_INSTRUCT_TIME = 4.0f;
//                                                                --- VARIABLES DECLARATION END HERE ---

// ----------------------------------------------------------------------------
// GetLevelState
// Returns the corresponding game state for the given level number.
// Used so the instruction screen knows which level state to go to next.
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// LevelInstruct
// Sets which instruction screen to show based on the given level number.
// Also resets the timer and stores which game state to go to next.
// ----------------------------------------------------------------------------
void LevelInstruct(int level)
{
    instructionLevel = level;                  // Store which level instruction image to show
    instructionNextState = GetLevelState(level); // Store which level state to enter next
    instructionTimer = 0.0f;                  // Reset timer whenever this screen is called
}

// ----------------------------------------------------------------------------
// LevelInstruct_Load
// Loads all instruction screen textures and creates the square mesh used
// to draw the full-screen image.
// ----------------------------------------------------------------------------
void LevelInstruct_Load()
{
    Level1instruct = AEGfxTextureLoad("Assets/Level1Instructions.png");
    Level2instruct = AEGfxTextureLoad("Assets/Level2Instructions.png");
    Level3instruct = AEGfxTextureLoad("Assets/Level3Instructions.png");

    pMesh = CreateSquareMesh();
}

// ----------------------------------------------------------------------------
// LevelInstruct_Initialize
// Resets the instruction timer when this state starts.
// ----------------------------------------------------------------------------
void LevelInstruct_Initialize()
{
    instructionTimer = 0.0f;
}

// ----------------------------------------------------------------------------
// LevelInstruct_Update
// Updates the timer for the instruction screen.
//
// Behaviour:
// 1. Automatically continues to the next level after a few seconds
// 2. Allows the player to skip manually with SPACE or left mouse click
// 3. Allows quitting with ESCAPE or when the window is closed
// ----------------------------------------------------------------------------
void LevelInstruct_Update()
{
    instructionTimer += (float)AEFrameRateControllerGetFrameTime();

    // Auto continue after timer
    if (instructionTimer >= LEVEL_INSTRUCT_TIME)
    {
        next = instructionNextState;
        return;
    }

    // Continue manually
    if (AEInputCheckTriggered(AEVK_SPACE) ||
        AEInputCheckTriggered(AEVK_LBUTTON))
    {
        next = instructionNextState;
        return;
    }

    // Quit the game
    if (AEInputCheckReleased(AEVK_ESCAPE) ||
        0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;
        return;
    }
}

// ----------------------------------------------------------------------------
// LevelInstruct_Draw
// Draws the correct instruction image based on the currently selected level.
// Also prints a small message at the bottom telling the player how to skip.
// ----------------------------------------------------------------------------
void LevelInstruct_Draw()
{
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    AEGfxTexture* currentInstruction = nullptr;

    // === SELECT CURRENT INSTRUCTION IMAGE ===
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

    // === DRAW FULL SCREEN INSTRUCTION IMAGE ===
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

    // === SKIP TEXT ===
    const float textScale = 0.65f;

    float w = 0.0f, h = 0.0f;

    const char* t0 = "Press SPACE or click to skip";
    AEGfxGetPrintSize(fontId, t0, textScale, &w, &h);
    float x0 = -0.5f * w;
    AEGfxPrint(fontId, t0, x0, -0.78f, textScale, 1.0f, 1.0f, 1.0f, 1.0f);
}

// ----------------------------------------------------------------------------
// LevelInstruct_Free
// Frees state-specific runtime data if needed.
// Currently only prints a debug message.
// ----------------------------------------------------------------------------
void LevelInstruct_Free()
{
}

// ----------------------------------------------------------------------------
// LevelInstruct_Unload
// Unloads all instruction textures and frees the shared mesh used by this
// state. Also resets the pointers to nullptr after freeing.
// ----------------------------------------------------------------------------
void LevelInstruct_Unload()
{

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
    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}