#include "AEEngine.h"
#include "leveleditor.hpp"
#include <fstream>
#include <iostream>
#include <math.h>

//tile size is 50.0f  YT   5/3

// GRID 
int level[18][32];
int ROWS = 18;
int COLS = 32;


float TILE_SIZE = 50.0f;

typedef enum Objects
{
    NON_WALKABLE = 1,
    PLAYER_SPAWN = 2,
    ENEMY_SPAWN = 3,
    COIN = 4  // coin value in level1 txt -- YT
} Objects;

//namespace { // namespace commented out to make this fucntion public

    void WorldToGrid(float worldX, float worldY, int& outRow, int& outCol)
    {
        float left = -(COLS * TILE_SIZE) * 0.5f;
        float top = + (ROWS * TILE_SIZE) * 0.5f;

        outCol = (int)floorf((worldX - left) / TILE_SIZE);
        outRow = (int)floorf((top - worldY) / TILE_SIZE);
    }

    void GridToWorldCenter(int row, int col, float& outX, float& outY)
    {
        float left = -(COLS * TILE_SIZE) * 0.5f;
        float top = +(ROWS * TILE_SIZE) * 0.5f;

        outX = left + col * TILE_SIZE + (TILE_SIZE * 0.5f);
        outY = top - row * TILE_SIZE - (TILE_SIZE * 0.5f);
    }

    // COLLISION
    bool isBlockedAt(float worldX, float worldY)
    {
        int row, col;
        WorldToGrid(worldX, worldY, row, col);

        // Outside map = blocked
        if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
            return true;

        return level[row][col] == NON_WALKABLE;
    }
//}




// FILE 
int print_file()
{
    std::ofstream os("level1.txt");
    if (!os.is_open()) {
        std::cout << "cannot find\n";
        return 0;
    }

    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            os << level[i][j] << ",";
        }
        os << "\n";
    }

    os.close();
    return 0;
}

void readfile()
{
    std::ifstream os("level1.txt");
    if (!os.is_open()) {
        std::cout << "cannot find\n";
        return;
    }

    int tile;
    char comma;

    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            os >> tile >> comma;       // number + ','
            level[row][col] = tile;
        }
    }

    os.close();
}

// DRAW GRID
void generateLevel(void)
{
    float half = TILE_SIZE * 0.5f;

    // Mouse click 
    if (AEInputCheckTriggered(AEVK_LBUTTON)) {
        int mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);

        // Convert screen pixels
        float worldX = (float)mouseX - 800.0f;
        float worldY = 450.0f - (float)mouseY;

        int row, col;
        WorldToGrid(worldX, worldY, row, col);

        if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
            level[row][col] = NON_WALKABLE;
            print_file();
            std::cout << "CLICKED row=" << row << " col=" << col
                << " value=" << level[row][col] << "\n";
        }
    }

    // Tiles
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {

            float x, y;
            GridToWorldCenter(row, col, x, y);

            // NON_WALKABLE = red fill
            if (level[row][col] == NON_WALKABLE)
            {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);

                AEGfxTextureSet(gDesertBlockTex, 0, 0);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

                AEMtx33 scale, translate, transform;
                AEMtx33Scale(&scale, TILE_SIZE, TILE_SIZE);
                AEMtx33Trans(&translate, x, y);
                AEMtx33Concat(&transform, &translate, &scale);

                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }
            else if (level[row][col] == COIN) {
                // Draw Coin as a smaller Yellow Square
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetColorToMultiply(1.0f, 0.8f, 0.0f, 1.0f); // Gold/Yellow
                AEMtx33 scale, trans, transform;
                AEMtx33Scale(&scale, 30.0f, 30.0f); // Smaller than tile
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }

            //can be removed after the maze is finished



            // White border
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_NONE);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

            float thickness = 2.0f;
            AEMtx33 transform, scale, translate;

            // Top
            AEMtx33Scale(&scale, TILE_SIZE, thickness);
            AEMtx33Trans(&translate, x, y + half);
            AEMtx33Concat(&transform, &translate, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

            // Bottom
            AEMtx33Scale(&scale, TILE_SIZE, thickness);
            AEMtx33Trans(&translate, x, y - half);
            AEMtx33Concat(&transform, &translate, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

            // Left
            AEMtx33Scale(&scale, thickness, TILE_SIZE);
            AEMtx33Trans(&translate, x - half, y);
            AEMtx33Concat(&transform, &translate, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

            // Right
            AEMtx33Scale(&scale, thickness, TILE_SIZE);
            AEMtx33Trans(&translate, x + half, y);
            AEMtx33Concat(&transform, &translate, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
}



bool canMove(float nextX, float nextY)
{

    float collisionrad = (TILE_SIZE * 0.5f) - 1.0f;

    if (isBlockedAt(nextX + collisionrad, nextY)) { return false; }
    if (isBlockedAt(nextX - collisionrad, nextY)) { return false; }
    if (isBlockedAt(nextX, nextY + collisionrad)) { return false; }
    if (isBlockedAt(nextX, nextY - collisionrad)) { return false; }

    if (isBlockedAt(nextX + collisionrad, nextY + collisionrad)) { return false; }
    if (isBlockedAt(nextX - collisionrad, nextY + collisionrad)) { return false; }
    if (isBlockedAt(nextX + collisionrad, nextY - collisionrad)) { return false; }
    if (isBlockedAt(nextX - collisionrad, nextY - collisionrad)) { return false; }

    return true;
}