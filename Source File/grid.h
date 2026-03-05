// grid.h - consumer-facing grid API that your logic calls
#pragma once
#include "AETypes.h" // s32
#include <stdbool.h> // bool

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum { TILE_FLOOR, TILE_WALL, TILE_EXIT, TILE_KEY, TILE_GATE, TILE_TRAP, TILE_SAVE } Tile;

	typedef struct Grid {
		s32 width, height; // grid size
		Tile* tiles;       // width*height tiles
		bool gateOpen;     // simple single-gate example
	} Grid;

	// Inline helpers
	static inline s32 Grid_Idx(const Grid* g, s32 x, s32 y) {
		return y * g->width + x;
	}
	static inline bool Grid_In(const Grid* g, s32 x, s32 y) {
		return x >= 0 && y >= 0 && x < g->width && y < g->height;
	}

	// Function declarations
	bool Grid_Blocking(const Grid* g, s32 x, s32 y);
	void Grid_ToggleGate(Grid* g, s32 triggerX, s32 triggerY);

#ifdef __cplusplus
}
#endif