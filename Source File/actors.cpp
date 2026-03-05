// actors.cpp - enemy implementation (C++14)
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

// ---- helpers --------------------------------------------------------------

static bool CanStep(const Grid* g, s32 x, s32 y) {
    if (!Grid_In(g, x, y)) {
        return false;
    }
    Tile t = g->tiles[Grid_Idx(g, x, y)];
    // Can step on floor, key, exit, save, or open gate
    if (t == TILE_FLOOR ||
        t == TILE_KEY ||
        t == TILE_EXIT ||
        t == TILE_SAVE) {
        return true;
    }
    if (t == TILE_GATE && g->gateOpen) {
        return true;
    }
    return false;
}

// Greedy Manhattan one-step toward player (used by White & as fallback)
static bool MoveOneStepTowardPlayer(const Grid* g, Actor* e, const Actor* p) {
    if (e->x == p->x && e->y == p->y) {
        return false;
    }
    s32 bestDx = 0, bestDy = 0;
    s32 bestDist = 1000000;
    const s32 dx[4] = { 1, -1,  0, 0 };
    const s32 dy[4] = { 0,  0,  1,-1 };
    for (s32 dir = 0; dir < 4; ++dir) {
        s32 nx = e->x + dx[dir];
        s32 ny = e->y + dy[dir];
        if (CanStep(g, nx, ny)) {
            s32 dist = (nx > p->x ? nx - p->x : p->x - nx) +
                (ny > p->y ? ny - p->y : p->y - ny);
            if (dist < bestDist) {
                bestDist = dist;
                bestDx = dx[dir];
                bestDy = dy[dir];
            }
        }
    }
    if (bestDist < 1000000) {
        e->x += bestDx;
        e->y += bestDy;
        return true;
    }
    return false;
}

// Scorpion: prefer horizontal first, then vertical fallback
static bool MoveScorpionStep(const Grid* g, Actor* e, const Actor* p) {
    if (e->x == p->x && e->y == p->y) return false;

    s32 dx = 0, dy = 0;
    // Horizontal intent
    if (p->x > e->x) dx = 1;
    else if (p->x < e->x) dx = -1;

    if (dx != 0 && CanStep(g, e->x + dx, e->y)) {
        e->x += dx; return true;
    }
    // Vertical fallback
    if (p->y > e->y) dy = 1;
    else if (p->y < e->y) dy = -1;

    if (dy != 0 && CanStep(g, e->x, e->y + dy)) {
        e->y += dy; return true;
    }
    // Side-step nudge (very simple)
    if (dx == 0) {
        if (CanStep(g, e->x + 1, e->y)) { e->x += 1; return true; }
        if (CanStep(g, e->x - 1, e->y)) { e->x -= 1; return true; }
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

// Perform ONE enemy unit-step (type-dependent)
static void DoOneEnemyStep(Grid* g, Actors* a, PlayerState* player, bool* playerCaught, s32 i) {
    if (i == player->index) return;
    if (!a->data[i].alive)  return;

    Actor* e = &a->data[i];
    Actor* p = &a->data[player->index];

    bool moved = false;
    switch (e->type) {
    case AT_MUMMY_WHITE:
        moved = MoveOneStepTowardPlayer(g, e, p);
        break;
    case AT_MUMMY_RED:
        // up to two moves per phase
        moved = MoveOneStepTowardPlayer(g, e, p);
        if (!(*playerCaught) && moved && !(e->x == p->x && e->y == p->y)) {
            MoveOneStepTowardPlayer(g, e, p);
        }
        break;
    case AT_SCORPION:
        moved = MoveScorpionStep(g, e, p);
        break;
    default:
        moved = MoveOneStepTowardPlayer(g, e, p);
        break;
    }

    // Enemy stepping on key may open gate
    HandleKeyToggle(g, e->x, e->y);

    // Check collision with player
    if (SameCell(e, p)) {
        if (player->buffs.immunitySteps > 0) {
            player->buffs.immunitySteps -= 1;
        }
        else {
            *playerCaught = true;
        }
    }
}

void ResolveEnemyTurns(Grid* g, Actors* a, PlayerState* player, bool* playerCaught) {
    *playerCaught = false;

    // Enemies frozen by sandglass
    if (player->buffs.freezeEnemyTurns > 0) {
        player->buffs.freezeEnemyTurns -= 1;
        return;
    }

    // All enemies move (type behavior handled per step)
    for (s32 i = 0; i < a->count; ++i) {
        DoOneEnemyStep(g, a, player, playerCaught, i);
        if (*playerCaught) return;
    }
}