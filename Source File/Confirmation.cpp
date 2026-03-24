#include "pch.h"
#include "Confirmation.h"
#include "GameStateManager.h"
#include "MouseCoor.h"
#include "Main.h"
#include <cstring>

// ======================================================
// LOCAL VARIABLES
// ======================================================
static int  gCurrentState = 0;
static int  gNextState = 0;
static char gConfirmMessage[128] = "Are you sure?";
static bool gConfirmationActive = false;

//----------------------------------------------------------------------------
// Loads pMesh
//----------------------------------------------------------------------------
void Confirmation_Load()
{
    pMesh = CreateSquareMesh();
}

//----------------------------------------------------------------------------
// Sets up the initial state
//----------------------------------------------------------------------------
void Confirmation_Initialize() {}

//----------------------------------------------------------------------------
//Function for the transition
//----------------------------------------------------------------------------
void Confirmation_Level(int currentState, int nextState, const char* message)
{
    gCurrentState = currentState;
    gNextState = nextState;
    gConfirmationActive = true;

    if (message)
        strcpy_s(gConfirmMessage, message);
    else
        strcpy_s(gConfirmMessage, "Are you sure?");
}

//----------------------------------------------------------------------------
// Updates Selection of the user navigation
//----------------------------------------------------------------------------
void Confirmation_Update()
{
    if (!gConfirmationActive)
        return;

    // YES : go to target state
    if (AEInputCheckReleased(AEVK_Y)) //go to the next state
    {
        next = gNextState;
        gConfirmationActive = false;
        return;
    }

    // NO: go back to original state
    if (AEInputCheckReleased(AEVK_N)) //stay in the same state
    {
        next = gCurrentState;
        gConfirmationActive = false;
        return;
    }
}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
//----------------------------------------------------------------------------
void Confirmation_Draw()
{
    if (!gConfirmationActive || !pMesh)
        return;

    // Dark background
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.55f);

    AEMtx33 bgScale, bgTrans, bgMat;
    AEMtx33Scale(&bgScale, 1600.0f, 900.0f);
    AEMtx33Trans(&bgTrans, 0.0f, 0.0f);
    AEMtx33Concat(&bgMat, &bgTrans, &bgScale);

    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.55f);
    AEGfxSetTransform(bgMat.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // White popup box
    AEMtx33 boxScale, boxTrans, boxMat;
    AEMtx33Scale(&boxScale, 700.0f, 260.0f);
    AEMtx33Trans(&boxTrans, 0.0f, 0.0f);
    AEMtx33Concat(&boxMat, &boxTrans, &boxScale);

    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransform(boxMat.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);


    float w, h;

    const float msgScale = 0.85f;
    AEGfxGetPrintSize(fontId, gConfirmMessage, msgScale, &w, &h);
    AEGfxPrint(fontId, gConfirmMessage, -0.5f * w, 0.08f, msgScale,0.0f, 0.0f, 0.0f, 1.0f);

    const char* yesText = "[Y] Yes";
    const char* noText = "[N] No";
    const float btnScale = 0.8f;

    AEGfxGetPrintSize(fontId, yesText, btnScale, &w, &h);
    AEGfxPrint(fontId, yesText, -0.20f - 0.5f * w, -0.10f, btnScale, 0.0f, 0.55f, 0.0f, 1.0f);

    AEGfxGetPrintSize(fontId, noText, btnScale, &w, &h);
    AEGfxPrint(fontId, noText, 0.20f - 0.5f * w, -0.10f, btnScale, 0.75f, 0.0f, 0.0f, 1.0f);
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
//----------------------------------------------------------------------------
void Confirmation_Free() {}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
//----------------------------------------------------------------------------
void Confirmation_Unload()
{
    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}