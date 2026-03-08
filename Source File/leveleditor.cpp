#include "AEEngine.h"
#include "leveleditor.hpp"
#include <fstream>
#include <iostream>
#include <math.h>
#include "GridUtils.h""

//tile size is 50.0f  YT   5/3
//global variables for tiles -- can consider to make it extern if needed
int level[GRID_ROWS][GRID_COLS];
//int ROWS = 18;
//int COLS = 32;
//float TILE_SIZE = 32.0f;

//can be used for map tile
typedef enum Objects
{
    NON_WALKABLE = 1,
    PLAYER_SPAWN = 2,
    ENEMY_SPAWN = 3,
    COIN = 4 // coin value in level1 txt -- YT
    /* NEW: value 8 = black buff block (5s immunity in Level1) -ths */
} Objects;

//declared first
int  print_file();
void readfile();

namespace
{
    int  gActiveLevel = 1; // 1..3
    bool gLocked = false; // true = cannot paint
    int  gBrushValue = NON_WALKABLE; // what value to paint when clicking grid
    int Selected = 1;

    struct Button
    {
        float pos_x, pos_y;// x and y coord center (world)
        float width, height; // width and height
        float r, g, b; //RGB colours
        int id; // what button from buttonID
    };

    enum ButtonID //button ID
    {
        BTN_WALL, BTN_ERASE, BTN_SAVE,
        BTN_LOAD, BTN_LOCK,
        BTN_L1, BTN_L2, BTN_L3
    };

    // button for text and clicking of button used button struct
    static Button const gButtons[] =
    {
        //pos_x | pos_y |  width | height| r | g | b | id |
        {650.0f,  240.0f, 220.0f, 50.0f, 0.85f, 0.25f, 0.25f, BTN_WALL},    //wall - draw the wall sprite
        {650.0f,  180.0f, 220.0f, 50.0f, 0.75f, 0.75f, 0.75f, BTN_ERASE},   //erase - remove the sprite wall 
        {650.0f,  120.0f, 220.0f, 50.0f, 0.95f, 0.95f, 0.25f, BTN_SAVE},    // save - save the edits to change binary 1 for non walkable tiles
        {650.0f,  60.0f, 220.0f, 50.0f, 0.25f, 0.95f, 0.95f, BTN_LOAD},     //load - if edited externally
        {650.0f,  0.0f, 220.0f, 50.0f, 0.85f, 0.45f, 0.90f, BTN_LOCK},      //lock - not be able to edit 
        {575.0f, -120.0f,  70.0f, 50.0f, 0.60f, 0.60f, 0.60f, BTN_L1},      // Go level 1
        {650.0f, -120.0f,  70.0f, 50.0f, 0.60f, 0.60f, 0.60f, BTN_L2},      // Go level 2
        {725.0f, -120.0f,  70.0f, 50.0f, 0.60f, 0.60f, 0.60f, BTN_L3},      //Go level 3
    };

    static int const gButtonCount = (int)(sizeof(gButtons) / sizeof(gButtons[0]));


