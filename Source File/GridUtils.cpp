#include "pch.h"

#include "GridUtils.h"
#include <cmath>

// ----------------------------------------------------------------------------
// WorldToGrid
// Converts a world-space position (worldX, worldY) into grid row/column indices.
//
// The grid is centered at world origin (0, 0):
//   - X increases rightward; leftmost column edge is at -(GRID_COLS * GRID_TILE_SIZE * 0.5)
//   - Y increases upward;    topmost row edge    is at +(GRID_ROWS * GRID_TILE_SIZE * 0.5)
//
// Formula:
//   col = floor((worldX - left) / GRID_TILE_SIZE)
//   row = floor((top - worldY)  / GRID_TILE_SIZE)
//
// Results are NOT bounds-checked here -- callers must clamp or validate.
// Used by: IsTileWalkable, isBlockedAt, coin collection, mummy AI, level editor painting.
// ----------------------------------------------------------------------------
void WorldToGrid(float worldX, float worldY, int& outRow, int& outCol) {
    float left = -(GRID_COLS * GRID_TILE_SIZE) * 0.5f; // world X of the leftmost column edge
    float top = +(GRID_ROWS * GRID_TILE_SIZE) * 0.5f; // world Y of the topmost row edge

    outCol = (int)std::floor((worldX - left) / GRID_TILE_SIZE); // horizontal cell index
    outRow = (int)std::floor((top - worldY) / GRID_TILE_SIZE); // vertical cell index (row 0 = top)
}

// ----------------------------------------------------------------------------
// IsTileWalkable
// Returns true if the grid cell at world position (worldX, worldY) is walkable
// (i.e. its tile value is NOT 1 / NON_WALKABLE).
//
// Steps:
//   1. Convert world pos to grid indices via WorldToGrid.
//   2. If the indices are outside grid bounds, return false (treat as wall).
//   3. Return true only if level[row][col] != 1.
//
// Used by player movement in all Level files: the player's candidate position
// is validated here before being applied.
// ----------------------------------------------------------------------------
bool IsTileWalkable(float worldX, float worldY) {
    int row, col;
    WorldToGrid(worldX, worldY, row, col);

    // Out-of-bounds positions are treated as non-walkable (invisible boundary wall)
    if (row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS) {
        return false;
    }

    // Tile value 1 = NON_WALKABLE wall; any other value is considered passable
    return level[row][col] != 1;
}

// ----------------------------------------------------------------------------
// GridToWorldCenter
// Converts a grid (row, col) index pair to the world-space center of that tile.
//
// Formula:
//   outX = left + col * GRID_TILE_SIZE + GRID_TILE_SIZE * 0.5   (center of column)
//   outY = top  - row * GRID_TILE_SIZE - GRID_TILE_SIZE * 0.5   (center of row, Y-down)
//
// Used when placing entities at tile centers (spawn, mummy AI positioning),
// and when rendering tile sprites (floor, walls) in all Level Draw functions.
// ----------------------------------------------------------------------------
void GridToWorldCenter(int row, int col, float& outX, float& outY) {
    float left = -(GRID_COLS * GRID_TILE_SIZE) * 0.5f;
    float top = +(GRID_ROWS * GRID_TILE_SIZE) * 0.5f;

    outX = left + (col * GRID_TILE_SIZE) + (GRID_TILE_SIZE * 0.5f); // horizontal center
    outY = top - (row * GRID_TILE_SIZE) - (GRID_TILE_SIZE * 0.5f); // vertical center (Y-up)
}

