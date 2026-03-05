// gs_play.h - Game state function declarations for Mummy Maze
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
	void GS_PlayLoad(void);
	void GS_PlayInit(void);
	void GS_PlayUpdate(void);
	void GS_PlayDraw(void);
	void GS_PlayFree(void);
	void GS_PlayUnload(void);
#ifdef __cplusplus
}
#endif