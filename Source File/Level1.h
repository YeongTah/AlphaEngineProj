/* Start Header ************************************************************************/
/*!
\file   Level1.h
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  This file declares the function for Load, Initialize, Update, Draw, Free, Unload
        to produce the level in the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

void Level1_Load();

void Level1_Initialize();

void Level1_Update();

void Level1_Draw();

void Level1_Free();

void Level1_Unload();

struct Entity {
	float x, y;          // Position in world space
	float width, height; // Size of the sprite			 -- Uncomment when textures are used --
	AEGfxTexture* pTex;  // Pointer to the loaded texture   -- Uncomment when textures are used --
	float size;          // Square dimensions
	float r, g, b;       // Color components
};