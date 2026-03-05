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


/*Entity structure
* * @brief  Structure to hold basic entity information
* @params x - X position in world space
* @params y - Y position in world space
* @params width - Width of the sprite
* @params height - Height of the sprite
* @params pTex - Pointer to the loaded texture
*/

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

    //// Read file to load level 1 counters
    //std::ifstream file1("Level1_Counter.txt");
    //if (file1.is_open())
    //{
    //    file1 >> level1_counter; // Process counter
    //}

	readfile();
	print_file();
	//loadLevelMap(1);

	// Loading of blue player texture
	player.pTex = AEGfxTextureLoad("Assets/Player.jpg");

}

//----------------------------------------------------------------------------
// Sets up the initial state and prepares it for gameplay
// ---------------------------------------------------------------------------
void Level1_Initialize()
{
	std::cout << "Level1:Initialize\n"; // Print onto standard output stream

<<<<<<< Updated upstream
	//						
	//												Initialize Mummy  
	// -- Uncomment when textures are used --	
		//mummy.x = 200.0f;
		//mummy.y = 200.0f;
		//mummy.width = 64.0f;
		//mummy.height = 64.0f;
		//mummy.pTex = AEGfxTextureLoad("Assets/Mummy.png");
	// \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

//																Initialize Font System
	
=======
	// Initialise positions only once
	if (!level1_initialised) {
		player.x = 225.0f;
		player.y = -125.0f;
	/*	player.size = 40.0f;*/
		player.size = 50.0f; // Adjusted to fit the grid better     as the size of grid is 50.0f in leveleditor -- YT
		player.r = 0.0f; player.g = 0.0f; player.b = 1.0f;

		mummy.x = 325.0f;
		mummy.y = 175.0f;
		//mummy.size = 40.0f;
		mummy.size = 50.0f; // Adjusted to fit the grid better
		mummy.r = 1.0f; mummy.g = 0.0f; mummy.b = 0.0f;

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
		//wall.size = 60.0f;
		wall.size = 50.0f; // Adjusted to fit the grid better
		wall.r = 0.2f; wall.g = 0.2f; wall.b = 0.2f;

		nextX = player.x;
		nextY = player.y;
		coinCounter = 0;
		turnCounter = 0;
		playerMoved = false;

		level1_initialised = true;
	}


	// sharon 3/3: Not sure if the below is necessary anymore. Have moved out the texture part to load instead
//	//						
//	//												Initialize Mummy  
//	// -- Uncomment when textures are used --	
//		mummy.x = 200.0f;
//		mummy.y = 200.0f;
//		mummy.width = 64.0f;
//		mummy.height = 64.0f;
//	
//
////																Initialize Font System
//	
//
//	//																		Initialize Player
//	// -- Uncomment when textures are used --	
//		player.x = 0.0f;
//		player.y = 0.0f;
//		player.width = 64.0f;
//		player.height = 64.0f;

>>>>>>> Stashed changes

	//																		Initialize Player
	// -- Uncomment when textures are used --	
	//player.x = 0.0f;
		//player.y = 0.0f;
		//player.width = 64.0f;
		//player.height = 64.0f;
		//player.pTex = AEGfxTextureLoad("Assets/Player.png"); // Ensure this path exists
	//
}

