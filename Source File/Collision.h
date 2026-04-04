/* Start Header ***************************************************************
\file       Collision.h
\coders     Sharon
Copyright (C) 2026 DigiPen Institute of Technology.
*/
/* End Header *************************************************************** */

#pragma once

#include "AEEngine.h"

// Collision between cursor and rectangle
bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, s32 click_x, s32 click_y);

