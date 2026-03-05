// actors.h - enemies, player model, and enemy resolution
#pragma once
#include "AETypes.h" // s32
#include <stdbool.h> // bool

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum { AT_PLAYER, AT_MUMMY_WHITE, AT_MUMMY_RED, AT_SCORPION } ActorType;

	typedef struct Actor {
		ActorType type;  // enemy or player
		s32 x, y;        // grid coordinates
		bool alive;      // active flag
	} Actor;

	typedef struct Actors {
		Actor* data;   // dynamic array
		s32 count;     // used size
		s32 capacity;  // capacity
	} Actors;

	typedef struct PlayerBuffs {
		s32 immunitySteps;    // decremented when enemy hits player while >0
		s32 freezeEnemyTurns; // number of full turns enemies skip
	} PlayerBuffs;

	typedef struct PlayerState {
		s32 index;        // index of the player actor in Actors
		PlayerBuffs buffs;// active buffs
		s32 health;       // simple health counter (for save/load & HEART pickup)
	} PlayerState;

	struct Grid; // forward decl for consumer grid API

	// Function declarations
	void Actors_Init(Actors* a);
	void Actors_Free(Actors* a);
	s32  Actors_Add(Actors* a, ActorType t, s32 x, s32 y);

	/**
	 * @brief Resolve a single enemy phase (each enemy may move once, or more per type).
	 * @param g            Grid
	 * @param a            All actors (player is inside too)
	 * @param player       Player state (buffs consumed here)
	 * @param playerCaught Out: set true if any enemy reaches player (unless immune)
	 */
	void ResolveEnemyTurns(struct Grid* g, Actors* a, PlayerState* player, bool* playerCaught);

#ifdef __cplusplus
}
#endif