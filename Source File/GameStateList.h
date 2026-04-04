/* Start Header ***************************************************************
\file       GameStateList.h
\coders     Sharon, Jasmine, Yeong, San
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#pragma once

enum GS_STATES
{
	INTROSTATE = -1,
	MAINMENUSTATE = 0,
	LEVELINSTRUCTIONS,
	LEVELPAGE,
	GS_LEVEL1,
	GS_LEVEL2,
	GS_LEVEL3,
	CREATOR,
	INSTRUCTIONS,
	CONFIRM,
	CREDIT,
	GS_PAUSE,
	GS_WIN,
	GS_LOSE,
	GS_QUIT,
	GS_RESTART
};