// actors.cpp - enemy implementation
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "actors.h"
#include "grid.h"

void Actors_Init(Actors* a) {
    a->data = nullptr;
    a->count = 0;
    a->capacity = 0;
}

void Actors_Free(Actors* a) {
    if (a->data) {
        free(a->data);
    }
    a->data = nullptr;
    a->count = a->capacity = 0;
}

s32 Actors_Add(Actors* a, ActorType t, s32 x, s32 y) {
    if (a->count == a->capacity) {
        s32 nc = a->capacity ? a->capacity * 2 : 8;
        void* newMem = realloc(a->data, sizeof(Actor) * static_cast<size_t>(nc));
        if (!newMem) {
            return -1;
        }
        a->data = static_cast<Actor*>(newMem);
        a->capacity = nc;
    }

    a->data[a->count].type = t;
    a->data[a->count].x = x;
    a->data[a->count].y = y;
    a->data[a->count].alive = true;

    return a->count++;
}

// Helper functions
static bool CanStep(const Grid* g, s32 x, s32 y) {
    if (!Grid_In(g, x, y)) {
        return false;
    }

    Tile t = g->tiles[Grid_Idx(g, x, y)];

    // Can step on floor, key, exit, or open gate
    if (t == TILE_FLOOR || t == TILE_KEY || t == TILE_EXIT) {
        return true;
    }

    // Can step on gate only if open
    if (t == TILE_GATE && g->gateOpen) {
        return true;
    }

    return false;
}

// Move ONE step toward player
static bool MoveOneStepTowardPlayer(const Grid* g, Actor* e, const Actor* p) {
    // If enemy is already at player position, don't move
    if (e->x == p->x && e->y == p->y) {
        return false;
    }

    s32 bestDx = 0, bestDy = 0;
    s32 bestDist = 1000000;

    // Check all 4 directions: right, left, down, up
    const s32 dx[4] = { 1, -1, 0, 0 };
    const s32 dy[4] = { 0, 0, 1, -1 };

    for (s32 dir = 0; dir < 4; dir++) {
        s32 nx = e->x + dx[dir];
        s32 ny = e->y + dy[dir];

        if (CanStep(g, nx, ny)) {
            // Manhattan distance to player
            s32 dist = (nx > p->x ? nx - p->x : p->x - nx) +
                (ny > p->y ? ny - p->y : p->y - ny);

            if (dist < bestDist) {
                bestDist = dist;
                bestDx = dx[dir];
                bestDy = dy[dir];
            }
        }
    }

    // If we found a valid move, execute it
    if (bestDist < 1000000) {
        e->x += bestDx;
        e->y += bestDy;
        return true;
    }

    return false;
}

static bool SameCell(const Actor* a, const Actor* b) {
    return a->x == b->x && a->y == b->y;
}

static void HandleKeyToggle(Grid* g, s32 x, s32 y) {
    if (Grid_In(g, x, y) && g->tiles[Grid_Idx(g, x, y)] == TILE_KEY) {
        Grid_ToggleGate(g, x, y);
        g->tiles[Grid_Idx(g, x, y)] = TILE_FLOOR;
    }
}

// Perform ONE enemy step
static void DoOneEnemyStep(Grid* g, Actors* a, PlayerState* player, bool* playerCaught, s32 i) {
    if (i == player->index) {
        return;
    }

    if (!a->data[i].alive) {
        return;
    }

    Actor* e = &a->data[i];
    Actor* p = &a->data[player->index];

    // Try to move toward player
    bool moved = MoveOneStepTowardPlayer(g, e, p);

    // Handle key collection if enemy steps on it
    HandleKeyToggle(g, e->x, e->y);

    // Check player collision
    if (SameCell(e, p)) {
        if (player->buffs.immunitySteps > 0) {
            player->buffs.immunitySteps -= 1;
        }
        else {
            *playerCaught = true;
        }
    }
}

// Enemies move only ONCE per turn
void ResolveEnemyTurns(Grid* g, Actors* a, PlayerState* player, bool* playerCaught) {
    *playerCaught = false;

    // Check if enemies are frozen
    if (player->buffs.freezeEnemyTurns > 0) {
        player->buffs.freezeEnemyTurns -= 1;
        return;
    }

    // All enemies move once
    for (s32 i = 0; i < a->count; ++i) {
        DoOneEnemyStep(g, a, player, playerCaught, i);

        if (*playerCaught) {
            return;
        }
    }
}