#include "pch.h"
#include "JumpScare.h"
#include "Main.h"

#include <iostream>
#include <fstream>
#include <cmath>

//                                                                --- Variables declaration start here ---

static bool jumpScareActive = false;
static float jumpScareTimer = 0.0f;
static float jumpScareScale = 0.0f;
static AEGfxTexture* jumpScareTexture = nullptr;

// Animation constants
static const float JUMP_SCARE_DURATION = 2.0f;
static const float POP_OUT_TIME = 0.5f;
static const float MAX_SCALE = 1.8f;
static const float MIN_SCALE = 0.8f;

//                                                                --- Variables declaration end here ---

void JumpScare_Init()
{
    jumpScareActive = false;
    jumpScareTimer = 0.0f;
    jumpScareScale = 0.0f;
}

void JumpScare_Load()
{
    // Load your jump scare image
    jumpScareTexture = AEGfxTextureLoad("Assets/JumpScare.png");

    if (!jumpScareTexture) {
        std::cout << "Failed to load jump scare texture!\n";
    }

    pMesh = CreateSquareMesh();
}

void JumpScare_Trigger()
{
    jumpScareActive = true;
    jumpScareTimer = 0.0f;
    jumpScareScale = 0.0f;

    std::cout << "JUMP SCARE TRIGGERED!\n"; // Debug
}

void JumpScare_Update()
{
    if (!jumpScareActive) return;

    // Update timer
    jumpScareTimer += AEFrameRateControllerGetFrameTime();

    if (jumpScareTimer >= JUMP_SCARE_DURATION)
    {
        jumpScareActive = false; // End jump scare
        return;
    }

    // Calculate animation based on time
    float t = jumpScareTimer / JUMP_SCARE_DURATION; // 0 to 1

    if (t < 0.2f)  // First 20% - POP OUT!
    {
        // Quick pop out from 0 to max scale
        float popProgress = t / 0.2f;  // 0 to 1
        // Ease-out effect: starts fast, slows down
        jumpScareScale = MAX_SCALE * (1.0f - powf(1.0f - popProgress, 2.0f));
    }
    else // Remaining 80% - PULSING
    {
        // Calculate pulse using sine wave
        // Speed up pulses over time for dramatic effect
        float pulseSpeed = 8.0f + (t - 0.2f) * 10.0f;
        float pulseValue = sinf((t - 0.2f) * pulseSpeed * 3.14159f * 2.0f);

        // Pulse between 1.0 and 1.4, gradually decreasing
        float baseScale = 1.4f - (t - 0.2f) * 0.8f; // Decrease from 1.4 to 0.8
        float pulseAmount = 0.3f * (1.0f - (t - 0.2f) * 0.8f); // Pulses get smaller

        jumpScareScale = baseScale + (pulseValue * pulseAmount);
    }
}

void JumpScare_Draw()
{
    if (!jumpScareActive || !jumpScareTexture) return;

    // Setup for texture rendering
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // Set the jump scare texture
    AEGfxTextureSet(jumpScareTexture, 0, 0);

    // Calculate scale and position
    float screenWidth = 1600.0f;
    float screenHeight = 900.0f;

    AEMtx33 scale, trans, transform;

    // Apply current animation scale
    float scaledWidth = screenWidth * jumpScareScale;
    float scaledHeight = screenHeight * jumpScareScale;

    AEMtx33Scale(&scale, scaledWidth, scaledHeight);
    AEMtx33Trans(&trans, 0.0f, 0.0f); // Center of screen
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);

    // Draw jumpscare
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

    // Optional: Add a screen flash during pop out
    if (jumpScareTimer < 0.1f)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 0.3f);  // White flash
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33Scale(&scale, screenWidth, screenHeight);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);

        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }
}

bool JumpScare_IsActive()
{
    return jumpScareActive;
}

void JumpScare_Unload()
{
    if (jumpScareTexture)
    {
        AEGfxTextureUnload(jumpScareTexture);
        jumpScareTexture = nullptr;
    }

    // Mesh cleanup
    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}