// PausePage.cpp
#include "pch.h"
#include "PausePage.h"
#include "GameStateManager.h"
#include "Main.h"
#include "leveleditor.hpp"
#include "GridUtils.h"
#include <cstring>
#include "Confirmation.h"

static AEAudio sfxButton;
static AEAudioGroup pauseGroup;

// Local helper to draw a solid rectangle
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

// Button definitions
static struct {
    float x, y, w, h;
    const char* text;
    int action; // 0=Resume, 1=Restart, 2=LevelSelect, 3=Quit
} pauseButtons[] = {
    {    0.0f,  80.0f, 280.0f, 70.0f, "Resume", 0 },
    {    0.0f,   0.0f, 280.0f, 70.0f, "Restart", 1 },
    {    0.0f, -80.0f, 280.0f, 70.0f, "Level Select", 2 },
    {    0.0f,-160.0f, 280.0f, 70.0f, "Quit", 3 }
};
static const int pauseBtnCount = sizeof(pauseButtons) / sizeof(pauseButtons[0]);

// ----------------------------------------------------------------------------
void PausePage_Load()
{
    pMesh = CreateSquareMesh();
    pauseGroup = AEAudioCreateGroup();
    sfxButton = AEAudioLoadSound("Assets/audio/button.wav");
}

void PausePage_Initialize() {}

void PausePage_Update()
{
    // Keyboard shortcuts
    if (AEInputCheckReleased(AEVK_P))
    {
        next = previous; // resume
        return;
    }

    // Restart: show confirmation
    if (AEInputCheckReleased(AEVK_R))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, pauseGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_PAUSE, GS_RESTART, "Are you sure you want to restart?");
        next = CONFIRM;
        return;
    }

    if (AEInputCheckReleased(AEVK_B))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, pauseGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_PAUSE, LEVELPAGE, "Are you sure you want to go to Level Select?");
        next = CONFIRM;
        return;
    }

    if (AEInputCheckReleased(AEVK_ESCAPE))
    {
        if (AEAudioIsValidAudio(sfxButton))
            AEAudioPlay(sfxButton, pauseGroup, 1.0f, 1.0f, 0);
        Confirmation_Level(GS_PAUSE, GS_QUIT, "Are you sure you want to quit the game?");
        next = CONFIRM;
        return;
    }

    // Save/Load with keyboard (no confirmation)
    if (AEInputCheckReleased(AEVK_F5))
    {
        print_file();
    }
    if (AEInputCheckReleased(AEVK_F9))
    {
        readfile();
        next = previous; // resume after load
        return;
    }

    // --- Mouse click detection ---
    s32 mouseX, mouseY;
    TransformScreentoWorld(mouseX, mouseY);
    if (AEInputCheckReleased(AEVK_LBUTTON))
    {
        for (int i = 0; i < pauseBtnCount; ++i)
        {
            if (IsAreaClicked(pauseButtons[i].x, pauseButtons[i].y,
                pauseButtons[i].w, pauseButtons[i].h, mouseX, mouseY))
            {
                if (AEAudioIsValidAudio(sfxButton))
                    AEAudioPlay(sfxButton, pauseGroup, 1.0f, 1.0f, 0);

                switch (pauseButtons[i].action)
                {
                case 0: // Resume
                    next = previous;
                    break;
                case 1: // Restart
                    Confirmation_Level(GS_PAUSE, GS_RESTART, "Are you sure you want to restart?");
                    next = CONFIRM;
                    break;
                case 2: // Level Select
                    Confirmation_Level(GS_PAUSE, LEVELPAGE, "Are you sure you want to go to Level Select?");
                    next = CONFIRM;
                    break;
                case 3: // Quit
                    Confirmation_Level(GS_PAUSE, GS_QUIT, "Are you sure you want to quit the game?");
                    next = CONFIRM;
                    break;
                }
                return;
            }
        }
    }
}

void PausePage_Draw()
{
    // Dim the background
    AEGfxSetBackgroundColor(0, 0, 0);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);

    AEMtx33 scale, trans, mat;
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0, 0);
    AEMtx33Concat(&mat, &trans, &scale);

    AEGfxSetColorToMultiply(0, 0, 0, 0.5f);
    AEGfxSetTransform(mat.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Title "PAUSED"
    const char* title = "PAUSED";
    float w, h;
    const float titleScale = 2.0f;
    AEGfxGetPrintSize(fontId, title, titleScale, &w, &h);
    AEGfxPrint(fontId, title, -0.5f * w, 0.50f, titleScale, 1, 1, 1, 1);

    // Draw buttons
    for (int i = 0; i < pauseBtnCount; ++i)
    {
        DrawRect(pauseButtons[i].x, pauseButtons[i].y,
            pauseButtons[i].w, pauseButtons[i].h,
            0.4f, 0.4f, 0.4f); // dark gray

        // Center text inside button
        float btnCenterNDCX = pauseButtons[i].x / 800.0f;
        float btnCenterNDCY = pauseButtons[i].y / 450.0f;

        float textW, textH;
        const float textScale = 0.9f;
        AEGfxGetPrintSize(fontId, pauseButtons[i].text, textScale, &textW, &textH);
        float leftX = btnCenterNDCX - textW * 0.5f;
        float baselineY = btnCenterNDCY + textH * 0.5f;
        AEGfxPrint(fontId, pauseButtons[i].text, leftX, baselineY, textScale,
            1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Keyboard shortcuts footer
    const float helpScale = 0.65f;
    const char* help1 = "Keyboard: [P] Resume | [R] Restart | [B] Level Select | [ESC] Quit";
    float helpW, helpH;
    AEGfxGetPrintSize(fontId, help1, helpScale, &helpW, &helpH);
    AEGfxPrint(fontId, help1, -0.5f * helpW, -0.85f, helpScale, 0.8f, 0.8f, 0.8f, 1.0f);
}

void PausePage_Free() {}

void PausePage_Unload()
{
    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
    if (AEAudioIsValidAudio(sfxButton))
        AEAudioUnloadAudio(sfxButton);
    AEAudioUnloadAudioGroup(pauseGroup);
}