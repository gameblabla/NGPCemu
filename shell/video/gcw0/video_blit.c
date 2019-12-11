//---------------------------------------------------------------------------
// Video specific code for NGPCEMU
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include <sys/time.h>
#include <sys/types.h>
#include "mednafen.h"

#include "video_blit.h"
#include "scaler.h"
#include "config.h"

#ifndef SDL_TRIPLEBUF
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

#define FLAGS_SDL SDL_HWSURFACE | SDL_DOUBLEBUF

SDL_Surface *sdl_screen, *backbuffer, *ngp_vs;
static SDL_Joystick *sdl_joy;

uint32_t width_of_surface;
uint16_t* Draw_to_Virtual_Screen;

static const char *KEEP_ASPECT_FILENAME = "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio";

static inline uint_fast8_t get_keep_aspect_ratio()
{
	FILE *f = fopen(KEEP_ASPECT_FILENAME, "rb");
	if (!f) return false;
	char c;
	fread(&c, 1, 1, f);
	fclose(f);
	return c == 'Y';
}

static inline void set_keep_aspect_ratio(uint_fast8_t n)
{
	FILE *f = fopen(KEEP_ASPECT_FILENAME, "wb");
	if (!f) return;
	char c = n ? 'Y' : 'N';
	fwrite(&c, 1, 1, f);
	fclose(f);
}

void Init_Video()
{
	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);
	
	SDL_ShowCursor(0);
	
	sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, FLAGS_SDL);
	
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0,0,0,0);
	
	// We need to increase Height by 1 to avoid memory leaks. I hope i can fix this properly later...
	ngp_vs = SDL_CreateRGBSurface(SDL_SWSURFACE, INTERNAL_NGP_WIDTH, INTERNAL_NGP_HEIGHT+1, 16, 0,0,0,0);
	
	Set_Video_InGame();
	
	if (SDL_NumJoysticks() > 0)
	{
		sdl_joy = SDL_JoystickOpen(0);
		SDL_JoystickEventState(SDL_ENABLE);
	}
}

void Set_Video_Menu()
{
	if (sdl_screen && SDL_MUSTLOCK(sdl_screen)) SDL_UnlockSurface(sdl_screen);
	
	/* Fix mismatches when adjusting IPU ingame */
	if (get_keep_aspect_ratio() == 1 && option.fullscreen == 0) option.fullscreen = 0;
	else if (get_keep_aspect_ratio() == 0 && option.fullscreen == 1) option.fullscreen = 1;
	
	if (sdl_screen->w != HOST_WIDTH_RESOLUTION)
	{
		sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, FLAGS_SDL);
	}
	
	if (SDL_MUSTLOCK(sdl_screen)) SDL_LockSurface(sdl_screen);
}

void Set_Video_InGame()
{
	if (sdl_screen && SDL_MUSTLOCK(sdl_screen)) SDL_UnlockSurface(sdl_screen);
		
	width_of_surface = INTERNAL_NGP_WIDTH;
	Draw_to_Virtual_Screen = ngp_vs->pixels;
		
	switch(option.fullscreen) 
	{
		/* Stretched, fullscreen */
		case 0:
			set_keep_aspect_ratio(0);
			sdl_screen = SDL_SetVideoMode(INTERNAL_NGP_WIDTH, INTERNAL_NGP_HEIGHT, 16, FLAGS_SDL);
		break;
		/* Keep Aspect Ratio */
		case 1:
			set_keep_aspect_ratio(1);
			sdl_screen = SDL_SetVideoMode(INTERNAL_NGP_WIDTH, INTERNAL_NGP_HEIGHT, 16, FLAGS_SDL);
		break;
    }
    
	if (SDL_MUSTLOCK(sdl_screen)) SDL_LockSurface(sdl_screen);
}

void Close_Video()
{
	if (SDL_JoystickOpened(0)) SDL_JoystickClose(sdl_joy);
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);
	if (ngp_vs) SDL_FreeSurface(ngp_vs);
	SDL_Quit();
}

void Update_Video_Menu()
{
	if (SDL_MUSTLOCK(sdl_screen))
		SDL_UnlockSurface(sdl_screen);
	
	SDL_Flip(sdl_screen);
	
	if (SDL_MUSTLOCK(sdl_screen))
		SDL_LockSurface(sdl_screen);
}

void Update_Video_Ingame()
{
	SDL_Rect dst;
	dst.x = 0;
	dst.y = 0;
	dst.w = INTERNAL_NGP_WIDTH;
	dst.h = INTERNAL_NGP_HEIGHT;
	
	if (SDL_MUSTLOCK(sdl_screen))
		SDL_UnlockSurface(sdl_screen);
		
	SDL_BlitSurface(ngp_vs, &dst, sdl_screen, NULL);
	
	SDL_Flip(sdl_screen);
	
	if (SDL_MUSTLOCK(sdl_screen))
		SDL_LockSurface(sdl_screen);
}
