#include "pch.h"

#include "MouseCoor.h"
#include <iostream>
#include <fstream>

//----------------------------------------------------------------------------
// Convert Screen coordinates to World coordinates for mouse position
// ---------------------------------------------------------------------------
void TransformScreentoWorld(s32& mouseX, s32& mouseY) {

    // Get mouse screen coordinate
    s32 screenX, screenY;
    AEInputGetCursorPosition(&screenX, &screenY);
    std::cout << "screen X pos:" << screenX << ", screen Y pos:" << screenY << "\n"; // Debug purposes 

    // Translate screen coordinates to world coordinates
    s32 worldX = screenX - (AEGfxGetWindowWidth() / 2);  // Subtract half width to move origin to center
    s32 worldY = (AEGfxGetWindowHeight() / 2) - screenY; // Use half height and subtract Y to move origin to center and invert y axis

    // Update the output parameters
    mouseX = worldX;
    mouseY = worldY;

    std::cout << "world X pos:" << mouseX << "world Y pos:" << mouseY << "\n"; // Debug purposes
}