/* Start Header ***************************************************************
\file       Level1.h
\coders     Sharon, Jasmine, Yeong, San
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#pragma once

void Level1_Load();

void Level1_Initialize();

void Level1_Update();

void Level1_Draw();

void Level1_Free();

void Level1_Unload();

/*Entity structure
* * @brief  Structure to hold basic entity information
* @params x - X position in world space
* @params y - Y position in world space
* @params width - Width of the sprite
* @params height - Height of the sprite
* @params pTex - Pointer to the loaded texture
*/
struct Entity {
	float x, y;          // Position in world space
	float width, height; // Size of the sprite			 -- Uncomment when textures are used --
	AEGfxTexture* pTex;  // Pointer to the loaded texture   -- Uncomment when textures are used --
	float size;          // Square dimensions
	float r, g, b;       // Color components
};

void ResetLevel1();