// LosePage.h
/******************************************************************************/
/*!
\author     Thu Htoo San
\date       April, 5 , 2026
\copyright  Copyright (C) 2013 DigiPen Institute of Technology. Reproduction
            or disclosure of this file or its contents without the prior
            written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/
#pragma once

// ----------------------------------------------------------------------------
// LosePage_Load
// Loads resources for the Lose page (audio, mesh).
// ----------------------------------------------------------------------------
void LosePage_Load();

// ----------------------------------------------------------------------------
// LosePage_Initialize
// Initialises the Lose page state.
// ----------------------------------------------------------------------------
void LosePage_Initialize();

// ----------------------------------------------------------------------------
// LosePage_Update
// Handles input and transitions from the Lose page.
// ----------------------------------------------------------------------------
void LosePage_Update();

// ----------------------------------------------------------------------------
// LosePage_Draw
// Renders the Lose page overlay.
// ----------------------------------------------------------------------------
void LosePage_Draw();

// ----------------------------------------------------------------------------
// LosePage_Free
// Frees runtime data for the Lose page.
// ----------------------------------------------------------------------------
void LosePage_Free();

// ----------------------------------------------------------------------------
// LosePage_Unload
// Unloads all resources used by the Lose page.
// ----------------------------------------------------------------------------
void LosePage_Unload();