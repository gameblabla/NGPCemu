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

#define FLAGS_SDL SDL_HWSURFACE | SDL_TRIPLEBUF

SDL_Surface *surf, *backbuffer;
static SDL_Joystick *sdl_joy;

uint32_t width_of_surface;

static const char *KEEP_ASPECT_FILENAME = "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio";

#if !defined(USE_SDL_SURFACE)
#error "USE_SDL_SURFACE define needs to be enabled for GCW0 build !"
#endif

static inline uint_fast8_t get_keep_aspect_ratio()
{
#ifdef RS97
	return 0;
#else
	FILE *f = fopen(KEEP_ASPECT_FILENAME, "rb");
	if (!f) return 0;
	char c;
	fread(&c, 1, 1, f);
	fclose(f);
	return c == 'Y';
#endif
}

static inline void set_keep_aspect_ratio(uint32_t n)
{
#ifndef RS97
	FILE *f = fopen(KEEP_ASPECT_FILENAME, "wb");
	if (!f) return;
	char c = n ? 'Y' : 'N';
	fwrite(&c, 1, 1, f);
	fclose(f);
#endif
}

void Init_Video()
{
	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);
	
	SDL_ShowCursor(0);
	
	surf = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, FLAGS_SDL);

	Set_Video_InGame();
	
	if (SDL_NumJoysticks() > 0)
	{
		sdl_joy = SDL_JoystickOpen(0);
		SDL_JoystickEventState(SDL_ENABLE);
	}
}

void Set_Video_Menu()
{
	if (surf && SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);
	
	/* Fix mismatches when adjusting IPU ingame */
	if (get_keep_aspect_ratio() == 1 && option.fullscreen == 0) option.fullscreen = 0;
	else if (get_keep_aspect_ratio() == 0 && option.fullscreen == 1) option.fullscreen = 1;
	
	if (surf->w != HOST_WIDTH_RESOLUTION)
	{
		surf = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, FLAGS_SDL);
	}
	
	if (SDL_MUSTLOCK(surf)) SDL_LockSurface(surf);
}

void Set_Video_InGame()
{
	if (surf && SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);
		
	width_of_surface = INTERNAL_NGP_WIDTH;
		
	switch(option.fullscreen) 
	{
		/* Stretched, fullscreen */
		case 0:
			set_keep_aspect_ratio(0);
			surf = SDL_SetVideoMode(INTERNAL_NGP_WIDTH, INTERNAL_NGP_HEIGHT, 16, FLAGS_SDL);
		break;
		/* Keep Aspect Ratio */
		case 1:
			set_keep_aspect_ratio(1);
			surf = SDL_SetVideoMode(INTERNAL_NGP_WIDTH, INTERNAL_NGP_HEIGHT, 16, FLAGS_SDL);
		break;
    }
    
	if (SDL_MUSTLOCK(surf)) SDL_LockSurface(surf);
	
	SDL_FillRect(sdl_screen, NULL, 0);
	SDL_Flip(sdl_screen);
	SDL_FillRect(sdl_screen, NULL, 0);
	SDL_Flip(sdl_screen);
#ifdef SDL_TRIPLEBUF
	SDL_FillRect(sdl_screen, NULL, 0);
	SDL_Flip(sdl_screen);
#endif
}

void Close_Video()
{
	if (SDL_JoystickOpened(0)) SDL_JoystickClose(sdl_joy);
	if (surf) SDL_FreeSurface(surf);
	if (backbuffer) SDL_FreeSurface(backbuffer);
	SDL_Quit();
}

void Update_Video_Menu()
{
	bitmap_scale(0,0,320,240,HOST_WIDTH_RESOLUTION,HOST_HEIGHT_RESOLUTION,backbuffer->w,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
	SDL_Flip(surf);
}

void Update_Video_Ingame(
#ifdef FRAMESKIP
uint_fast8_t skip
#endif
)
{
	#ifdef FRAMESKIP
	if (!skip) SDL_Flip(sdl_screen);
	#else
	SDL_Flip(sdl_screen);
	#endif
}
