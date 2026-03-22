#pragma once

#include "AEEngine.h"

// Initialise jump scare system
void JumpScare_Init();

// Load jump scare texture (call in Level_Load)
void JumpScare_Load();

// Trigger the jump scare
void JumpScare_Trigger();

// Update jump scare animation (call in Level_Update)
void JumpScare_Update();

// Draw jump scare (call in Level_Draw)
void JumpScare_Draw();

// Check if jump scare is active
bool JumpScare_IsActive();

// Unload jump scare texture (call in Level_Unload)
void JumpScare_Unload();