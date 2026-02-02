// collectibles.cpp - item and power-up application
#include <cstdlib>
#include <cstdio>
#include "collectibles.h"
#include "actors.h"

void Col_Init(Collectibles* c) {
    c->data = nullptr;
    c->count = 0;
    c->capacity = 0;
}

void Col_Free(Collectibles* c) {
    if (c->data) {
        free(c->data);
    }
    c->data = nullptr;
    c->count = c->capacity = 0;
}

s32 Col_Add(Collectibles* c, CollectibleType t, s32 x, s32 y) {
    if (c->count == c->capacity) {
        s32 nc = c->capacity ? c->capacity * 2 : 8;
        void* newMem = realloc(c->data, sizeof(Collectible) * static_cast<size_t>(nc));
        if (!newMem) {
            return -1;
        }
        c->data = static_cast<Collectible*>(newMem);
        c->capacity = nc;
    }

    c->data[c->count].type = t;
    c->data[c->count].x = x;
    c->data[c->count].y = y;
    c->data[c->count].active = true;

    return c->count++;
}

s32 Col_FindAt(const Collectibles* c, s32 x, s32 y) {
    for (s32 i = 0; i < c->count; ++i) {
        if (c->data[i].active && c->data[i].x == x && c->data[i].y == y) {
            return i;
        }
    }
    return -1;
}

void Col_ApplyAndConsume(Collectibles* c, s32 index, PlayerState* player) {
    if (index < 0 || index >= c->count) {
        return;
    }

    Collectible* col = &c->data[index];
    if (!col->active) {
        return;
    }

    switch (col->type) {
    case CT_GEM:
        player->buffs.immunitySteps += 2;
        break;
    case CT_ANKH:
        player->buffs.immunitySteps += 4;
        break;
    case CT_SANDGLASS:
        player->buffs.freezeEnemyTurns += 2;
        break;
    default:
        break;
    }

    col->active = false;
}