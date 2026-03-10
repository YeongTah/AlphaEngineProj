/* Start Header ************************************************************************/
/*!
\file   GameStateList.h
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  This file declares the common game states, from levels to exiting and restart.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

// Additional states for Pause / Win / Lose pages -ths
enum GS_STATES
{
	INTROSTATE = -1,
	MAINMENUSTATE = 0,
	LEVELPAGE,
	GS_LEVEL1,
	GS_LEVEL2,
	GS_LEVEL3,
	CREATOR,
	INSTRUCTIONS,
	CREDIT,
	GS_WIN,
	GS_QUIT,
	GS_RESTART
};