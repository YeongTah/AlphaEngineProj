#include "pch.h"
#include "Confirmation.h"
#include "GameStateManager.h"
#include "MouseCoor.h"
#include "Main.h"
#include <cstring>

//                                                                --- VARIABLES DECLARATION START HERE ---
static int  gCurrentState = 0;
static int  gNextState = 0;
static char gConfirmMessage[128] = "Are you sure?";
static bool gConfirmationActive = false;
//                                                                --- VARIABLES DECLARATION END HERE ---

// ----------------------------------------------------------------------------
// DrawRect
// Draws a solid coloured rectangle using the shared square mesh.
// Used for the confirmation box background and the Yes / No buttons.
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// ConfirmAction
// Identifies the action associated with each confirmation button.
// ----------------------------------------------------------------------------
enum ConfirmAction
{
    YES = 0,
    NO
};

// Button data for the confirmation popup
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

// ----------------------------------------------------------------------------
// Confirmation_Load
// Creates the square mesh used to draw the confirmation popup.
// ----------------------------------------------------------------------------
void Confirmation_Load()
{
    pMesh = CreateSquareMesh();
}

// ----------------------------------------------------------------------------
// Confirmation_Initialize
// Sets up the confirmation state when it starts.
// ----------------------------------------------------------------------------
void Confirmation_Initialize() {}

// ----------------------------------------------------------------------------
// Confirmation_Level
// Stores the current state, the next state to go to on confirmation,
// and the message to show in the confirmation popup.
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// Confirmation_Update
// Handles input for the confirmation popup.
//
// Behaviour:
// 1. If the confirmation popup is not active, do nothing
// 2. If the left mouse button is released, check whether a button was clicked
// 3. YES sends the game to the next state
// 4. NO returns the game to the current state
// ----------------------------------------------------------------------------
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
            if (IsAreaClicked(confirmButtons[i].x, confirmButtons[i].y,
                confirmButtons[i].w, confirmButtons[i].h, mouseX, mouseY))
            {
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

// ----------------------------------------------------------------------------
// Confirmation_Draw
// Draws the confirmation popup when it is active, including:
// 1. The white background box
// 2. The confirmation message
// 3. The Yes / No buttons
// 4. Centered text inside each button
// ----------------------------------------------------------------------------
void Confirmation_Draw()
{
    if (!gConfirmationActive || !pMesh)
        return;

    // === CONFIRMATION BOX ===
    DrawRect(0.0f, 0.0f, 700.0f, 250.0f, 1.0f, 1.0f, 1.0f); // white box

    // === CONFIRMATION MESSAGE ===
    float w, h;
    const float msgScale = 0.9f;
    AEGfxGetPrintSize(fontId, gConfirmMessage, msgScale, &w, &h);
    AEGfxPrint(fontId, gConfirmMessage, -0.5f * w, 0.10f, msgScale, 0.0f, 0.0f, 0.0f, 1.0f);

    // === BUTTONS ===
    for (int i = 0; i < confirmBtnCount; ++i)
    {
        // Draw button rectangle with colour based on action
        if (confirmButtons[i].action == YES)
        {
            DrawRect(confirmButtons[i].x, confirmButtons[i].y,
                confirmButtons[i].w, confirmButtons[i].h,
                0.2f, 0.7f, 0.2f);
        }
        else
        {
            DrawRect(confirmButtons[i].x, confirmButtons[i].y,
                confirmButtons[i].w, confirmButtons[i].h,
                0.8f, 0.2f, 0.2f);
        }

        // Convert button centre from world coordinates to NDC for text printing
        float btnCenterNDCX = confirmButtons[i].x / 800.0f;
        float btnCenterNDCY = confirmButtons[i].y / 450.0f;

        // Measure text size so it can be centered inside the button
        float textW, textH;
        const float textScale = 0.8f;
        AEGfxGetPrintSize(fontId, confirmButtons[i].text, textScale, &textW, &textH);

        float leftX = btnCenterNDCX - textW * 0.5f;
        float baseY = btnCenterNDCY + textH * 0.5f;

        AEGfxPrint(fontId, confirmButtons[i].text, leftX, baseY, textScale,
            1.0f, 1.0f, 1.0f, 1.0f);
    }
}

// ----------------------------------------------------------------------------
// Confirmation_Free
// Cleans up runtime data for the confirmation state if needed.
// ----------------------------------------------------------------------------
void Confirmation_Free() {}

// ----------------------------------------------------------------------------
// Confirmation_Unload
// Frees the square mesh used by the confirmation popup.
// ----------------------------------------------------------------------------
void Confirmation_Unload()
{
    if (pMesh) {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}