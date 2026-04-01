#pragma once

#include "AEEngine.h"

// Constants
#define MAX_TRAIL_PARTICLES 100

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
extern float g_trailSpawnTimer;
extern float g_lastPlayerX;
extern float g_lastPlayerY;

// Player Trail Particle Functions
void TrailParticle_Init(void);
void Particle_Load(void);
void TrailParticle_Spawn(float playerX, float playerY);
void TrailParticle_Update(float dt, float playerX, float playerY);
void TrailParticle_Draw(void);
void TrailParticle_Clear(void);
void TrailParticle_Stop(void);
void TrailParticle_Unload(void);