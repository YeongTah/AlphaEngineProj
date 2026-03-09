#include "pch.h"

#include "AEEngine.h"
#include "leveleditor.hpp"
#include <fstream>
#include <iostream>
#include <math.h>
#include "GridUtils.h"

// Tile size is 50.0f -- matches GRID_TILE_SIZE in GridUtils.h
// level[][] is the SHARED global tile map used by ALL levels and the editor.
// Each level's Load function overwrites this array from its own .txt file.
int level[GRID_ROWS][GRID_COLS];

// Tile value meanings (used throughout leveleditor, GridUtils, and all Level files):
//   0  = empty / walkable floor
//   1  = NON_WALKABLE wall (rendered as DesertBlock, blocks movement)
//   2  = PLAYER_SPAWN  (reserved marker; actual spawn via FindFreeSpawnCell)
//   3  = ENEMY_SPAWN   (reserved marker; actual spawn via FindFreeSpawnCell)
//   4  = COIN          (collected when player steps on it; tile set to 0 on collect)
//   5+ = powerup tiles (Level1 only)
typedef enum Objects
{
    NON_WALKABLE = 1, // Wall tile -- impassable
    PLAYER_SPAWN = 2, // Player start marker (not used directly at runtime)
    ENEMY_SPAWN = 3, // Enemy start marker  (not used directly at runtime)
    COIN = 4  // Coin tile -- collected on player overlap
    /* NEW: value 8 = black buff block (5s immunity in Level1) -ths */
} Objects;

// Forward declarations for functions defined later in this file
int  print_file();
void readfile();

namespace
{
    int  gActiveLevel = 1; // Which level file the editor is currently editing (1, 2, or 3)
    bool gLocked = false;             // When true, mouse clicks do NOT paint tiles
    int  gBrushValue = NON_WALKABLE;      // Tile value placed when clicking the grid
    int  Selected = 1;                 // Currently highlighted button ID

    // Represents one UI button in the level editor panel
    struct Button
    {
        float pos_x, pos_y;   // World-space center of the button
        float width, height;  // Dimensions of the button rectangle
        float r, g, b;        // RGB fill color
        int   id;             // ButtonID enum value for click dispatch
    };

    // Identifies each editor panel button
    enum ButtonID
    {
        BTN_WALL,   // Paint wall (NON_WALKABLE) tiles
        BTN_ERASE,  // Erase tiles back to 0 (walkable)
        BTN_SAVE,   // Save current level[][] to the active .txt file
        BTN_LOAD,   // Reload level[][] from the active .txt file
        BTN_LOCK,   // Toggle tile painting on/off (lock mode)
        BTN_L1,     // Switch editor to level1.txt
        BTN_L2,     // Switch editor to level2.txt
        BTN_L3      // Switch editor to level3.txt
    };

    // All editor panel buttons -- positions are in world space (right side of screen)
    static Button const gButtons[] =
    {
        //pos_x  | pos_y  | width  | height|  r    |  g    |  b    | id
        {650.0f,  240.0f, 220.0f, 50.0f, 0.85f, 0.25f, 0.25f, BTN_WALL},   // Wall button (red)
        {650.0f,  180.0f, 220.0f, 50.0f, 0.75f, 0.75f, 0.75f, BTN_ERASE},  // Erase button (grey)
        {650.0f,  120.0f, 220.0f, 50.0f, 0.95f, 0.95f, 0.25f, BTN_SAVE},   // Save button (yellow)
        {650.0f,   60.0f, 220.0f, 50.0f, 0.25f, 0.95f, 0.95f, BTN_LOAD},   // Load button (cyan)
        {650.0f,    0.0f, 220.0f, 50.0f, 0.85f, 0.45f, 0.90f, BTN_LOCK},   // Lock button (purple)
        {575.0f, -120.0f,  70.0f, 50.0f, 0.60f, 0.60f, 0.60f, BTN_L1},    // Level 1 selector
        {650.0f, -120.0f,  70.0f, 50.0f, 0.60f, 0.60f, 0.60f, BTN_L2},    // Level 2 selector
        {725.0f, -120.0f,  70.0f, 50.0f, 0.60f, 0.60f, 0.60f, BTN_L3},    // Level 3 selector
    };

    static int const gButtonCount = (int)(sizeof(gButtons) / sizeof(gButtons[0]));

