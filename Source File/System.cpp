/* Start Header ************************************************************************/
/*!
\file   System.cpp
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  This file defines the system state for a game, namely calling the initialization
		and exit of all other managers such as audio, graphics etc.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"

#include "System.h"
#include <iostream>

//----------------------------------------------------------------------------
// This function simulates the initialization of the system and prints it to the 
// standard output stream.
// ---------------------------------------------------------------------------
void System_Initialize() {
	// Print onto standard output stream
	std::cout << "System:Initialize\n";
}

//----------------------------------------------------------------------------
// This function simulates the exit of the system and prints it to the 
// standard output stream.
// ---------------------------------------------------------------------------
void System_Exit() {
	// Print onto standard output stream
	std::cout << "System:Exit\n";
}