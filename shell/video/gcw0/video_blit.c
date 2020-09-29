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

SDL_Surface *surf;
static SDL_Joystick *sdl_joy;

uint32_t width_of_surface;

#if !defined(USE_SDL_SURFACE)
#error "USE_SDL_SURFACE define needs to be enabled for GCW0 build !"
#endif

#if !defined(IPU_SCALE)
#error "IPU_SCALE define needs to be enabled for GCW0 build !"
#endif

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
		
	surf = SDL_SetVideoMode(INTERNAL_NGP_WIDTH, INTERNAL_NGP_HEIGHT, 16, FLAGS_SDL);
    
	if (SDL_MUSTLOCK(surf)) SDL_LockSurface(surf);
	
	SDL_FillRect(surf, NULL, 0);
	SDL_Flip(surf);
	SDL_FillRect(surf, NULL, 0);
	SDL_Flip(surf);
#ifdef SDL_TRIPLEBUF
	SDL_FillRect(surf, NULL, 0);
	SDL_Flip(surf);
#endif
}

void Close_Video()
{
	if (SDL_JoystickOpened(0)) SDL_JoystickClose(sdl_joy);
	if (surf) SDL_FreeSurface(surf);
	SDL_Quit();
}

void Update_Video_Menu()
{
	SDL_Flip(surf);
}

void Update_Video_Ingame(
#ifdef FRAMESKIP
uint_fast8_t skip
#endif
)
{
	#ifdef FRAMESKIP
	if (!skip) SDL_Flip(surf);
	#else
	SDL_Flip(surf);
	#endif
}
