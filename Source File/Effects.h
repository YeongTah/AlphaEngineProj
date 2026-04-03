#pragma once

#include "AEEngine.h"

// Constants
#define MAX_TRAIL_PARTICLES 20

// Particle structure
struct Particle {
    float x, y;       // Position
    float velX, velY; // Velocity
    float lifetime;   // Remaining lifetime in seconds
    float size;       // Size of particle
    bool active;      // Is particle active
    float r, g, b;    // Colour
};

// Player Trail Particle System
extern Particle g_trailParticles[MAX_TRAIL_PARTICLES];
extern bool g_trailParticlesActive;
extern float g_lastPlayerX;
extern float g_lastPlayerY;

// Player Trail Particle Functions
void TrailParticle_Init(void);
void TrailParticle_Update(float dt, float playerX, float playerY);
void TrailParticle_OnPlayerMoved(float playerX, float playerY);
void TrailParticle_Draw(void);