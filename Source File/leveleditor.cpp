#include "pch.h"
#include "AEEngine.h"
#include "leveleditor.hpp"
#include <fstream>
#include <iostream>
#include <math.h>
#include "GridUtils.h"
#include "Level1.h"

/*************************************************************************************************/
/*!
\file   leveleditor.h
\author your name here

\par    email: your email here

\brief
  This file contains the declaration of functions to generate a level from the level editor in game.

  Functions included
    - generateLevel
    - canMove

*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\brief
    This function writes the current level to a CSV text file.

*/
/*************************************************************************************************/
int print_file();

/*************************************************************************************************/
/*!
\brief
    This function reads the level from a text file and loads it into memory.

*/
/*************************************************************************************************/
void readfile();

int level[GRID_ROWS][GRID_COLS]; // Updated the array name to match the extern

/*-----------------------------------------------------------------------------------------------*/
/*!
    Enum for different object types in the level.

    0 - walkable space
    1 - non-walkable blocked tile
    4 - coin tile

*/
/*-----------------------------------------------------------------------------------------------*/
typedef enum Objects
{
    NON_WALKABLE = 1,
    ENEMY_SPAWN = 3,    // Added enemy spawn support for save/load -ths
    COIN = 4
} Objects;

namespace
{
    int gActiveLevel = 1;     // Current active level number
    bool gLocked = false;     // Lock editing
    int gBrushValue = NON_WALKABLE; // Active brush value
    int Selected = 1;         // Current selected button
    AEGfxTexture* CoinTex = nullptr;   // Coin texture reference
    bool EditorOpen = false;  // Is the side panel open?

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Button structure: position, size, color, ID
    */
    /*-------------------------------------------------------------------------------------------*/
    struct Button
    {
        float pos_x;
        float pos_y;
        float width;
        float height;
        float r;
        float g;
        float b;
        int id;
    };

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Enum for all button IDs
    */
    /*-------------------------------------------------------------------------------------------*/
    enum ButtonID
    {
        BTN_WALL,
        BTN_COIN,
        BTN_ERASE,
        BTN_SAVE,
        BTN_LOAD,
        BTN_LOCK,
        BTN_L1,
        BTN_L2,
        BTN_L3,
        BTN_ENEMY,       // New button for placing enemy spawn tile -ths
        BTN_EDITOR
    };

    Button const ToggleButton =
    {
        730.0f,
        400.0f,
        120.0f,
        40.0f,
        0.85f,
        0.25f,
        0.25f,
        BTN_EDITOR
    };

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Editor side panel buttons array
    */
    /*-------------------------------------------------------------------------------------------*/
    static Button const gButtons[] =
    {
        {650.0f, 240.0f, 220.0f, 50.0f, 0.95f, 0.65f, 0.35f, BTN_WALL},
        {650.0f, 180.0f, 220.0f, 50.0f, 0.95f, 0.85f, 0.45f, BTN_COIN},
        {650.0f, 120.0f, 220.0f, 50.0f, 0.75f, 0.75f, 0.75f, BTN_ERASE},
        {650.0f,  60.0f, 220.0f, 50.0f, 0.55f, 0.85f, 0.55f, BTN_SAVE},
        {650.0f,   0.0f, 220.0f, 50.0f, 0.55f, 0.70f, 0.95f, BTN_LOAD},
        {650.0f, -60.0f, 220.0f, 50.0f, 0.80f, 0.65f, 0.95f, BTN_LOCK},
        {575.0f, -180.0f, 70.0f, 50.0f, 0.95f, 0.75f, 0.85f, BTN_L1},
        {650.0f, -180.0f, 70.0f, 50.0f, 0.95f, 0.75f, 0.85f, BTN_L2},
        {725.0f, -180.0f, 70.0f, 50.0f, 0.95f, 0.75f, 0.85f, BTN_L3},

        // NEW enemy tile button -ths
        {650.0f, 300.0f, 220.0f, 50.0f, 0.95f, 0.45f, 0.45f, BTN_ENEMY}
    };

    static int const gButtonCount =
        (int)(sizeof(gButtons) / sizeof(gButtons[0]));

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Convert mouse cursor from screen space to world coordinates.

