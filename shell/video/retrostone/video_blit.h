#ifndef VIDEO_BLIT_H
#define VIDEO_BLIT_H

#include <SDL/SDL.h>
extern SDL_Surface *surf;
#define backbuffer surf

extern SDL_Surface *sdl_screen, *backbuffer;

#define HOST_WIDTH_RESOLUTION (sdl_screen->w)
#define HOST_HEIGHT_RESOLUTION (sdl_screen->h)

#define INTERNAL_NGP_WIDTH 160
#define INTERNAL_NGP_HEIGHT 152

extern uint32_t width_of_surface;
extern uint16_t* Draw_to_Virtual_Screen;

void Init_Video();
void Set_Video_Menu();
void Set_Video_InGame();
void Close_Video();
void Update_Video_Menu();
void Update_Video_Ingame(
#ifdef FRAMESKIP
uint_fast8_t skip
#endif
);

#endif