    void GetMouseWorld(float& worldX, float& worldY)
    {
        int mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);
        worldX = (float)mouseX - 800.0f;
        worldY = 450.0f - (float)mouseY;
    }

    /*  void WorldToGrid(float worldX, float worldY, int& outRow, int& outCol)
      {
          float left = -(COLS * TILE_SIZE) * 0.5f;
          float top = +(ROWS * TILE_SIZE) * 0.5f;

          outCol = (int)std::floor((worldX - left) / TILE_SIZE);
          outRow = (int)std::floor((top - worldY) / TILE_SIZE);
      }*/

      //void GridToWorldCenter(int row, int col, float& outX, float& outY)
      //{
      //    float left = -(COLS * TILE_SIZE) * 0.5f;
      //    float top = +(ROWS * TILE_SIZE) * 0.5f;
      //
      //    outX = left + col * TILE_SIZE + (TILE_SIZE * 0.5f);
      //    outY = top - row * TILE_SIZE - (TILE_SIZE * 0.5f);
      //}

          //retrieve the file txt with binary numbers
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

    //clear the file contents to zero -- can be changed to another button
    void ClearLevelToZeros()
    {
        for (int row = 0; row < GRID_ROWS; ++row) {
            for (int col = 0; col < GRID_COLS; ++col) {
                level[row][col] = 0;
            }
        }
    }

    //edit out -- for player movement not needed in level editor
    bool isBlockedAt(float worldX, float worldY)
    {
        int row, col;
        WorldToGrid(worldX, worldY, row, col);

        if (row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS)
            return true;

        return level[row][col] == NON_WALKABLE;
    }


    //checks for mouse in the button
    bool PointInRect(float mouse_x, float mouse_y, Button const& button)
    {
        return (mouse_x >= button.pos_x - button.width * 0.5f &&
            mouse_x <= button.pos_x + button.width * 0.5f &&
            mouse_y >= button.pos_y - button.height * 0.5f &&
            mouse_y <= button.pos_y + button.height * 0.5f);
    }

    void DrawRect(float centre_x, float centre_y, float width, float height, float r, float g, float b)
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

    void DrawButton(Button const& b)
    {
        //draw border if is the selected button
        if (b.id == Selected) //if button is selected = 1 
        {
            DrawRect(b.pos_x, b.pos_y, b.width + 8.0f, b.height + 8.0f, 1.0f, 0.0f, 0.0f); // red border for selections
        }
        DrawRect(b.pos_x, b.pos_y, b.width, b.height, b.r, b.g, b.b);
    }

    void SetActiveLevel(int newLevel, bool loadFromFile)
    {
        gActiveLevel = newLevel;

        if (gActiveLevel < 1) {
            gActiveLevel = 1;
        }
        if (gActiveLevel > 3) {
            gActiveLevel = 3;
        }
        if (loadFromFile) {
            readfile();
        }
        else ClearLevelToZeros();
    }

    bool HandleButtonClick(float mouseWorldX, float mouseWorldY) //mouse click of the buttons
    {
        for (int i = 0; i < gButtonCount; ++i)
        {
            if (!PointInRect(mouseWorldX, mouseWorldY, gButtons[i])) {
                continue;
            }

            Selected = gButtons[i].id;

            switch (gButtons[i].id)
            {
            case BTN_WALL:  gBrushValue = NON_WALKABLE; return true;
            case BTN_ERASE: gBrushValue = 0;            return true;
            case BTN_SAVE:  print_file(); gBrushValue = NON_WALKABLE; return true;
            case BTN_LOAD:  readfile();                 return true; //can be changed to make all zero for better functionality 
            case BTN_LOCK:  gLocked = !gLocked;         return true;
            case BTN_L1:    SetActiveLevel(1, true);    return true;
            case BTN_L2:    SetActiveLevel(2, true);    return true;
            case BTN_L3:    SetActiveLevel(3, true);    return true;
            default: break;
            }
        }
        return false;
    }

    void DrawEditorUI()
    {
        DrawRect(650.0f, 60.0f, 240.0f, 450.0f, 0.16f, 0.11f, 0.06f); //brown background for button UI holder

        // Buttons
        for (int i = 0; i < gButtonCount; ++i) {
            DrawButton(gButtons[i]);
        }

        //Add text and set them in the middle of the buttons
        if (fontId >= 0)
        {
            float const HalfW = 1.0f / 800.0f;
            float const HalfH = 1.0f / 450.0f;

            //main buttons
            AEGfxPrint(fontId, "WALL", (gButtons[0].pos_x * HalfW) - 0.05f, (gButtons[0].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "ERASE", (gButtons[1].pos_x * HalfW) - 0.06f, (gButtons[1].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "SAVE", (gButtons[2].pos_x * HalfW) - 0.05f, (gButtons[2].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "LOAD", (gButtons[3].pos_x * HalfW) - 0.05f, (gButtons[3].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            if (gLocked)
            {
                AEGfxPrint(fontId, "LOCKED", (gButtons[4].pos_x * HalfW) - 0.085f, (gButtons[4].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            }
            else
            {
                AEGfxPrint(fontId, "EDIT", (gButtons[4].pos_x * HalfW) - 0.055f, (gButtons[4].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            }
            //levels
            AEGfxPrint(fontId, "L1", (gButtons[5].pos_x * HalfW) - 0.02f, (gButtons[5].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "L2", (gButtons[6].pos_x * HalfW) - 0.02f, (gButtons[6].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
            AEGfxPrint(fontId, "L3", (gButtons[7].pos_x * HalfW) - 0.02f, (gButtons[7].pos_y * HalfH) - 0.02f, 1.0f, 0, 0, 0, 1);
        }
    }
} // end anon namespace

//printing of the numbers when new file
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
            os << level[i][j] << ",";
        os << "\n";
    }

    os.close();
    return 0;
}

//get the binary
void readfile()
{
    std::ifstream is(GetLevelFilename());
    if (!is.is_open())
    {
        /*ClearLevelToZeros();*/
        LoadDefaultLevel();
        std::cout << "No file: " << GetLevelFilename() << " starting blank\n";
        return;
    }

    int  tile;
    char comma;

    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++)
        {
            is >> tile >> comma; // reads: number + ','
            level[row][col] = tile;
        }
    }

    is.close();
}

//generate the level map based on file name
void generateLevel(void)
{

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        float wx, wy;
        GetMouseWorld(wx, wy);

        //if click hits UI buttons, do that and DO NOT paint grid
        if (!HandleButtonClick(wx, wy))
        {
            //paint only if not locked
            if (!gLocked)
            {
                int row, col;
                WorldToGrid(wx, wy, row, col);

                if (row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS)
                    level[row][col] = gBrushValue;
            }
        }
    }

    // Draw tiles
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
    // Draw UI
    DrawEditorUI();
}

//edit out -- for player movement not needed in level editor
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