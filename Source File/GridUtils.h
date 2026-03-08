#pragma once
#include "AEEngine.h"
// Constants for the entire project
// Match these to ensure player and editor are synced
const int GRID_ROWS = 18;
const int GRID_COLS = 32;
const float GRID_TILE_SIZE = 50.0f; // Use the value you set in Level1

// Use 'extern' so the actual array in leveleditor.cpp is shared
extern int level[GRID_ROWS][GRID_COLS];

// Function declarations
void WorldToGrid(float worldX, float worldY, int& outRow, int& outCol);
void GridToWorldCenter(int row, int col, float& outX, float& outY);
bool IsTileWalkable(float worldX, float worldY);
void DrawGridLines(AEGfxVertexList* pMesh);
void DrawTileOutline(int row, int col, float r, float g, float b, AEGfxVertexList* pMesh);
//bool canMove(float nextX, float nextY);
void LoadDefaultLevel();