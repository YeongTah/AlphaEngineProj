#include "pch.h"
#include "Effects.h"
#include "Main.h"
#include <iostream>
#include <cmath>

//                                                                --- VARIABLES DECLARATION START HERE ---
Particle g_trailParticles[MAX_TRAIL_PARTICLES];
bool g_trailParticlesActive = false;
float g_trailSpawnTimer = 0.0f;
float g_lastPlayerX = 0.0f;
float g_lastPlayerY = 0.0f;

static const float TRAIL_SPAWN_DELAY = 0.15f;  // Slower spawn rate
static const float TRAIL_OFFSET_DISTANCE = 50.0f;  // Distance behind player
static const float TRAIL_PARTICLE_LIFETIME = 2.0f;
static const float TRAIL_PARTICLE_SIZE = 5.0f;
//                                                                --- VARIABLES DECLARATION END HERE ---

// Spawn a single trail particle behind the player
static void SpawnTrailParticle(float playerX, float playerY)
{   
    static int spawnCounter = 0;
    spawnCounter++;

    for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
        if (!g_trailParticles[i].active) {
            // Calculate direction of movement (using last position)
            float dirX = playerX - g_lastPlayerX;
            float dirY = playerY - g_lastPlayerY;
            float length = sqrtf(dirX * dirX + dirY * dirY);

            if (length > 0.01f) {
                // Normalize direction
                dirX /= length;
                dirY /= length;

                // Place particle behind player (opposite direction of movement)
                float offsetX = -dirX * TRAIL_OFFSET_DISTANCE;
                float offsetY = -dirY * TRAIL_OFFSET_DISTANCE;

                g_trailParticles[i].x = playerX + offsetX;
                g_trailParticles[i].y = playerY + offsetY;
            }
            else {
                // If not moving, place slightly behind based on last position
                g_trailParticles[i].x = playerX - 20.0f;
                g_trailParticles[i].y = playerY;
            }

            // Minimal velocity (barely moves)
            g_trailParticles[i].velX = (AERandFloat() - 0.5f) * 20.0f;
            g_trailParticles[i].velY = (AERandFloat() - 0.5f) * 20.0f;

            // Shorter lifetime for trail particles
            g_trailParticles[i].lifetime = TRAIL_PARTICLE_LIFETIME * (0.5f + AERandFloat() * 0.5f);

            // Larger size for visibility
            g_trailParticles[i].size = TRAIL_PARTICLE_SIZE * (0.7f + AERandFloat() * 0.6f);

            // BRIGHT RED for testing visibility
            g_trailParticles[i].r = 1.0f;
            g_trailParticles[i].g = 0.0f;
            g_trailParticles[i].b = 0.0f;

            g_trailParticles[i].active = true;

            // DEBUG: Print particle info
            std::cout << "Spawned particle #" << spawnCounter
                << " at (" << g_trailParticles[i].x << ", " << g_trailParticles[i].y
                << ") | lifetime: " << g_trailParticles[i].lifetime
                << " | size: " << g_trailParticles[i].size << "\n";
            return;
        }
    }
    std::cout << "No free particle slot! Active count: ";

    // Count active particles for debug
    int activeCount = 0;
    for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
        if (g_trailParticles[i].active) activeCount++;
    }
    std::cout << activeCount << "/" << MAX_TRAIL_PARTICLES << "\n";

    //for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
    //    if (!g_trailParticles[i].active) {
    //        // Calculate direction of movement (using last position)
    //        float dirX = playerX - g_lastPlayerX;
    //        float dirY = playerY - g_lastPlayerY;
    //        float length = sqrtf(dirX * dirX + dirY * dirY);

    //        if (length > 0.01f) {
    //            // Normalize direction
    //            dirX /= length;
    //            dirY /= length;

    //            // Place particle behind player (opposite direction of movement)
    //            float offsetX = -dirX * TRAIL_OFFSET_DISTANCE;
    //            float offsetY = -dirY * TRAIL_OFFSET_DISTANCE;

    //            g_trailParticles[i].x = playerX + offsetX;
    //            g_trailParticles[i].y = playerY + offsetY;
    //        }
    //        else {
    //            // If not moving, place slightly behind based on last position
    //            g_trailParticles[i].x = playerX - 20.0f;
    //            g_trailParticles[i].y = playerY;
    //        }

    //        // Minimal velocity (barely moves)
    //        g_trailParticles[i].velX = (AERandFloat() - 0.5f) * 20.0f;
    //        g_trailParticles[i].velY = (AERandFloat() - 0.5f) * 20.0f;

    //        // Shorter lifetime for trail particles
    //        g_trailParticles[i].lifetime = TRAIL_PARTICLE_LIFETIME * (0.5f + AERandFloat() * 0.5f);

    //        // Smaller size for trail
    //        g_trailParticles[i].size = TRAIL_PARTICLE_SIZE * (0.7f + AERandFloat() * 0.6f);

    //        // Dust/cloud colors (light brown/gray)
    //        g_trailParticles[i].r = 0.6f + AERandFloat() * 0.3f;
    //        g_trailParticles[i].g = 0.5f + AERandFloat() * 0.3f;
    //        g_trailParticles[i].b = 0.4f + AERandFloat() * 0.3f;

    //        g_trailParticles[i].r = 1.0f;
    //        g_trailParticles[i].g = 0.0f;
    //        g_trailParticles[i].b = 1.0f;

    //        g_trailParticles[i].active = true;
    //        break;
    //    }
    //}
}

void TrailParticle_Init(void)
{
    std::cout << "Initialise trail\n";
    
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
    g_trailSpawnTimer = 0.0f;
    g_lastPlayerX = 0.0f;
    g_lastPlayerY = 0.0f;
}

void Particle_Load() {
    pMesh = CreateSquareMesh();
}

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

    // Spawn particles
    g_trailSpawnTimer += dt;
    if (g_trailSpawnTimer >= TRAIL_SPAWN_DELAY) {
        g_trailSpawnTimer = 0.0f;
        int spawnCount = 5 + (int)(AERandFloat() * 1.5f);
        for (int s = 0; s < spawnCount; s++) {
            SpawnTrailParticle(playerX, playerY);
        }
    }

    // Update last position for next frame
    g_lastPlayerX = playerX;
    g_lastPlayerY = playerY;
}

void TrailParticle_Draw(void)
{    
    if (!g_trailParticlesActive) return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEMtx33 scale, trans, transform;

    for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
        if (g_trailParticles[i].active) {
            float alpha = g_trailParticles[i].lifetime / TRAIL_PARTICLE_LIFETIME;
            if (alpha > 0.7f) alpha = 0.7f;  // Cap alpha for subtle effect
            if (alpha < 0.1f) alpha = 0.1f;

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

    //AEMtx33Identity(&transform);
    //AEGfxSetTransform(transform.m);
}

void TrailParticle_Clear(void)
{
    std::cout << "Trail particles cleared\n";

    // Deactivate all trail particles
    for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
        g_trailParticles[i].active = false;
    }
    g_trailParticlesActive = true; // keep active for player movement
    g_trailSpawnTimer = 0.0f;
    g_lastPlayerX = 0.0f;
    g_lastPlayerY = 0.0f;
}

void TrailParticle_Stop(void)
{
    g_trailParticlesActive = false;

    // Clear all trail particles
    for (int i = 0; i < MAX_TRAIL_PARTICLES; i++) {
        g_trailParticles[i].active = false;
    }
}

void TrailParticle_Unload(void)
{
    // Mesh cleanup
    if (pMesh)
    {
        AEGfxMeshFree(pMesh);
        pMesh = nullptr;
    }

}