// ----------------------------------------------------------------------------
// DrawLine
// Draws a colored line from (startX, startY) to (endX, endY) with a given
// pixel thickness, using the provided square mesh scaled and rotated.
//
// Implementation:
//   1. Compute the midpoint (centerX, centerY) of the line.
//   2. Compute the line's length and angle via atan2.
//   3. Build a TRS matrix: Scale(length, thickness) * Rot(angle) * Trans(center).
//   4. Draw the mesh as a thin rectangle aligned to the line direction.
//
// Used by DrawGridLines and DrawTileOutline for visual debugging/editor overlays.
// ----------------------------------------------------------------------------
void DrawLine(float startX, float startY, float endX, float endY,
    float thickness, float r, float g, float b, AEGfxVertexList* mesh)
{
    float centerX = (startX + endX) * 0.5f; // midpoint X
    float centerY = (startY + endY) * 0.5f; // midpoint Y
    float dx = endX - startX;
    float dy = endY - startY;
    float length = std::sqrt(dx * dx + dy * dy); // line length = mesh scale X

    float angle = std::atan2(dy, dx); // rotation angle to align mesh to the line

    AEMtx33 scale, trans, rot, transform, temp;

    AEMtx33Scale(&scale, length, thickness); // stretch mesh to line length
    AEMtx33Rot(&rot, angle);                 // rotate to match line direction
    AEMtx33Trans(&trans, centerX, centerY);  // position at line midpoint

    // Concatenate: Transform = Trans * Rot * Scale  (right-to-left application)
    AEMtx33Concat(&temp, &rot, &scale);
    AEMtx33Concat(&transform, &trans, &temp);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(r, g, b, 1.0f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// ----------------------------------------------------------------------------
// DrawGridLines
// Draws a full grid overlay of thin grey lines for visual debugging.
// Draws GRID_COLS+1 vertical lines and GRID_ROWS+1 horizontal lines spanning
// the entire grid area.  Used in the level editor to show cell boundaries.
// ----------------------------------------------------------------------------
void DrawGridLines(AEGfxVertexList* pMesh) {
    float left = -(GRID_COLS * GRID_TILE_SIZE) * 0.5f;
    float top = +(GRID_ROWS * GRID_TILE_SIZE) * 0.5f;
    float totalW = GRID_COLS * GRID_TILE_SIZE;
    float totalH = GRID_ROWS * GRID_TILE_SIZE;

    // Vertical lines (one per column boundary)
    for (int i = 0; i <= GRID_COLS; ++i) {
        float x = left + (i * GRID_TILE_SIZE);
        DrawLine(x, top, x, top - totalH, 1.0f, 0.5f, 0.5f, 0.5f, pMesh); // grey, 1px thick
    }

    // Horizontal lines (one per row boundary)
    for (int i = 0; i <= GRID_ROWS; ++i) {
        float y = top - (i * GRID_TILE_SIZE);
        DrawLine(left, y, left + totalW, y, 1.0f, 0.5f, 0.5f, 0.5f, pMesh); // grey, 1px thick
    }
}

// ----------------------------------------------------------------------------
// DrawTileOutline
// Draws a colored square border (4 lines) around a single grid cell (row, col).
// Useful for highlighting selected or hovered tiles in the level editor.
// The border is drawn 2px thick using the specified (r, g, b) color.
// ----------------------------------------------------------------------------
void DrawTileOutline(int row, int col, float r, float g, float b, AEGfxVertexList* pMesh) {
    float x, y;
    GridToWorldCenter(row, col, x, y); // get world-space center of the tile
    float half = GRID_TILE_SIZE * 0.5f;

    // Draw each of the 4 edges of the tile square
    DrawLine(x - half, y - half, x + half, y - half, 2.0f, r, g, b, pMesh); // Bottom edge
    DrawLine(x - half, y + half, x + half, y + half, 2.0f, r, g, b, pMesh); // Top edge
    DrawLine(x - half, y - half, x - half, y + half, 2.0f, r, g, b, pMesh); // Left edge
    DrawLine(x + half, y - half, x + half, y + half, 2.0f, r, g, b, pMesh); // Right edge
}


// ----------------------------------------------------------------------------
// LoadDefaultLevel
// Initializes level[][] to a minimal default layout used as a fallback when
// no .txt file is found on disk (e.g. first run or missing asset).
//
// Default layout:
//   - All cells set to 0 (empty / walkable)
//   - Two wall tiles placed at [5][5] and [5][6] as a basic obstacle example
//
// Called by readfile() in leveleditor.cpp when the file cannot be opened.
// ----------------------------------------------------------------------------
void LoadDefaultLevel()
{
    // Step 1: Clear all tiles to walkable (0)
    for (int r = 0; r < GRID_ROWS; ++r)
        for (int c = 0; c < GRID_COLS; ++c)
            level[r][c] = 0;

    // Step 2: Place a minimal example wall so the grid is not completely empty
    level[5][5] = 1; // NON_WALKABLE
    level[5][6] = 1; // NON_WALKABLE
}