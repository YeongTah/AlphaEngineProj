#include "pch.h"
#include "Effects.h"
#include "Main.h"
#include <cmath>

//                                                                --- VARIABLES DECLARATION START HERE ---
Particle g_trailParticles[MAX_TRAIL_PARTICLES];
bool g_trailParticlesActive = false;
float g_lastPlayerX = 0.0f; // player's previous X position
float g_lastPlayerY = 0.0f; // player's previous Y position

static const float TRAIL_OFFSET_DISTANCE = 40.0f;  // Distance behind player
static const float TRAIL_PARTICLE_LIFETIME = 0.75f;
static const float TRAIL_PARTICLE_SIZE = 7.0f;
//                                                                --- VARIABLES DECLARATION END HERE ---

// ----------------------------------------------------------------------------
// SpawnTrailParticle
// Spawns a single trail particle behind the player based on movement direction.
// Calculates the offset position using the difference between the player's
// current and last position, then assigns random velocity, lifetime, size,
// and dust/brown color. Finds the first inactive slot in g_trailParticles[].
// Called by TrailParticle_OnPlayerMoved.
// ----------------------------------------------------------------------------
static void SpawnTrailParticle(float playerX, float playerY)
{   

    for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
        if (!g_trailParticles[i].active) {
            // Calculate direction of movement (using last position)
            float dirX = playerX - g_lastPlayerX;
            float dirY = playerY - g_lastPlayerY;
            float length = sqrtf(dirX * dirX + dirY * dirY);

            if (length > 0.01f) {
                // Normalise direction
                dirX /= length;
                dirY /= length;

                // Place particle behind player (opposite direction of movement)
                float offsetX = -dirX * TRAIL_OFFSET_DISTANCE;
                float offsetY = -dirY * TRAIL_OFFSET_DISTANCE;

                g_trailParticles[i].x = playerX + offsetX;
                g_trailParticles[i].y = playerY + offsetY;
            }

            // Velocity of the particles (scattering effect)
            g_trailParticles[i].velX = (AERandFloat() - 0.5f) * 50.0f;
            g_trailParticles[i].velY = (AERandFloat() - 0.5f) * 50.0f;

            // Shorter lifetime for trail particles
            g_trailParticles[i].lifetime = TRAIL_PARTICLE_LIFETIME * (0.5f + AERandFloat() * 0.5f);

            // Smaller size for trail
            g_trailParticles[i].size = TRAIL_PARTICLE_SIZE * (0.7f + AERandFloat() * 0.6f);

            // Dust/cloud colors (light brown/gray)
            g_trailParticles[i].r = 0.6f + AERandFloat() * 0.3f;
            g_trailParticles[i].g = 0.5f + AERandFloat() * 0.3f;
            g_trailParticles[i].b = 0.4f + AERandFloat() * 0.3f;

            g_trailParticles[i].active = true;
            break;
        }
    }
}

// ----------------------------------------------------------------------------
// TrailParticle_Init
// Initialises the trail particle system by resetting all particle slots to
// inactive and zeroing their properties. Sets g_trailParticlesActive to true
// and clears the last known player position. Called in Level1_Initialize and
// ResetLevel1 to ensure a clean state on every level entry.
// ----------------------------------------------------------------------------
void TrailParticle_Init(void)
{
    // Clear all trail particles
    for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
        g_trailParticles[i].active = false;

        // Also reset other properties for safety
        g_trailParticles[i].x = 0.0f;
        g_trailParticles[i].y = 0.0f;
        g_trailParticles[i].velX = 0.0f;
        g_trailParticles[i].velY = 0.0f;
        g_trailParticles[i].lifetime = 0.0f;
        g_trailParticles[i].size = 0.0f;
    }

    g_trailParticlesActive = true;  // Always active for player trail
    g_lastPlayerX = 0.0f;
    g_lastPlayerY = 0.0f;
}

// ----------------------------------------------------------------------------
// TrailParticle_Update
// Updates all currently active particles each frame. Moves each particle by
// its velocity scaled by dt, decrements its lifetime, and deactivates it when
// lifetime reaches zero. Also records the player's current position into
// g_lastPlayerX/Y for use by SpawnTrailParticle on the next move.
// Called every frame in Level1_Update regardless of player movement.
// ----------------------------------------------------------------------------
void TrailParticle_Update(float dt, float playerX, float playerY)
{
    if (!g_trailParticlesActive) return;

    // Update existing particles
    for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
        if (g_trailParticles[i].active) {
            // Trail particles drift slowly
            g_trailParticles[i].x += g_trailParticles[i].velX * dt;
            g_trailParticles[i].y += g_trailParticles[i].velY * dt;

            // Fade out
            g_trailParticles[i].lifetime -= dt;

            if (g_trailParticles[i].lifetime <= 0.0f) {
                g_trailParticles[i].active = false;
            }
        }
    }

    // Update last position for next frame
    g_lastPlayerX = playerX;
    g_lastPlayerY = playerY;
}

// ----------------------------------------------------------------------------
// TrailParticle_OnPlayerMoved
// Spawns a small burst of 5 to 6 trail particles at the player's new position.
// Should be called once per valid player move, after player.x/y has been
// updated, so SpawnTrailParticle can correctly calculate movement direction
// using g_lastPlayerX/Y from the previous frame.
// ---
// Separated from TrailParticle_Update because the game is turn-based, hence
// update runs every frame to fade existing particles, but spawning should only
// trigger on discrete player movement, not on a continuous timer.
// ----------------------------------------------------------------------------
void TrailParticle_OnPlayerMoved(float playerX, float playerY) {
    // Spawn particles
    int spawnCount = 5 + (int)(AERandFloat() * 1.5f);
    for (int s = 0; s < spawnCount; s++) {
        SpawnTrailParticle(playerX, playerY);
    }
}

// ----------------------------------------------------------------------------
// TrailParticle_Draw
// Renders all active trail particles using AE_GFX_RM_COLOR blend mode.
// Alpha is derived from remaining lifetime and capped between 0.1 and 0.7
// for a subtle fade-out effect. Builds each particle's transform matrix from
// its position and size, then draws using the shared pMesh.
// Called in Level1_Draw after all game entities are rendered.
// ----------------------------------------------------------------------------
void TrailParticle_Draw(void)
{    
    if (!g_trailParticlesActive) return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);

    // Reset colour mode before adding colours
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEMtx33 scale, trans, transform;

    for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
        if (g_trailParticles[i].active) {
            float alpha = g_trailParticles[i].lifetime / TRAIL_PARTICLE_LIFETIME;
            if (alpha > 0.7f) alpha = 0.7f;  // Cap alpha for subtle effect
            if (alpha < 0.1f) alpha = 0.1f;

            // Set actual colours to particles
            AEGfxSetColorToMultiply(g_trailParticles[i].r, g_trailParticles[i].g,
                g_trailParticles[i].b, alpha);

            AEMtx33Scale(&scale, g_trailParticles[i].size, g_trailParticles[i].size);
            AEMtx33Trans(&trans, g_trailParticles[i].x, g_trailParticles[i].y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);

            if (pMesh) {
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

}

