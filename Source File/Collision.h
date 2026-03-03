#pragma once

#include "AEEngine.h"

// Sharon: feel free to add any collision that you all neeed in here
// Collision structure

//struct AABB
//{
//	AEVec2	min;
//	AEVec2	max;
//};
//
////								Collisions between square and square
//bool SquareSquareCollision_Static(const AABB& aabb1, const AABB& aabb2);	
//
//bool SquareSquareCollision_Dynamic(const AABB& aabb1,					//Input
//	const AEVec2& vel1,					//Input 
//	const AABB& aabb2,					//Input 
//	const AEVec2& vel2,					//Input
//	float& firstTimeOfCollision);		//Output: the calculated value of tFirst

// Collision between cursor and rectangle
bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y);

