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

//Drawing Rectangle for buttons-------------
static void DrawRect(float centre_x, float centre_y, float width, float height, float r, float g, float b)
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(r, g, b, 1.0f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);

    AEMtx33 Scale, Transform, ConTrans;
    AEMtx33Scale(&Scale, width, height);
    AEMtx33Trans(&Transform, centre_x, centre_y);
    AEMtx33Concat(&ConTrans, &Transform, &Scale);

    AEGfxSetTransform(ConTrans.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
}

enum ConfirmAction
{
    YES = 0,NO
};

static struct
{
    float x, y, w, h;
    const char* text;
    int action;
} 

confirmButtons[] =
{
    { -120.0f, -40.0f, 180.0f, 60.0f, "Yes", YES },
    {  120.0f, -40.0f, 180.0f, 60.0f, "No",  NO  }
};

static const int confirmBtnCount = sizeof(confirmButtons) / sizeof(confirmButtons[0]);

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

    int mouseX, mouseY;
    TransformScreentoWorld(mouseX, mouseY);

    if (AEInputCheckReleased(AEVK_LBUTTON))
    {
        for (int i = 0; i < confirmBtnCount; ++i)
        {
            if (IsAreaClicked(confirmButtons[i].x, confirmButtons[i].y, confirmButtons[i].w, confirmButtons[i].h, mouseX, mouseY)) {
                switch (confirmButtons[i].action)
                {
                case YES:
                    next = gNextState;
                    gConfirmationActive = false;
                    return;

                case NO:
                    next = gCurrentState;
                    gConfirmationActive = false;
                    return;
                }
                return;
            }
        }
    }
}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
//----------------------------------------------------------------------------
void Confirmation_Draw()
{
    if (!gConfirmationActive || !pMesh)
        return;

    //AEGfxSetBackgroundColor(0.1f, 0.1f, 0.1f);

    DrawRect(0.0f, 0.0f, 700.0f, 250.0f, 1.0f, 1.0f, 1.0f); //white box

    float w, h;
    const float msgScale = 0.9f;
    AEGfxGetPrintSize(fontId, gConfirmMessage, msgScale, &w, &h);
    AEGfxPrint(fontId, gConfirmMessage, -0.5f * w, 0.10f, msgScale, 0.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < confirmBtnCount; ++i)
    {
        if (confirmButtons[i].action == YES) {
            DrawRect(confirmButtons[i].x, confirmButtons[i].y,confirmButtons[i].w, confirmButtons[i].h, 0.2f, 0.7f, 0.2f);
        }
        else {
            DrawRect(confirmButtons[i].x, confirmButtons[i].y, confirmButtons[i].w, confirmButtons[i].h, 0.8f, 0.2f, 0.2f);
        }

        float btnCenterNDCX = confirmButtons[i].x / 800.0f;
        float btnCenterNDCY = confirmButtons[i].y / 450.0f;

        float textW, textH;
        const float textScale = 0.8f;
        AEGfxGetPrintSize(fontId, confirmButtons[i].text, textScale, &textW, &textH);

        float leftX = btnCenterNDCX - textW * 0.5f;
        float baseY = btnCenterNDCY + textH * 0.5f;

        AEGfxPrint(fontId, confirmButtons[i].text, leftX, baseY, textScale,
            1.0f, 1.0f, 1.0f, 1.0f);
    }
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