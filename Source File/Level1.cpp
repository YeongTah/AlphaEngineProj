/* Start Header ************************************************************************/
/*!
\file   Level1.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  This file defines the function Load, Initialize, Update, Draw, Free, Unload
        to produce the level in the game and manage their own counters loaded from text 
        files.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"

#include "Level1.h"
#include "gamestatemanager.h"
#include "Main.h"
#include <iostream>
#include <fstream>
//																--- Variables declaration start here ---

static bool level1_initialised = false; // initialisation flag

Entity player;
Entity mummy;
Entity exitPortal;  //may call it exit portal                // Added for Win Condition 
//static AEGfxVertexList* pMesh = nullptr; // Pointer for the square mesh
Entity coin;      // Added for Coin Collection
Entity wall;
int coinCounter = 0; // To track how many coins the player has collected

// Logic for balancing
int turnCounter = 0; // To make the mummy move every 2nd turn


AEGfxTexture* gDesertBlockTex = nullptr;

int level1_counter = 1;
int live1_counter = 3; // If want to include lives, adjust number of lives here

bool playerMoved = false;
float gridStep = 50.0f;
float nextX = player.x;
float nextY = player.y;

//																--- Variables declaration end here ---



//----------------------------------------------------------------------------
// Loads Level 1 resources and reads the level counter from a text file
//---------------------------------------------------------------------------
void Level1_Load()
{
    std::cout << "Level1:Load\n"; // Print onto standard output stream

	readfile();
	print_file();
	//loadLevelMap(1);

	// Loading of blue player texture
	player.pTex = AEGfxTextureLoad("Assets/Player.jpg");
	gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");

	//																Create a unit square mesh (centered at 0,0)
	AEGfxMeshStart();
	AEGfxTriAdd(-0.5f, -0.5f, 0x00FFFFFF, 0.0f, 1.0f,
		0.5f, -0.5f, 0x00FFFFFF, 1.0f, 1.0f,
		-0.5f, 0.5f, 0x00FFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(0.5f, -0.5f, 0x00FFFFFF, 1.0f, 1.0f,
		0.5f, 0.5f, 0x00FFFFFF, 1.0f, 0.0f,
		-0.5f, 0.5f, 0x00FFFFFF, 0.0f, 0.0f);
	pMesh = AEGfxMeshEnd();

}

//----------------------------------------------------------------------------
// Sets up the initial state and prepares it for gameplay
// ---------------------------------------------------------------------------
void Level1_Initialize()
{
	std::cout << "Level1:Initialize\n"; // Print onto standard output stream

	// Initialise positions only once
	if (!level1_initialised) {
		player.x = 225.0f;
		player.y = -125.0f;
		player.size = 40.0f;
		player.r = 0.0f; player.g = 0.0f; player.b = 1.0f;

		mummy.x = 325.0f;
		mummy.y = 175.0f;
		mummy.size = 40.0f;
		mummy.r = 1.0f; player.g = 0.0f; player.b = 0.0f;

		exitPortal.x = 425.0f;
		exitPortal.y = 25.0f;
		exitPortal.size = 40.0f;
		exitPortal.r = 1.0f; exitPortal.g = 1.0f; exitPortal.b = 0.0f;

		coin.x = 25.0f;
		coin.y = 75.0f;
		coin.size = 30.0f;
		coin.r = 1.0f; coin.g = 0.5f; coin.b = 0.0f;

		wall.x = -60.0f;
		wall.y = 0.0f;
		wall.size = 60.0f;
		wall.r = 0.2f; wall.g = 0.2f; wall.b = 0.2f;

		nextX = player.x;
		nextY = player.y;
		coinCounter = 0;
		turnCounter = 0;
		playerMoved = false;

		level1_initialised = true;
	}

	//						
	//												Initialize Mummy  
	// -- Uncomment when textures are used --	
//		mummy.x = 200.0f;
//		mummy.y = 200.0f;
//		mummy.width = 64.0f;
//		mummy.height = 64.0f;
//		mummy.pTex = AEGfxTextureLoad("Assets/Mummy.png");
//	
//
////																Initialize Font System
//	
//
//	//																		Initialize Player
//	// -- Uncomment when textures are used --	
//	player.x = 0.0f;
//		player.y = 0.0f;
//		player.width = 64.0f;
//		player.height = 64.0f;
//		player.pTex = AEGfxTextureLoad("Assets/Player.png"); // Ensure this path exists
	

}

//----------------------------------------------------------------------------
// Updates game logic and state each frame during gameplay
// ---------------------------------------------------------------------------
void Level1_Update()
{
	std::cout << "Level1:Update\n"; // Print onto standard output stream  yt 25-2 comment up first, my computer cannot stand D:
   
    level1_counter--; // Decrement counter for level
    

    if (level1_counter == 0)
    {
        // Level 1 completed
		//next = GS_LEVEL2;

		level1_initialised = false; // Reset for next time
		next = MAINMENUSTATE; // sharon: for now setting it as go back to main menu as havent set up level 2, hence the gsm would make it just loop until the system closes itself
    }

	//// sharon 2/3: Loop for level 1 with lives implemented. commented out for the time being until system is fixed
	//if (level1_counter <= 0)
	//{
	//	live1_counter--; // Decrement life
	//	// Level 1 iteration completed
	//	if (live1_counter > 0)
	//	{
	//		next = GS_RESTART; // Will restart Level 2
	//	}
	//	else
	//	{
	//		next = MAINMENUSTATE; // Sharon 2/3 : No lives left, for now setting it as go back to main menu as havent set up level 2, hence the gsm would make it just loop until the system closes itself
	//	}
	//}

	// MOVEMENT UPDATE
	if (AEInputCheckTriggered(AEVK_W))      nextY += gridStep;
	else if (AEInputCheckTriggered(AEVK_S)) nextY -= gridStep;
	else if (AEInputCheckTriggered(AEVK_A)) nextX -= gridStep;
	else if (AEInputCheckTriggered(AEVK_D)) nextX += gridStep;

	// Collision Check: Only move if the next position isn't the wall
	if ((nextX != player.x || nextY != player.y) && canMove(nextX, nextY)) {
		if (fabsf(nextX - wall.x) > 1.0f || fabsf(nextY - wall.y) > 1.0f) {
			player.x = nextX;
			player.y = nextY;
			playerMoved = true;
		}
	}

	//// Player Movement logic 
	//if (AEInputCheckTriggered(AEVK_W)) { player.y += gridStep; playerMoved = true; }
	//else if (AEInputCheckTriggered(AEVK_S)) { player.y -= gridStep; playerMoved = true; }
	//else if (AEInputCheckTriggered(AEVK_A)) { player.x -= gridStep; playerMoved = true; }
	//else if (AEInputCheckTriggered(AEVK_D)) { player.x += gridStep; playerMoved = true; }

			//													--- Basic Mummy AI (Balanced for winnable gameplay) ---


	if (playerMoved)
	{
		turnCounter++;

		// Mummy only moves every 2nd turn
		if (turnCounter % 2 == 0)
		{
			float nextMummyX = mummy.x;
			float nextMummyY = mummy.y;

			// 1. Calculate intended X movement
			if (mummy.x < player.x)      nextMummyX += gridStep;
			else if (mummy.x > player.x) nextMummyX -= gridStep;

			// 2. Check X collision: If the NEW X is the wall, stay at OLD X
			if (fabsf(nextMummyX - wall.x) < 1.0f && fabsf(mummy.y - wall.y) < 1.0f) {
				nextMummyX = mummy.x;
			}

			// 3. Calculate intended Y movement
			if (mummy.y < player.y)      nextMummyY += gridStep;
			else if (mummy.y > player.y) nextMummyY -= gridStep;

			// 4. Check Y collision: If the NEW Y is the wall, stay at OLD Y
			if (fabsf(nextMummyY - wall.y) < 1.0f && fabsf(nextMummyX - wall.x) < 1.0f) {
				nextMummyY = mummy.y;
			}

			// 5. Apply the final valid position
			mummy.x = nextMummyX;
			mummy.y = nextMummyY;
		}

		printf("Turn: %d | Player: (%.0f, %.0f) | Mummy: (%.0f, %.0f)\n",
			turnCounter, player.x, player.y, mummy.x, mummy.y);
	}

	//																--- Lose Condition (Caught by Mummy) ---
	if (fabsf(player.x - mummy.x) < 1.0f && fabsf(player.y - mummy.y) < 1.0f)
	{
		// Sharon 2/3: commented out these as its shifted to initialise and reset
		//// Reset Player and Mummy
		//player.x = 225.0f;
		//player.y = -125.0f;
		//mummy.x = 325.0f;
		//mummy.y = 175.0f;
		//turnCounter = 0;

		//// --- RESET COIN HERE ---
		//coin.x = 25.0f;       // Position it somewhere in the middle
		//coin.y = 75.0f;
		//coinCounter = 0;
		
		//							=== BETWEEN GS_RESTART AND MANUAL RESTART WHICH IS BETTER? NEED TO TEST ===
		//next = GS_RESTART;
		ResetLevel1();

		printf("Caught by the Mummy! Level Reset!\n");

	}

	//																--- Win Condition (Reached Exit Portal)  ---
	if (fabsf(player.x - exitPortal.x) < 1.0f && fabsf(player.y - exitPortal.y) < 1.0f)
	{
		// Sharon 2/3: commented out these as its shifted to initialise
		//// Reset Player and Mummy
		//player.x = 225.0f;
		//player.y = -125.0f;
		//mummy.x = 325.0f;
		//mummy.y = 175.0f;
		//turnCounter = 0;

		//// --- RESET COIN HERE ---
		//coin.x = 25.0f;       // Position it somewhere in the middle
		//coin.y = 75.0f;
		//coinCounter = 0;    // Reset score to 0 for the new attempt

		printf("You Escaped the Maze!\n");
		level1_counter = 0; // Trigger level completion
	}


	//																	--- Coin Collection Condition ---
	// We check if player is at the same grid position as the coin
	if (fabsf(player.x - coin.x) < 1.0f && fabsf(player.y - coin.y) < 1.0f)
	{
		coinCounter++;
		printf("Coin Collected! Total Coins: %d\n", coinCounter);

		// Move the coin off-screen or to a new spot so it doesn't trigger again immediately
		coin.x = 2000.0f;
		coin.y = 2000.0f;
	}



	// 																	--- update logic end here ---

}

//----------------------------------------------------------------------------
// Renders or draws the visual representation each frame 
// ---------------------------------------------------------------------------
void Level1_Draw()
{
	
	std::cout << "Level1:Draw\n"; // Print onto standard output stream  yt 25-2 comment up first, my computer cannot stand D:
	
	// Sharon 2/3: Creation of mesh AND player, wall, enemy positions is done in Load, not draw


	//																			--- rendering logic goes here ---


		// Set the background to white.
	AEGfxSetBackgroundColor(255.0f, 255.0f, 255.0f);

	//AEGfxSetRenderMode(AE_GFX_RM_COLOR); // Using colors, not textures
	AEMtx33 transform, scale, trans;

	//																			 Render Player with Texture
	// Use Texture mode for the player
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

	// Set up blending for transparency (this is the key fix!)
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Use white so the texture colors show correctly
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f); // Don't add any color
	AEGfxTextureSet(player.pTex, 0, 0); // Bind the player texture

	AEMtx33Scale(&scale, player.size, player.size);
	AEMtx33Trans(&trans, player.x, player.y);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);


	AEGfxSetRenderMode(AE_GFX_RM_COLOR); // Switch back to color mode for other entities
	AEGfxSetBlendMode(AE_GFX_BM_NONE); // Disable blending for solid colors


	// Render Mummy (Red Square)
	AEGfxSetColorToMultiply(mummy.r, mummy.g, mummy.b, 1.0f);
	AEMtx33Scale(&scale, mummy.size, mummy.size);
	AEMtx33Trans(&trans, mummy.x, mummy.y);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

	//																					Render exit portal (Yellow Square) 
	AEGfxSetColorToMultiply(exitPortal.r, exitPortal.g, exitPortal.b, 1.0f);
	AEMtx33Scale(&scale, exitPortal.size, exitPortal.size);
	AEMtx33Trans(&trans, exitPortal.x, exitPortal.y);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
	AEGfxPrint(fontId, "EXIT", 0.48f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	//																					Render Coin (Orange Square)

	if (coin.x < 1000.0f) // Only render if the coin hasn't been "eaten"
	{
		AEGfxSetColorToMultiply(coin.r, coin.g, coin.b, 1.0f);
		AEMtx33Scale(&scale, coin.size, coin.size);
		AEMtx33Trans(&trans, coin.x, coin.y);
		AEMtx33Concat(&transform, &trans, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

		// The text is now tied to the coin's active state
		AEGfxPrint(fontId, "COIN", -0.05f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}


	//																		Render Wall (Dark Grey Square)			
	/*AEGfxSetColorToMultiply(wall.r, wall.g, wall.b, 1.0f);
	AEMtx33Scale(&scale, wall.size, wall.size);
	AEMtx33Trans(&trans, wall.x, wall.y);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);*/

	// Does generate level create wall? if so, the above can be replaced
	generateLevel();
    
}