    // -------------------------------------------------------------------------
    // GetMouseWorld
    // Converts the raw screen cursor position (pixels, top-left origin) to
    // world space (center origin, Y-up).  Window assumed 1600x900.
    // -------------------------------------------------------------------------
    void GetMouseWorld(float& worldX, float& worldY)
    {
        int mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);
        worldX = (float)mouseX - 800.0f;  // shift so center = 0
        worldY = 450.0f - (float)mouseY;  // flip Y axis
    }

    // -------------------------------------------------------------------------
    // GetLevelFilename
    // Returns the .txt asset path for the currently active editor level.
    // Used by readfile() and print_file() to know which file to read/write.
    // -------------------------------------------------------------------------
    const char* GetLevelFilename()
    {
        switch (gActiveLevel)
        {
        case 1: return "Assets/level1.txt";
        case 2: return "Assets/level2.txt";
        case 3: return "Assets/level3.txt";
        default: return "Assets/level1.txt";
        }
    }

    // -------------------------------------------------------------------------
    // ClearLevelToZeros
    // Sets every cell in level[][] to 0 (empty/walkable).
    // Called when creating a blank new level or before loading a fresh file.
    // -------------------------------------------------------------------------
    void ClearLevelToZeros()
    {
        for (int row = 0; row < GRID_ROWS; ++row)
            for (int col = 0; col < GRID_COLS; ++col)
                level[row][col] = 0;
    }

    // -------------------------------------------------------------------------
    // isBlockedAt
    // Returns true if the grid cell under world position (worldX, worldY) has
    // value NON_WALKABLE (1), or if the position is outside the grid bounds.
    // Used internally by canMove() to validate movement.
    // -------------------------------------------------------------------------
    bool isBlockedAt(float worldX, float worldY)
    {
        int row, col;
        WorldToGrid(worldX, worldY, row, col);
        if (row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS)
            return true; // treat out-of-bounds as blocked
        return level[row][col] == NON_WALKABLE;
    }

    // -------------------------------------------------------------------------
    // PointInRect
    // Returns true if (mouse_x, mouse_y) is inside the rectangle defined by
    // the button's center and half-extents.  Used for editor UI hit-testing.
    // -------------------------------------------------------------------------
    bool PointInRect(float mouse_x, float mouse_y, Button const& button)
    {
        return (mouse_x >= button.pos_x - button.width * 0.5f &&
            mouse_x <= button.pos_x + button.width * 0.5f &&
            mouse_y >= button.pos_y - button.height * 0.5f &&
            mouse_y <= button.pos_y + button.height * 0.5f);
    }

    // -------------------------------------------------------------------------
    // DrawRect
    // Draws a solid colored rectangle at world position (centre_x, centre_y)
    // with the given dimensions, using the shared pMesh.
    // Used to draw editor UI backgrounds and button fill colors.
    // -------------------------------------------------------------------------
    void DrawRect(float centre_x, float centre_y, float width, float height,
        float r, float g, float b)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_NONE);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, 1.0f);
        AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
        AEMtx33 Scale, Transform, ConTrans;
        AEMtx33Scale(&Scale, width, height);
        AEMtx33Trans(&Transform, centre_x, centre_y);
        AEMtx33Concat(&ConTrans, &Transform, &Scale);
        AEGfxSetTransform(ConTrans.m);
        AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
    }

    // -------------------------------------------------------------------------
    // DrawButton
    // Renders one editor button.  If the button's id matches 'Selected',
    // draws a red highlight border (8px larger) before the button fill rect.
    // -------------------------------------------------------------------------
    void DrawButton(Button const& b)
    {
        if (b.id == Selected) // Highlight selected button with red border
        {
            DrawRect(b.pos_x, b.pos_y, b.width + 8.0f, b.height + 8.0f, 1.0f, 0.0f, 0.0f);
        }
        DrawRect(b.pos_x, b.pos_y, b.width, b.height, b.r, b.g, b.b);
    }

    // -------------------------------------------------------------------------
    // SetActiveLevel
    // Switches the editor to edit a different level file (1, 2, or 3).
    // If loadFromFile is true, reads the existing .txt file into level[][].
    // If false, clears level[][] to all zeros (blank canvas).
    // -------------------------------------------------------------------------
    void SetActiveLevel(int newLevel, bool loadFromFile)
    {
        gActiveLevel = newLevel;
        if (gActiveLevel < 1) gActiveLevel = 1;
        if (gActiveLevel > 3) gActiveLevel = 3;
        if (loadFromFile) readfile();
        else              ClearLevelToZeros();
    }

    // -------------------------------------------------------------------------
    // HandleButtonClick
    // Checks whether a mouse click at (mouseWorldX, mouseWorldY) hit any editor
    // button.  If so, sets the Selected button and performs the button action:
    //   BTN_WALL  -> set brush to paint NON_WALKABLE tiles
    //   BTN_ERASE -> set brush to erase (value 0)
    //   BTN_SAVE  -> write current level[][] to the active .txt file
    //   BTN_LOAD  -> reload level[][] from the active .txt file
    //   BTN_LOCK  -> toggle the locked/edit state
    //   BTN_L1/2/3 -> switch active level and reload from file
    // Returns true if a button was hit (caller skips grid painting in that case).
    // -------------------------------------------------------------------------
    bool HandleButtonClick(float mouseWorldX, float mouseWorldY)
    {
        for (int i = 0; i < gButtonCount; ++i)
        {
            if (!PointInRect(mouseWorldX, mouseWorldY, gButtons[i])) continue;

            Selected = gButtons[i].id;

            switch (gButtons[i].id)
            {
            case BTN_WALL:  gBrushValue = NON_WALKABLE; return true;
            case BTN_ERASE: gBrushValue = 0;            return true;
            case BTN_SAVE:  print_file(); gBrushValue = NON_WALKABLE; return true;
            case BTN_LOAD:  readfile();                 return true;
            case BTN_LOCK:  gLocked = !gLocked;         return true;
            case BTN_L1:    SetActiveLevel(1, true);    return true;
            case BTN_L2:    SetActiveLevel(2, true);    return true;
            case BTN_L3:    SetActiveLevel(3, true);    return true;
            default: break;
            }
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // DrawEditorUI
    // Renders the entire right-side editor panel each frame:
    //   1. Brown background rectangle behind the buttons.
    //   2. All editor buttons (with selection highlight via DrawButton).
    //   3. Text labels centered on each button using AEGfxPrint.
    //      The LOCK button label changes to "LOCKED" or "EDIT" based on state.
    // -------------------------------------------------------------------------
    void DrawEditorUI()
    {
        DrawRect(650.0f, 60.0f, 240.0f, 450.0f, 0.16f, 0.11f, 0.06f); // Brown UI background

        for (int i = 0; i < gButtonCount; ++i)
            DrawButton(gButtons[i]);

        if (fontId >= 0)
        {
            // NDC conversion constants: map world positions to AEGfxPrint's [-1,1] coordinate system
            float const HalfW = 1.0f / 800.0f;
            float const HalfH = 1.0f / 450.0f;

            AEGfxPrint(fontId, "WALL", (gButtons[0].pos_x * HalfW) - 0.05f, (gButtons[0].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "ERASE", (gButtons[1].pos_x * HalfW) - 0.06f, (gButtons[1].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "SAVE", (gButtons[2].pos_x * HalfW) - 0.05f, (gButtons[2].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "LOAD", (gButtons[3].pos_x * HalfW) - 0.05f, (gButtons[3].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);

            // Lock button text changes depending on current lock state
            if (gLocked)
                AEGfxPrint(fontId, "LOCKED", (gButtons[4].pos_x * HalfW) - 0.085f, (gButtons[4].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            else
                AEGfxPrint(fontId, "EDIT", (gButtons[4].pos_x * HalfW) - 0.055f, (gButtons[4].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);

            AEGfxPrint(fontId, "L1", (gButtons[5].pos_x * HalfW) - 0.02f, (gButtons[5].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "L2", (gButtons[6].pos_x * HalfW) - 0.02f, (gButtons[6].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "L3", (gButtons[7].pos_x * HalfW) - 0.02f, (gButtons[7].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
        }
    }
} // end anonymous namespace

// ----------------------------------------------------------------------------
// print_file  <-- THIS SAVES THE LEVEL TO DISK
// Writes the current level[][] contents to the active level .txt file
// (determined by GetLevelFilename() / gActiveLevel).
//
// Output format: each cell is written as  <value>,  with rows separated by
// newlines.  Example row for 32 cols: 0,1,0,0,1,0,0,...,0,
//
// This is the file that Level1/2/3 Load functions read back at game startup.
// The level editor calls this when the SAVE button is clicked.
// Returns 0 on success.
// ----------------------------------------------------------------------------
int print_file()
{
    std::ofstream os(GetLevelFilename());
    if (!os.is_open())
    {
        std::cout << "cannot find\n";
        return 0;
    }

    for (int i = 0; i < GRID_ROWS; ++i)
    {
        for (int j = 0; j < GRID_COLS; ++j)
            os << level[i][j] << ","; // value then comma, no space
        os << "\n"; // newline after each row
    }

    os.close();
    return 0;
}

// ----------------------------------------------------------------------------
// readfile  <-- THIS LOADS THE LEVEL FROM DISK
// Reads the active level .txt file (e.g. "Assets/level1.txt") and populates
// level[][] with the tile values.
//
// Expected format: each cell is <value>,  rows separated by newlines.
// Reads exactly GRID_ROWS * GRID_COLS (int, char) pairs.
//
// If the file cannot be opened, calls LoadDefaultLevel() (which places a
// minimal default layout: two walls at [5][5] and [5][6], rest zeros).
//
// Also called at runtime by Level1/2/3 Load functions indirectly through
// the level-specific LoadLevelTxt wrappers.
// ----------------------------------------------------------------------------
void readfile()
{
    std::ifstream is(GetLevelFilename());
    if (!is.is_open())
    {
        LoadDefaultLevel(); // fall back to a minimal default map
        std::cout << "No file: " << GetLevelFilename() << " starting blank\n";
        return;
    }

    int  tile;
    char comma;

    for (int row = 0; row < GRID_ROWS; row++)
        for (int col = 0; col < GRID_COLS; col++)
        {
            is >> tile >> comma; // reads: integer value then ',' character
            level[row][col] = tile;
        }

    is.close();
}

// ----------------------------------------------------------------------------
// generateLevel
// Called each frame when the Level Editor game state is active.
// Handles two responsibilities:
//
//   INPUT (on left-mouse-button triggered):
//     1. Gets mouse position in world space.
//     2. Checks if the click hit an editor UI button (HandleButtonClick).
//        - If yes: execute the button action; do NOT paint the grid.
//        - If no AND not locked: convert mouse world pos to grid coords and
//          set level[row][col] = gBrushValue (currently selected tile value).
//
//   RENDERING (every frame):
//     1. Iterates all grid cells and draws the DesertBlock texture on any cell
//        with value NON_WALKABLE (1).
//     2. Calls DrawEditorUI() to render the right-side editor panel.
//
// NOTE: Floor tiles (value 0) are NOT drawn here -- the editor shows an empty
// background for walkable cells so the grid structure is clear.
// ----------------------------------------------------------------------------
void generateLevel(void)
{
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        float wx, wy;
        GetMouseWorld(wx, wy); // convert cursor to world coordinates

        // If click hit a UI button, handle it and skip grid painting
        if (!HandleButtonClick(wx, wy))
        {
            if (!gLocked) // Only paint when editor is in edit (unlocked) mode
            {
                int row, col;
                WorldToGrid(wx, wy, row, col); // find which grid cell was clicked

                if (row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS)
                    level[row][col] = gBrushValue; // paint with active brush value
            }
        }
    }

    // Draw all wall tiles using the DesertBlock texture
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            float x, y;
            GridToWorldCenter(row, col, x, y); // world center of this tile

            if (level[row][col] == NON_WALKABLE)
            {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxTextureSet(gDesertBlockTex, 0, 0);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

                AEMtx33 scale, translate, transform;
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&translate, x, y);
                AEMtx33Concat(&transform, &translate, &scale);

                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    DrawEditorUI(); // Draw right-side editor panel on top of tiles
}

// ----------------------------------------------------------------------------
// canMove
// Returns true if it is safe for an entity centered at (nextX, nextY) to move
// there, based on the current level[][] grid.
//
// Performs 8-point collision sampling:  center-edges (N/S/E/W) and all four
// corners of a square with radius = (GRID_TILE_SIZE / 2) - 1 pixel.
// If ANY sampled point lands on a NON_WALKABLE cell (value 1) or out of bounds,
// the move is blocked.
//
// Used by the mummy AI in all levels to prevent enemies from walking through walls.
// (Player movement uses IsTileWalkable() instead -- a simpler single-point check.)
// ----------------------------------------------------------------------------
bool canMove(float nextX, float nextY)
{
    float collisionrad = (GRID_TILE_SIZE * 0.5f) - 1.0f; // slightly inside tile edge

    // Cardinal direction checks
    if (isBlockedAt(nextX + collisionrad, nextY)) return false; // right
    if (isBlockedAt(nextX - collisionrad, nextY)) return false; // left
    if (isBlockedAt(nextX, nextY + collisionrad)) return false; // up
    if (isBlockedAt(nextX, nextY - collisionrad)) return false; // down

    // Corner checks
    if (isBlockedAt(nextX + collisionrad, nextY + collisionrad)) return false; // top-right
    if (isBlockedAt(nextX - collisionrad, nextY + collisionrad)) return false; // top-left
    if (isBlockedAt(nextX + collisionrad, nextY - collisionrad)) return false; // bottom-right
    if (isBlockedAt(nextX - collisionrad, nextY - collisionrad)) return false; // bottom-left

    return true; // all 8 sample points are on walkable tiles
}