// WinPage.h
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
// WinPage_Load
// Loads resources for the Win page (audio, mesh).
// ----------------------------------------------------------------------------
void WinPage_Load();

// ----------------------------------------------------------------------------
// WinPage_Initialize
// Initialises the Win page state.
// ----------------------------------------------------------------------------
void WinPage_Initialize();

// ----------------------------------------------------------------------------
// WinPage_Update
// Handles input and transitions from the Win page.
// ----------------------------------------------------------------------------
void WinPage_Update();

// ----------------------------------------------------------------------------
// WinPage_Draw
// Renders the Win page overlay.
// ----------------------------------------------------------------------------
void WinPage_Draw();

// ----------------------------------------------------------------------------
// WinPage_Free
// Frees runtime data for the Win page.
// ----------------------------------------------------------------------------
void WinPage_Free();

// ----------------------------------------------------------------------------
// WinPage_Unload
// Unloads all resources used by the Win page.
// ----------------------------------------------------------------------------
void WinPage_Unload();