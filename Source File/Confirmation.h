#pragma once

void Confirmation_Load();
void Confirmation_Initialize();
void Confirmation_Update();
void Confirmation_Draw();
void Confirmation_Free();
void Confirmation_Unload();

void Confirmation_Level(int currentState, int nextState, const char* message); 
// This function ius to be put inside the function you want to have the confirmation popup //jas