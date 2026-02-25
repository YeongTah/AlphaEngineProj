/* Start Header ************************************************************************/
/*!
\file   pch.h
\author Sharon Lim Joo Ai, sharonjooai.lim, 2502241
\par    sharonjooai.lim@digipen.edu
\date   January, 26, 2026
\brief  This file contains headers to be precompiled once, typically for includes that 
		are used consistently across all source files without redefinition.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

//======= ADD ANY HEADER FILES THAT IS USED IN MOST SOURCE FILES AND UNLIKELY TO BE CHANGED ======//
#include "GameStateList.h"
#include "AEEngine.h"
#include "AEInput.h"
#include "AEFrameRateController.h"
#include "leveleditor.hpp"

#endif //PCH_H
