#include "pch.h"
#include "JumpScare.h"
#include "Main.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <ctime> 

//                                                                --- VARIABLES DECLARATION START HERE ---

static bool jumpScareActive = false;
static float jumpScareTimer = 0.0f;
static float jumpScareScale = 0.0f;
static AEGfxTexture* jumpScareTextures[2] = { nullptr, nullptr }; // 2 images
static int currentTextureIndex = 0; // Which image to show

// === Animation constants ===
static const float JUMP_SCARE_DURATION = 2.0f;
static const float POP_OUT_TIME = 0.5f;
static const float MAX_SCALE = 1.4f;
static const float MIN_SCALE = 0.6f;

//                                                                --- VARIABLES DECLARATION END HERE ---

// ----------------------------------------------------------------------------
// JumpScare_Init
// Initialise jump scare system
// ----------------------------------------------------------------------------
void JumpScare_Init()
{
    jumpScareActive = false;
    jumpScareTimer = 0.0f;
    jumpScareScale = 0.0f;
}

// ----------------------------------------------------------------------------
// JumpScare_Load
// Load jump scare texture (call in Level_Load)
// ----------------------------------------------------------------------------
void JumpScare_Load()
{
    // Load both jump scare images
    jumpScareTextures[0] = AEGfxTextureLoad("Assets/JumpScare1.jpg");
    jumpScareTextures[1] = AEGfxTextureLoad("Assets/JumpScare2.jpg");

    // Check if loaded successfully
    if (!jumpScareTextures[0]) return;
    if (!jumpScareTextures[1]) return;

    // Seed random once
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }

    pMesh = CreateSquareMesh();
}

// ----------------------------------------------------------------------------
// JumpScare_Trigger
// Trigger the jump scare
// ----------------------------------------------------------------------------
void JumpScare_Trigger()
{
    jumpScareActive = true;
    jumpScareTimer = 0.0f;
    jumpScareScale = 0.0f;

    // Randomly choose which image to show (0 or 1)
    currentTextureIndex = rand() % 2; // 0 or 1

}

// ----------------------------------------------------------------------------
// JumpScare_Update
// Update jump scare animation (call in Level_Update)
// ----------------------------------------------------------------------------
void JumpScare_Update()
{
    if (!jumpScareActive) return;

    // Add delta time to timers
    float dt = (float)AEFrameRateControllerGetFrameTime();
    jumpScareTimer += dt;

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

// ----------------------------------------------------------------------------
// JumpScare_Draw
// Draw jump scare (call in Level_Draw)
// ----------------------------------------------------------------------------
void JumpScare_Draw()
{
    if (!jumpScareActive) return;

    // Check if current texture exists
    if (!jumpScareTextures[currentTextureIndex]) return;

    // Setup for texture rendering
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // Set the randomly chosen texture
    AEGfxTextureSet(jumpScareTextures[currentTextureIndex], 0, 0);

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
}

// ----------------------------------------------------------------------------
// JumpScare_Draw
// Returns when jumpscare is active
// ----------------------------------------------------------------------------
bool JumpScare_IsActive()
{
    return jumpScareActive;
}

// ----------------------------------------------------------------------------
// JumpScare_Draw
// Unload jump scare texture (call in Level_Unload)
// ----------------------------------------------------------------------------
void JumpScare_Unload()
{
    // Unload both textures
    for (int i = 0; i < 2; i++) {
        if (jumpScareTextures[i]) {
            AEGfxTextureUnload(jumpScareTextures[i]);
            jumpScareTextures[i] = nullptr;
        }
    }

    // Mesh cleanup
    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }
}