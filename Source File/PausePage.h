// PausePage.h
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
// PausePage_Load
// Loads resources for the Pause page (audio, mesh).
// ----------------------------------------------------------------------------
void PausePage_Load();

// ----------------------------------------------------------------------------
// PausePage_Initialize
// Initialises the Pause page state.
// ----------------------------------------------------------------------------
void PausePage_Initialize();

// ----------------------------------------------------------------------------
// PausePage_Update
// Handles input and transitions from the Pause page.
// ----------------------------------------------------------------------------
void PausePage_Update();

// ----------------------------------------------------------------------------
// PausePage_Draw
// Renders the Pause page overlay.
// ----------------------------------------------------------------------------
void PausePage_Draw();

// ----------------------------------------------------------------------------
// PausePage_Free
// Frees runtime data for the Pause page.
// ----------------------------------------------------------------------------
void PausePage_Free();

// ----------------------------------------------------------------------------
// PausePage_Unload
// Unloads all resources used by the Pause page.
// ----------------------------------------------------------------------------
void PausePage_Unload();