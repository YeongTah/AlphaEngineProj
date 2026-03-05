#pragma once
#include "AEEngine.h"

//  changed later once mesh have functions
extern AEGfxVertexList* pMesh;
extern AEGfxTexture* gDesertBlockTex;
extern int level[18][32];  // aded to make global so that it can be used in Level1.cpp and LevelPage.cpp

void generateLevel(void);
int print_file(void);
void readfile(void);
bool canMove(float nextX, float nextY);

// Add this line so Level1.cpp can find the player's grid tile
void WorldToGrid(float worldX, float worldY, int& outRow, int& outCol);