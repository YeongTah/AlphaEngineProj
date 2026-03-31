// LosePage.cpp
#include "pch.h"
#include "LosePage.h"
#include "GameStateManager.h"
#include "Main.h"
#include "leveleditor.hpp"
#include "Confirmation.h"

static AEAudio sfxButton;
static AEAudioGroup loseGroup;

static void DrawRect(float centre_x, float centre_y, float width, float height,
    float r, float g, float b)
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

static struct {
    float x, y, w, h;
    const char* text;
    int action; // 0=LevelSelect, 1=Restart, 2=Quit
} loseButtons[] = {
    {    0.0f,  60.0f, 280.0f, 70.0f, "Level Select", 0 },
    {    0.0f, -20.0f, 280.0f, 70.0f, "Restart", 1 },
    {    0.0f,-100.0f, 280.0f, 70.0f, "Quit", 2 }
};
static const int loseBtnCount = sizeof(loseButtons) / sizeof(loseButtons[0]);

// ----------------------------------------------------------------------------
void LosePage_Load()
{
    pMesh = CreateSquareMesh();
    loseGroup = AEAudioCreateGroup();
    sfxButton = AEAudioLoadSound("Assets/audio/button.wav");
}

void LosePage_Initialize() {}

void LosePage_Update()
{
    // Keyboard fallback
    if (AEInputCheckReleased(AEVK_RETURN))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, loseGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_LOSE, LEVELPAGE, "Are you sure you want to go to Level Select?");
        next = CONFIRM;
        return;
    }

    if (AEInputCheckReleased(AEVK_R))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, loseGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_LOSE, previous, "Are you sure you want to restart the level?");
        next = CONFIRM;
        return;
    }

    if (AEInputCheckReleased(AEVK_B))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, loseGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_LOSE, LEVELPAGE, "Are you sure you want to go to Level Select?");
        next = CONFIRM;
        return;
    }

    if (AEInputCheckReleased(AEVK_ESCAPE))
    {
        // Show confirmation before quitting
        Confirmation_Level(GS_LOSE, GS_QUIT, "Are you sure you want to quit the game?");
        next = CONFIRM;
        return;
    }

    if (0 == AESysDoesWindowExist())
    {
        next = GS_QUIT;   // immediate quit if window is already closed
        return;
    }

    // --- Mouse click detection with audio ---
    s32 mouseX, mouseY;
    TransformScreentoWorld(mouseX, mouseY);
    if (AEInputCheckReleased(AEVK_LBUTTON))
    {
        for (int i = 0; i < loseBtnCount; ++i)
        {
            if (IsAreaClicked(loseButtons[i].x, loseButtons[i].y,
                loseButtons[i].w, loseButtons[i].h, mouseX, mouseY))
            {
                if (AEAudioIsValidAudio(sfxButton))
                    AEAudioPlay(sfxButton, loseGroup, 1.0f, 1.0f, 0);

                switch (loseButtons[i].action)
                {
                case 0: // Level Select
                    Confirmation_Level(GS_LOSE, LEVELPAGE, "Are you sure you want to go to Level Select?");
                    next = CONFIRM;
                    break;
                case 1: // Restart
                    Confirmation_Level(GS_LOSE, previous, "Are you sure you want to restart the level?");
                    next = CONFIRM;
                    break;
                case 2: // Quit
                    Confirmation_Level(GS_LOSE, GS_QUIT, "Are you sure you want to quit the game?");
                    next = CONFIRM;
                    break;
                }
                return; // Important: exit after handling click
            }
        }
    }
}

void LosePage_Draw()
{
    // Background color (dark red)
    AEGfxSetBackgroundColor(0.25f, 0.07f, 0.07f);

    // Title
    const char* title = "CAUGHT BY THE MUMMY!";
    float w, h;
    const float titleScale = 1.8f;
    AEGfxGetPrintSize(fontId, title, titleScale, &w, &h);
    AEGfxPrint(fontId, title, -0.5f * w, 0.45f, titleScale, 0.65f, 0.12f, 0.12f, 1.0f);

    // Draw buttons
    for (int i = 0; i < loseBtnCount; ++i)
    {
        DrawRect(loseButtons[i].x, loseButtons[i].y,
            loseButtons[i].w, loseButtons[i].h,
            0.5f, 0.5f, 0.5f);

        float btnCenterNDCX = loseButtons[i].x / 800.0f;
        float btnCenterNDCY = loseButtons[i].y / 450.0f;

        float textW, textH;
        const float textScale = 0.9f;
        AEGfxGetPrintSize(fontId, loseButtons[i].text, textScale, &textW, &textH);
        float leftX = btnCenterNDCX - textW * 0.5f;
        float baselineY = btnCenterNDCY + textH * 0.5f;
        AEGfxPrint(fontId, loseButtons[i].text, leftX, baselineY, textScale,
            1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Keyboard shortcuts footer
    const float helpScale = 0.65f;
    const char* help = "Keyboard: [ENTER] Level Select | [R] Restart | [B] Level Select | [ESC] Quit";
    float helpW, helpH;
    AEGfxGetPrintSize(fontId, help, helpScale, &helpW, &helpH);
    AEGfxPrint(fontId, help, -0.5f * helpW, -0.85f, helpScale, 0.8f, 0.8f, 0.8f, 1.0f);
}

void LosePage_Free() {}

void LosePage_Unload()
{
    if (AEAudioIsValidAudio(sfxButton))
        AEAudioUnloadAudio(sfxButton);
    AEAudioUnloadAudioGroup(loseGroup);

    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}