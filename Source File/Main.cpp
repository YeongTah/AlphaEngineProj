// ---------------------------------------------------------------------------
// includes
#include "pch.h"
#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "GameStateManager.h"
#include "System.h"
#include "leveleditor.hpp"
#include "Level1.h"
#include "Main.h"

#include <iostream>
#include <fstream>
#include <math.h>   // Added for fabsf() -- fabsolute value function to ensure no negative results
//jas change - coordinates for the entities, reset, and canmove function added readfile();
//print_file(), generateLevel() and added mesh


// MAIN

//																--- Variables declaration start here ---

//																--- Variables declaration end here ---
s8 fontId = -1;										//global variable for font ID, initialized to -1 to indicate not loaded

//															--- helper header ----   yt 4/3 
//// Function to create a standard 1x1 square mesh
//AEGfxVertexList* CreateSquareMesh() {
//	AEGfxMeshStart();
//
//	// Triangle 1
//	AEGfxTriAdd(
//		-0.5f, -0.5f, 0x00FFFFFF, 0.0f, 1.0f,
//		0.5f, -0.5f, 0x00FFFFFF, 1.0f, 1.0f,
//		-0.5f, 0.5f, 0x00FFFFFF, 0.0f, 0.0f);
//
//	// Triangle 2
//	AEGfxTriAdd(
//		0.5f, -0.5f, 0x00FFFFFF, 1.0f, 1.0f,
//		0.5f, 0.5f, 0x00FFFFFF, 1.0f, 0.0f,
//		-0.5f, 0.5f, 0x00FFFFFF, 0.0f, 0.0f);
//
//	return AEGfxMeshEnd();
//}
// to trigger for example in Level1_Load() like this: pMesh = CreateSquareMesh();


AEGfxVertexList* pMesh = nullptr;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialization of your own variables go here
	

	// 															--- Initialization of variables end here ---
	// IF THERE IS NO OTHER MANAGERS BUILT, CAN REMOVE THIS AS IT DOES THE SAME AS AESysInit
	// Initialize system components
    System_Initialize();

    // Initialize game state manager
    GSM_Initialize(MAINMENUSTATE);

    // Using custom window procedure
    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);
	
		// Load the font from the Assets folder with a size of 32
	fontId = AEGfxCreateFont("Assets/Roboto-Regular.ttf", 32);
//	gDesertBlockTex = AEGfxTextureLoad("Assets/DesertBlock.png");


	//  Check if the font loaded successfully
	if (fontId < 0) {
		printf("Failed to load font!\n");
	}
	else {
		printf("Font Roboto-Regular in Assets folder loaded successfully with ID: %d\n", fontId);
	}

    // Changing the window title
	AESysSetWindowTitle("Mummy Maze Balanced Prototype");

	//																	--- GAME STATE MANAGER LOOP ---
	// Controls the transition between games
	while (current != GS_QUIT)
	{
		AESysReset();

		// Your own update logic goes here

		// If current game state is not equal to restart (game loop)
		if (current != GS_RESTART)
		{
			// Call "Update" for current level


			GSM_Update();  // This handles state switching and function pointer setup
			fpLoad(); // Load current level
		}
		else
		{
			// Set next game state to be equal to previous game state
			next = previous;
			// set current game state to be equal to previous game state
			current = previous;
		}

		// Call "init" for current game state
		fpInitialize();

		//																	--- START OF GAME LOOP ---
		// THE WHILE LOOP BELOW CONSIST OF THE ACTUAL GAME LOOP. DO NOT ADJUST 
		// While next game state is equal to current game state
		while (next == current)
		{
			// Informing the system about the loop's start
			AESysFrameStart();

			// Update input status by calling input function
			//std::cout << "Input\n"; // Debug purposes    -- yt 25-2 comment up first, my computer cannot stand D:

			// Call update for current level
			fpUpdate();

			// Call "draw" for current level
			fpDraw();

			// Informing the system about the loop's end
			AESysFrameEnd();

		}
		//																	--- END OF GAME LOOP ---

		// Call "free" for current level
		fpFree();

		// If next game state is not equal to restart
		if (next != GS_RESTART)
		{
			// Call "unload" for current level
			fpUnload();
		}

		// Set previous game state to be equal to current game state
		previous = current;
		// Set current game state to be equal to next game state
		current = next;

		// Your own rendering logic goes here

	}

	// IF THERE IS NO OTHER MANAGERS BUILT, CAN REMOVE THIS AS IT DOES THE SAME AS AESysExit
	// Terminate system components
	System_Exit();

	// Unload font
	AEGfxDestroyFont(fontId);

	// free the system
	AESysExit();


}