    */
    /*-------------------------------------------------------------------------------------------*/
    void GetMouseWorld(float& worldX, float& worldY)
    {
        int mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);

        worldX = (float)mouseX - 800.0f;
        worldY = 450.0f - (float)mouseY;
    }

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Return filename of currently active level.

    */
    /*-------------------------------------------------------------------------------------------*/
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

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Clear the level array to all zeros.

    */
    /*-------------------------------------------------------------------------------------------*/
    void ClearLevelToZeros()
    {
        for (int row = 0; row < GRID_ROWS; ++row)
        {
            for (int col = 0; col < GRID_COLS; ++col)
            {
                level[row][col] = 0;
            }
        }
    }

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Check if there's a non-walkable tile at given position.

    */
    /*-------------------------------------------------------------------------------------------*/
    bool isBlockedAt(float worldX, float worldY)
    {
        int row, col;
        WorldToGrid(worldX, worldY, row, col);

        if (row < 0 || row >= GRID_ROWS ||
            col < 0 || col >= GRID_COLS)
            return true;

        return level[row][col] == NON_WALKABLE;
    }

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Check if mouse point is inside a UI button.

    */
    /*-------------------------------------------------------------------------------------------*/
    bool PointInRect(float mouse_x, float mouse_y, Button const& button)
    {
        return (mouse_x >= button.pos_x - button.width * 0.5f &&
            mouse_x <= button.pos_x + button.width * 0.5f &&
            mouse_y >= button.pos_y - button.height * 0.5f &&
            mouse_y <= button.pos_y + button.height * 0.5f);
    }

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Draw a rectangle with given color.

    */
    /*-------------------------------------------------------------------------------------------*/
    void DrawRect
    (float cx, float cy, float w, float h, float r, float g, float b)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_NONE);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, 1.0f);

        AEMtx33 scale, trans, result;
        AEMtx33Scale(&scale, w, h);
        AEMtx33Trans(&trans, cx, cy);
        AEMtx33Concat(&result, &trans, &scale);

        AEGfxSetTransform(result.m);
        AEGfxMeshDraw
        (pMesh, AE_GFX_MDM_TRIANGLES);
    }

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Draw button background and highlight if selected.

    */
    /*-------------------------------------------------------------------------------------------*/
    void DrawButton(Button const& b)
    {
        if (b.id == Selected)
        {
            DrawRect(b.pos_x, b.pos_y,
                b.width + 8.0f, b.height + 8.0f,
                1.0f, 0.0f, 0.0f);
        }
        DrawRect(b.pos_x, b.pos_y,
            b.width, b.height,
            b.r, b.g, b.b);
    }

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Change active level and optionally load from file.

    */
    /*-------------------------------------------------------------------------------------------*/
    void SetActiveLevel(int newLevel, bool loadFromFile)
    {
        gActiveLevel = newLevel;
        if (gActiveLevel < 1)
            gActiveLevel = 1;
        if (gActiveLevel > 3)
            gActiveLevel = 3;

        if (loadFromFile)
            readfile();
        else
            ClearLevelToZeros();
    }

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Handle clicks on editor UI buttons.

    */
    /*-------------------------------------------------------------------------------------------*/
    bool HandleButtonClick(float mouseWorldX, float mouseWorldY)
    {
        if (PointInRect(mouseWorldX, mouseWorldY, ToggleButton))
        {
            EditorOpen = !EditorOpen;
            return true;
        }

        if (!EditorOpen)
            return false;

        for (int i = 0; i < gButtonCount; ++i)
        {
            if (!PointInRect(mouseWorldX, mouseWorldY, gButtons[i]))
                continue;

            Selected = gButtons[i].id;

            switch (gButtons[i].id)
            {
            case BTN_WALL:
                gBrushValue = NON_WALKABLE;
                return true;

            case BTN_COIN:
                gBrushValue = COIN;
                return true;

            case BTN_ERASE:
                gBrushValue = 0;
                return true;

            case BTN_SAVE:
                print_file();
                return true;

            case BTN_LOAD:
                readfile();
                return true;

            case BTN_LOCK:
                gLocked = !gLocked;
                return true;

            case BTN_L1:
                SetActiveLevel(1, true);
                return true;

            case BTN_L2:
                SetActiveLevel(2, true);
                return true;

            case BTN_L3:
                SetActiveLevel(3, true);
                return true;

            case BTN_ENEMY:
                gBrushValue = ENEMY_SPAWN;  // select enemy tile -ths
                return true;

            default:
                break;
            }
        }

        return false;
    }

    /*-------------------------------------------------------------------------------------------*/
    /*!
        Draw the side panel UI.

    */
    /*-------------------------------------------------------------------------------------------*/
    void DrawEditorUI()
    {
        DrawButton(ToggleButton);

        if (fontId >= 0)
        {
            float const HalfW = 1.0f / 800.0f;
            float const HalfH = 1.0f / 450.0f;

            if (EditorOpen)
            {
                AEGfxPrint(fontId, "CLOSE",
                    (ToggleButton.pos_x * HalfW) - 0.06f,
                    (ToggleButton.pos_y * HalfH) - 0.02f,
                    0.8f, 0, 0, 0, 1);
            }
            else
            {
                AEGfxPrint(fontId, "EDITOR",
                    (ToggleButton.pos_x * HalfW) - 0.07f,
                    (ToggleButton.pos_y * HalfH) - 0.02f,
                    0.8f, 0, 0, 0, 1);
            }
        }

        if (!EditorOpen)
            return;

        DrawRect(650.0f, 30.0f, 240.0f, 560.0f,
            0.16f, 0.11f, 0.06f);

        for (int i = 0; i < gButtonCount; ++i)
        {
            DrawButton(gButtons[i]);
        }

        if (fontId >= 0)
        {
            float const HalfW = 1.0f / 800.0f;
            float const HalfH = 1.0f / 450.0f;

            AEGfxPrint(fontId, "WALL",
                (gButtons[0].pos_x * HalfW) - 0.05f,
                (gButtons[0].pos_y * HalfH) - 0.02f,
                1.0f, 0, 0, 0, 1);

            AEGfxPrint(fontId, "COIN",
                (gButtons[1].pos_x * HalfW) - 0.05f,
                (gButtons[1].pos_y * HalfH) - 0.02f,
                1.0f, 0, 0, 0, 1);

            AEGfxPrint(fontId, "ERASE",
                (gButtons[2].pos_x * HalfW) - 0.06f,
                (gButtons[2].pos_y * HalfH) - 0.02f,
                1.0f, 0, 0, 0, 1);

            AEGfxPrint(fontId, "SAVE",
                (gButtons[3].pos_x * HalfW) - 0.05f,
                (gButtons[3].pos_y * HalfH) - 0.02f,
                1.0f, 0, 0, 0, 1);

            AEGfxPrint(fontId, "LOAD",
                (gButtons[4].pos_x * HalfW) - 0.05f,
                (gButtons[4].pos_y * HalfH) - 0.02f,
                1.0f, 0, 0, 0, 1);

            if (gLocked)
            {
                AEGfxPrint(fontId, "LOCKED",
                    (gButtons[5].pos_x * HalfW) - 0.085f,
                    (gButtons[5].pos_y * HalfH) - 0.02f,
                    1.0f, 0, 0, 0, 1);
            }
            else
            {
                AEGfxPrint(fontId, "EDIT",
                    (gButtons[5].pos_x * HalfW) - 0.055f,
                    (gButtons[5].pos_y * HalfH) - 0.02f,
                    1.0f, 0, 0, 0, 1);
            }

            AEGfxPrint(fontId, "L1",
                (gButtons[6].pos_x * HalfW) - 0.02f,
                (gButtons[6].pos_y * HalfH) - 0.02f,
                1.0f, 0, 0, 0, 1);

            AEGfxPrint(fontId, "L2",
                (gButtons[7].pos_x * HalfW) - 0.02f,
                (gButtons[7].pos_y * HalfH) - 0.02f,
                1.0f, 0, 0, 0, 1);

            AEGfxPrint(fontId, "L3",
                (gButtons[8].pos_x * HalfW) - 0.02f,
                (gButtons[8].pos_y * HalfH) - 0.02f,
                1.0f, 0, 0, 0, 1);

            // New ENEMY label -ths
            AEGfxPrint(fontId, "ENEMY",
                (gButtons[9].pos_x * HalfW) - 0.07f,
                (gButtons[9].pos_y * HalfH) - 0.02f,
                1.0f, 0, 0, 0, 1);
        }
    }

} // anonymous namespace end

