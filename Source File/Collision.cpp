#include "pch.h"

#include "Collision.h"

// ----------------------------------------------------------------------------
// IsAreaClicked
// For button clicking. 
// Check if click is within the area calculated from the center. Returns
// True if it is, False otherwise.
// ----------------------------------------------------------------------------
bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, s32 click_x, s32 click_y) {
    
    if (click_x >= area_center_x - area_width / 2 &&
        click_x <= area_center_x + area_width / 2 &&
        click_y >= area_center_y - area_height / 2 &&
        click_y <= area_center_y + area_height / 2) {
        return true;
    }
    return false;
}