// collectibles.h - item data and application to player buffs
#pragma once
#include "AETypes.h" // s32
#include <stdbool.h> // bool

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum { CT_GEM, CT_ANKH, CT_SANDGLASS, CT_HEART } CollectibleType;

	typedef struct Collectible {
		CollectibleType type; // which item
		s32 x, y;             // grid position
		bool active;          // still present
	} Collectible;

	typedef struct Collectibles {
		Collectible* data; // dynamic array
		s32 count;         // used
		s32 capacity;      // capacity
	} Collectibles;

	struct PlayerState; // from actors.h

	void Col_Init(Collectibles* c);
	void Col_Free(Collectibles* c);
	s32  Col_Add(Collectibles* c, CollectibleType t, s32 x, s32 y);
	s32  Col_FindAt(const Collectibles* c, s32 x, s32 y);
	void Col_ApplyAndConsume(Collectibles* c, s32 index, struct PlayerState* player);

#ifdef __cplusplus
}
#endif