//----------------------------------------------------------------------------
// Updates game logic and state each frame during gameplay
// ---------------------------------------------------------------------------
void Level1_Update()
{
<<<<<<< Updated upstream
	//std::cout << "Level1:Update\n"; // Print onto standard output stream  yt 25-2 comment up first, my computer cannot stand D:
    
    level1_counter--; // Decrement counter for level
    
    if (level1_counter == 0)
    {
        // Level 1 completed
        next = GS_LEVEL2;
    }
=======
	// std::cout << "Level1:Update\n"; // Print onto standard output stream  yt 25-2 comment up first, my computer cannot stand D:

	level1_counter--; // Decrement counter for level


	if (level1_counter == 0)
	{
		// Level 1 completed
		//next = GS_LEVEL2;

		level1_initialised = false; // Reset for next time
		next = MAINMENUSTATE; // sharon: for now setting it as go back to main menu as havent set up level 2, hence the gsm would make it just loop until the system closes itself
	}
>>>>>>> Stashed changes

	// MOVEMENT UPDATE
	float testNextX = player.x;
	float testNextY = player.y;

	if (AEInputCheckTriggered(AEVK_W))      testNextY += gridStep;
	else if (AEInputCheckTriggered(AEVK_S)) testNextY -= gridStep;
	else if (AEInputCheckTriggered(AEVK_A)) testNextX -= gridStep;
	else if (AEInputCheckTriggered(AEVK_D)) testNextX += gridStep;

	// Bounding Box Collision Check for Player vs Wall
	bool playerWallCollision = (fabsf(testNextX - wall.x) < (player.size / 2.0f + wall.size / 2.0f)) &&
		(fabsf(testNextY - wall.y) < (player.size / 2.0f + wall.size / 2.0f));

	//// Collision Check: Only move if the next position isn't the wall
	//if ((testNextX != player.x || testNextY != player.y) && !playerWallCollision) {
	//	player.x = testNextX;
	//	player.y = testNextY;
	//	playerMoved = true;
	//																			-- the above commented out collision function is to replaced the grey wall with the gridbased wall.... -- YT
	// Replaced the old AABB wall check with the Grid check to fit the new grid-based movement and level design in level editor -- YT
	if ((testNextX != player.x || testNextY != player.y) && canMove(testNextX, testNextY)) {
		player.x = testNextX;
		player.y = testNextY;
		playerMoved = true;
	}
	

	//// Player Movement logic 
	//if (AEInputCheckTriggered(AEVK_W)) { player.y += gridStep; playerMoved = true; }
	//else if (AEInputCheckTriggered(AEVK_S)) { player.y -= gridStep; playerMoved = true; }
	//else if (AEInputCheckTriggered(AEVK_A)) { player.x -= gridStep; playerMoved = true; }
	//else if (AEInputCheckTriggered(AEVK_D)) { player.x += gridStep; playerMoved = true; }

	//													--- Basic Mummy AI (Balanced for winnable gameplay) ---   -- Debugging mummy movement 5/3 --  YT


	if (playerMoved)
	{
		turnCounter++;

		// 2. Coin Collection (Using value 4)
		int r, c;
		WorldToGrid(player.x, player.y, r, c);
		if (level[r][c] == 4) { // We use '4' because enum isn't in header
			level[r][c] = 0;    // Change back to EMPTY
			coinCounter++;
			std::cout << "Collected! Coins: " << coinCounter << "\n";
		}

		//																	Mummy only moves every 3rd turn (waits for 2 player movements)
		//                                                    
		if (turnCounter % 2 == 0)
		{
			float nextMummyX = mummy.x;
			float nextMummyY = mummy.y;

			// 1. Calculate direction for both axes
			float diffX = player.x - mummy.x;
			float diffY = player.y - mummy.y;

			// 2. Determine intended movement for X
			if (fabsf(diffX) > 1.0f) {
				nextMummyX += (diffX > 0) ? gridStep : -gridStep;
			}

			// 3. Determine intended movement for Y
			if (fabsf(diffY) > 1.0f) {
				nextMummyY += (diffY > 0) ? gridStep : -gridStep;
			}

			// 4. SMART COLLISION: Try diagonal first, then individual axes
			// This prevents the mummy from getting stuck if a diagonal is blocked
			if (canMove(nextMummyX, nextMummyY)) {
				mummy.x = nextMummyX;
				mummy.y = nextMummyY;
			}
			else if (canMove(nextMummyX, mummy.y)) {
				// If diagonal is blocked, try moving only horizontally
				mummy.x = nextMummyX;
			}
			else if (canMove(mummy.x, nextMummyY)) {
				// If horizontal is blocked, try moving only vertically
				mummy.y = nextMummyY;
			}
		}

		printf("Turn: %d | Player: (%.0f, %.0f) | Mummy: (%.0f, %.0f)\n",
			turnCounter, player.x, player.y, mummy.x, mummy.y);

		playerMoved = false; // Reset flag after processing turn
	}

	//																--- Lose Condition (Caught by Mummy) ---
	if (fabsf(player.x - mummy.x) < 1.0f && fabsf(player.y - mummy.y) < 1.0f)
	{
		// Reset Player and Mummy
		player.x = 225.0f;
		player.y = -125.0f;
		mummy.x = 325.0f;
		mummy.y = 175.0f;
		turnCounter = 0;

<<<<<<< Updated upstream
		// --- RESET COIN HERE ---
		coin.x = 25.0f;       // Position it somewhere in the middle
		coin.y = 75.0f;
		coinCounter = 0;
=======
		//// --- RESET COIN HERE ---
		//coin.x = 25.0f;       // Position it somewhere in the middle
		//coin.y = 75.0f;
		//coinCounter = 0;

		//							=== BETWEEN GS_RESTART AND MANUAL RESTART WHICH IS BETTER? NEED TO TEST ===
		//next = GS_RESTART;
		ResetLevel1();
>>>>>>> Stashed changes

		printf("Caught by the Mummy! Level Reset!\n");

	}

	//																--- Win Condition (Reached Exit Portal)  ---
	if (fabsf(player.x - exitPortal.x) < 1.0f && fabsf(player.y - exitPortal.y) < 1.0f)
	{
		// Reset Player and Mummy
		player.x = 225.0f;
		player.y = -125.0f;
		mummy.x = 325.0f;
		mummy.y = 175.0f;
		turnCounter = 0;

		// --- RESET COIN HERE ---
		coin.x = 25.0f;       // Position it somewhere in the middle
		coin.y = 75.0f;
		coinCounter = 0;    // Reset score to 0 for the new attempt

		printf("You Escaped the Maze!\n");
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
<<<<<<< Updated upstream
	//																Create a unit square mesh (centered at 0,0)
	AEGfxMeshStart();
	AEGfxTriAdd(-0.5f, -0.5f, 0x00FFFFFF, 0.0f, 1.0f,
		0.5f, -0.5f, 0x00FFFFFF, 1.0f, 1.0f,
		-0.5f, 0.5f, 0x00FFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(0.5f, -0.5f, 0x00FFFFFF, 1.0f, 1.0f,
		0.5f, 0.5f, 0x00FFFFFF, 1.0f, 0.0f,
		-0.5f, 0.5f, 0x00FFFFFF, 0.0f, 0.0f);
	pMesh = AEGfxMeshEnd();
	//																		2. Setup Player (Blue) 
	player.x = 225.0f;
	player.y = -125.0f;
	player.size = 40.0f;
	player.r = 0.0f; player.g = 0.0f; player.b = 1.0f;
	
	//																			3. Setup Mummy (Red) 
	mummy.x = 325.0f;
	mummy.y = 175.0f;
	mummy.size = 40.0f;
	mummy.r = 1.0f; mummy.g = 0.0f; mummy.b = 0.0f;

	//																			4. Setup Treasure (Yellow) 
	exitPortal.x = 425.0f;
	exitPortal.y = 25.0f;
	exitPortal.size = 40.0f;
	exitPortal.r = 1.0f; exitPortal.g = 1.0f; exitPortal.b = 0.0f;

	//																			 5. Setup Coin (Orange) 
	coin.x = 25.0f;       // Position it somewhere in the middle
	coin.y = 75.0f;
	coin.size = 30.0f;   // Slightly smaller than the player
	coin.r = 1.0f;
	coin.g = 0.5f;
	coin.b = 0.0f; //rgb Orange  

	//																			6. Initialise Wall 
	wall.x = -60.0f; // Example position
	wall.y = 0.0f;
	wall.size = 60.0f;
	wall.r = 0.2f; wall.g = 0.2f; wall.b = 0.2f; // Dark Grey
    
    //====== INCLUDE RENDERING HERE ======//
=======

	//std::cout << "Level1:Draw\n"; // Print onto standard output stream  yt 25-2 comment up first, my computer cannot stand D:

	// Sharon 2/3: Creation of mesh AND player, wall, enemy positions is done in Load, not draw
>>>>>>> Stashed changes


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

	//if (coin.x < 1000.0f) // Only render if the coin hasn't been "eaten"
	//{
	//	AEGfxSetColorToMultiply(coin.r, coin.g, coin.b, 1.0f);
	//	AEMtx33Scale(&scale, coin.size, coin.size);
	//	AEMtx33Trans(&trans, coin.x, coin.y);
	//	AEMtx33Concat(&transform, &trans, &scale);
	//	AEGfxSetTransform(transform.m);
	//	AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

	//	// The text is now tied to the coin's active state
	//	AEGfxPrint(fontId, "COIN", -0.05f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	//}


	////																		Render Wall (Dark Grey Square)			   -- comment this out cause there is already walll in leveleditor -- YT
	//AEGfxSetColorToMultiply(wall.r, wall.g, wall.b, 1.0f);
	//AEMtx33Scale(&scale, wall.size, wall.size);
	//AEMtx33Trans(&trans, wall.x, wall.y);
	//AEMtx33Concat(&transform, &trans, &scale);
	//AEGfxSetTransform(transform.m);
	//AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

	// This generates the level for each level. need an additional source file for it   -- uncommmented this file to collaborate with level editor.
	generateLevel();

<<<<<<< Updated upstream
	generateLevel();

	// Informing the system about the loop's end
	AESysFrameEnd();
    
    //std::cout << "Level1:Draw\n"; // Print onto standard output stream  yt 25-2 comment up first, my computer cannot stand D:
=======
>>>>>>> Stashed changes
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
}