/* Start Header ***************************************************************
\file       leveleditor.hpp
\coders     Jasmine, Yeong
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#pragma once
#include "AEEngine.h"


extern AEGfxVertexList* pMesh;
extern AEGfxTexture* gDesertBlockTex;
extern s8 fontId;
extern int level[18][32];

void generateLevel(void);
int print_file(void);
void readfile(void);
bool canMove(float nextX, float nextY);
void Editor_Unload(void);