/*************************************************************************************************/
/*!
\brief
    Writes the current level values to a CSV file.

*/
/*************************************************************************************************/
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
        {
            os << level[i][j] << ",";
        }
        os << "\n";
    }

    os.close();
    return 0;
}

/*************************************************************************************************/
/*!
\brief
    Read the level file into memory.

*/
/*************************************************************************************************/
void readfile()
{
    std::ifstream is(GetLevelFilename());

    if (!is.is_open())
    {
        LoadDefaultLevel();
        std::cout << "No file: " << GetLevelFilename() << " starting blank\n";
        return;
    }

    int tile;
    char comma;

    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            is >> tile >> comma;
            level[row][col] = tile;
        }
    }

    is.close();
}

/*************************************************************************************************/
/*!
\brief
    Generate the level editor, draw tiles, handle clicks.

*/
/*************************************************************************************************/
void generateLevel(void)
{
    CoinTex = AEGfxTextureLoad("Assets/Coin.png");

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        float wx, wy;
        GetMouseWorld(wx, wy);

        if (!HandleButtonClick(wx, wy))
        {
            if (!gLocked)
            {
                int row, col;
                WorldToGrid(wx, wy, row, col);

                if (row >= 0 && row < GRID_ROWS &&
                    col >= 0 && col < GRID_COLS)
                {
                    level[row][col] = gBrushValue;
                    std::cout << "Painted row=" << row
                        << " col=" << col
                        << " value=" << gBrushValue << "\n";
                }
            }
        }
    }

    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            float x, y;
            GridToWorldCenter(row, col, x, y);

            if (level[row][col] == NON_WALKABLE)
            {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxTextureSet(gDesertBlockTex, 0, 0);

                AEMtx33 scale, trans, res;
                AEMtx33Scale(&scale, GRID_TILE_SIZE, GRID_TILE_SIZE);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&res, &trans, &scale);

                AEGfxSetTransform(res.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }
            else if (level[row][col] == COIN)
            {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxTextureSet(CoinTex, 0, 0);

                AEMtx33 scale, trans, res;
                AEMtx33Scale(&scale,
                    GRID_TILE_SIZE * 0.8f,
                    GRID_TILE_SIZE * 0.8f);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&res, &trans, &scale);

                AEGfxSetTransform(res.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }

            else if (level[row][col] == ENEMY_SPAWN)  // Draw enemy tile -ths
            {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_NONE);
                AEGfxSetColorToMultiply(1.0f, 0.2f, 0.2f, 1.0f); // red tile -ths

                AEMtx33 scale, trans, res;
                AEMtx33Scale(&scale,
                    GRID_TILE_SIZE * 0.8f,
                    GRID_TILE_SIZE * 0.8f);
                AEMtx33Trans(&trans, x, y);
                AEMtx33Concat(&res, &trans, &scale);

                AEGfxSetTransform(res.m);
                AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    DrawEditorUI();
}

/*************************************************************************************************/
/*!
\brief
    Check if the player can move to a given position.

*/
/*************************************************************************************************/
bool canMove(float nextX, float nextY)
{
    float collisionrad = (GRID_TILE_SIZE * 0.5f) - 1.0f;

    if (isBlockedAt(nextX + collisionrad, nextY)) return false;
    if (isBlockedAt(nextX - collisionrad, nextY)) return false;
    if (isBlockedAt(nextX, nextY + collisionrad)) return false;
    if (isBlockedAt(nextX, nextY - collisionrad)) return false;

    if (isBlockedAt(nextX + collisionrad, nextY + collisionrad)) return false;
    if (isBlockedAt(nextX - collisionrad, nextY + collisionrad)) return false;
    if (isBlockedAt(nextX + collisionrad, nextY - collisionrad)) return false;
    if (isBlockedAt(nextX - collisionrad, nextY - collisionrad)) return false;

    return true;
}