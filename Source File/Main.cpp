//																	--Files to include--						
#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "leveleditor.hpp"
#include <math.h>   // Added for fabsf() -- fabsolute value function to ensure no negative results
//jas change - coordinates for the entities, reset, and canmove function added readfile();
//print_file(), generateLevel() and added mesh

// MAIN
//																--- Variables declaration start here ---

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
	//float width, height; // Size of the sprite			 -- Uncomment when textures are used --
	AEGfxTexture* pTex;  // Pointer to the loaded texture   -- Uncomment when textures are used --
	float size;          // Square dimensions
	float r, g, b;       // Color components
};
Entity player;
Entity mummy;
Entity exitPortal;  //may call it exit portal                // Added for Win Condition 
AEGfxVertexList* pMesh = nullptr; // Pointer for the square mesh
Entity coin;      // Added for Coin Collection
Entity wall;
int coinCounter = 0; // To track how many coins the player has collected

// Logic for balancing
int turnCounter = 0; // To make the mummy move every 2nd turn

s8 fontId = -1; // To store the loaded font ID
AEGfxTexture* gDesertBlockTex = nullptr;

//																--- Variables declaration end here ---

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	int gGameRunning = 1;


	// Using custom window procedure
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);

	//																	--- Initialization of variables ---

	// Load the font from the Assets folder with a size of 32
	fontId = AEGfxCreateFont("Assets/Roboto-Regular.ttf", 32);
	gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");


	//  Check if the font loaded successfully
	if (fontId < 0) {
		printf("Failed to load font!\n");
	}
	else {
		printf("Font Roboto-Regular in Assets folder loaded successfully with ID: %d\n", fontId);
	}
	//																Initialize Font System


	//																		Initialize Player
	// -- Uncomment when textures are used --	
	//player.x = 0.0f;
		//player.y = 0.0f;
		//player.width = 64.0f;
		//player.height = 64.0f;
		//player.pTex = AEGfxTextureLoad("Assets/Player.png"); // Ensure this path exists
	// \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
	// Check if font loaded successfully



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
	player.pTex = AEGfxTextureLoad("Assets/Player.jpg");
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


	//						
	//												Initialize Mummy  
	// -- Uncomment when textures are used --	
		//mummy.x = 200.0f;
		//mummy.y = 200.0f;
		//mummy.width = 64.0f;
		//mummy.height = 64.0f;
		//mummy.pTex = AEGfxTextureLoad("Assets/Mummy.png");
	// \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


		// 															--- Initialization of variables end here ---

		// Changing the window title
	AESysSetWindowTitle("Mummy Maze Balanced Prototype");
	readfile();
	print_file();
	//loadLevelMap(1);

	// reset the system modules
	AESysReset();

	printf("Hello World\n");

	// Game Loop
	while (gGameRunning)
	{
		// Informing the system about the loop's start
		AESysFrameStart();

		//		--- set bg to grey
		AEGfxSetBackgroundColor(0.30f, 0.22f, 0.12f);
		// Basic way to trigger exiting the application
		// when ESCAPE is hit or when the window is closed
		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
			gGameRunning = 0;

		//		
		// 															---	update logic goes here ---	





		bool playerMoved = false;
		float gridStep = 50.0f;
		float nextX = player.x;
		float nextY = player.y;


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
			// Reset Player and Mummy
			player.x = 225.0f;
			player.y = -125.0f;
			mummy.x = 325.0f;
			mummy.y = 175.0f;
			turnCounter = 0;

			// --- RESET COIN HERE ---
			coin.x = 25.0f;       // Position it somewhere in the middle
			coin.y = 75.0f;
			coinCounter = 0;

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



		//																			--- rendering logic goes here ---



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
		AEGfxSetColorToMultiply(wall.r, wall.g, wall.b, 1.0f);
		AEMtx33Scale(&scale, wall.size, wall.size);
		AEMtx33Trans(&trans, wall.x, wall.y);
		AEMtx33Concat(&transform, &trans, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);

		generateLevel();

		// Informing the system about the loop's end
		AESysFrameEnd();

	}

	AEGfxTextureUnload(player.pTex);
	AEGfxTextureUnload(gDesertBlockTex);
	// free the system
	AESysExit();
}