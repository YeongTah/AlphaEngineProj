// grid_stub.cpp - grid implementation
#include "grid.h"
#include <cstdio>

bool Grid_Blocking(const Grid* g, s32 x, s32 y) {
    if (!Grid_In(g, x, y)) {
        return true;
    }

    Tile t = g->tiles[Grid_Idx(g, x, y)];

    if (t == TILE_WALL) {
        return true;
    }

    // GATE blocks when closed
    if (t == TILE_GATE && !g->gateOpen) {
        return true;
    }

    return false;
}

void Grid_ToggleGate(Grid* g, s32 triggerX, s32 triggerY) {
    g->gateOpen = !g->gateOpen;

#ifndef AE_FINAL
    printf("Gate toggled: %s at (%d, %d)\n",
        g->gateOpen ? "OPEN" : "CLOSED",
        triggerX, triggerY);
#endif
}