#pragma once
#include "AEEngine.h"

//  changed later once mesh have functions
extern AEGfxVertexList* pMesh;
extern AEGfxTexture* gDesertBlockTex;
extern s8 fontId;

void generateLevel(void);
int print_file(void);
void readfile(void);
//bool canMove(float nextX, float nextY);