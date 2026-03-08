#include "GridUtils.h"
#include <cmath>
#include "pch.h"


void WorldToGrid(float worldX, float worldY, int& outRow, int& outCol) {
    float left = -(GRID_COLS * GRID_TILE_SIZE) * 0.5f; //
    float top = +(GRID_ROWS * GRID_TILE_SIZE) * 0.5f;  //

    outCol = (int)std::floor((worldX - left) / GRID_TILE_SIZE); //
    outRow = (int)std::floor((top - worldY) / GRID_TILE_SIZE); //
}

bool IsTileWalkable(float worldX, float worldY) {
    int row, col;
    WorldToGrid(worldX, worldY, row, col);

    // Boundary check: If out of bounds, it's not walkable
    if (row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS) {
        return false;
    }

    // Return true if the tile is NOT a wall (value 1)
    return level[row][col] != 1; //
}

//bool canMove(float nextX, float nextY) {
//    // This checks if the center of the target tile is walkable
//    return IsTileWalkable(nextX, nextY);
//}

void GridToWorldCenter(int row, int col, float& outX, float& outY) {
    float left = -(GRID_COLS * GRID_TILE_SIZE) * 0.5f;
    float top = +(GRID_ROWS * GRID_TILE_SIZE) * 0.5f;

    // Calculate center of the tile
    outX = left + (col * GRID_TILE_SIZE) + (GRID_TILE_SIZE * 0.5f);
    outY = top - (row * GRID_TILE_SIZE) - (GRID_TILE_SIZE * 0.5f);
}
void DrawLine(float startX, float startY, float endX, float endY, float thickness, float r, float g, float b, AEGfxVertexList* mesh) {
    float centerX = (startX + endX) * 0.5f;
    float centerY = (startY + endY) * 0.5f;
    float dx = endX - startX;
    float dy = endY - startY;
    float length = std::sqrt(dx * dx + dy * dy);

    // 1. Calculate the angle of the line
    float angle = std::atan2(dy, dx);

    // 2. Prepare matrices
    AEMtx33 scale, trans, rot, transform, temp;

    AEMtx33Scale(&scale, length, thickness);
    AEMtx33Rot(&rot, angle);
    AEMtx33Trans(&trans, centerX, centerY);

    // 3. Concatenate: Trans * Rot * Scale
    // Note: Concatenation order matters. 
    // We scale first, then rotate, then translate to position.
    AEMtx33Concat(&temp, &rot, &scale);
    AEMtx33Concat(&transform, &trans, &temp);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(r, g, b, 1.0f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}
void DrawGridLines(AEGfxVertexList* pMesh) {
    float left = -(GRID_COLS * GRID_TILE_SIZE) * 0.5f;
    float top = +(GRID_ROWS * GRID_TILE_SIZE) * 0.5f;
    float totalW = GRID_COLS * GRID_TILE_SIZE;
    float totalH = GRID_ROWS * GRID_TILE_SIZE;

    // 1. Draw Vertical Lines
    for (int i = 0; i <= GRID_COLS; ++i) {
        float x = left + (i * GRID_TILE_SIZE);
        DrawLine(x, top, x, top - totalH, 1.0f, 0.5f, 0.5f, 0.5f, pMesh);
    }

    // 2. Draw Horizontal Lines (This was missing)
    for (int i = 0; i <= GRID_ROWS; ++i) {
        float y = top - (i * GRID_TILE_SIZE);
        DrawLine(left, y, left + totalW, y, 1.0f, 0.5f, 0.5f, 0.5f, pMesh);
    }
}

void DrawTileOutline(int row, int col, float r, float g, float b, AEGfxVertexList* pMesh) {
    float x, y;
    GridToWorldCenter(row, col, x, y); // Get center
    float half = GRID_TILE_SIZE * 0.5f;

    // Draw 4 lines for the square border
    DrawLine(x - half, y - half, x + half, y - half, 2.0f, r, g, b, pMesh); // Bottom
    DrawLine(x - half, y + half, x + half, y + half, 2.0f, r, g, b, pMesh); // Top
    DrawLine(x - half, y - half, x - half, y + half, 2.0f, r, g, b, pMesh); // Left
    DrawLine(x + half, y - half, x + half, y + half, 2.0f, r, g, b, pMesh); // Right
}


void LoadDefaultLevel()
{
    // 1. Clear everything (set all to 0)
    for (int r = 0; r < GRID_ROWS; ++r) {
        for (int c = 0; c < GRID_COLS; ++c) {
            level[r][c] = 0;
        }
    }

    // 2. Remove the two loops that created the outer border here.

    // 3. Manually place only the walls you want
    level[5][5] = 1;
    level[5][6] = 1;

    // Add any other walls you specifically want here:
    // level[10][10] = 1; 
}