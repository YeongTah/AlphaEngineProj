/* Start Header ***************************************************************
\file       GameStateManager.h
\coders     Sharon, Jasmine, Yeong, San
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#pragma once

typedef void(*FP)(void);

extern int current, previous, next;

extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

void GSM_Initialize(int startingState);
void GSM_Update();

extern int gLastLevelPlayed;