//----------------------------------------------------------------------------
// Cleans up dynamic resources while keeping static data 
// ---------------------------------------------------------------------------
void Level1_Free()
{
	std::cout << "Level1:Free\n"; // Print onto standard output stream
}

//----------------------------------------------------------------------------
// Unloads all resources completely when exiting the level 
// ---------------------------------------------------------------------------
void Level1_Unload()
{
	std::cout << "Level1:Unload\n"; // Print onto standard output stream

	AEGfxTextureUnload(player.pTex);
	AEGfxTextureUnload(gDesertBlockTex);
	// Unload font
	AEGfxDestroyFont(fontId);

	if (pMesh) {
		AEGfxMeshFree(pMesh);
		pMesh = nullptr;
	}

	level1_initialised = false; // Reset for next time level is loaded
}


//										===== HELPER FUNCTIONS =====
//----------------------------------------------------------------------------
// Resets level when mummy catches player. A manual reset is needed as
// initialise only runs once at the entry of a level, so need a reset for if
// player dies within the level itself.
// ---------------------------------------------------------------------------
void ResetLevel1()
{
	player.x = 225.0f;
	player.y = -125.0f;
	mummy.x = 325.0f;
	mummy.y = 175.0f;
	coin.x = 25.0f;
	coin.y = 75.0f;
	nextX = player.x;
	nextY = player.y;
	coinCounter = 0;
	turnCounter = 0;
	playerMoved = false;
}