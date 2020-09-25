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

SDL_Surface *sdl_screen, *backbuffer, *surf;

uint32_t width_of_surface;
uint16_t* Draw_to_Virtual_Screen;

#if !defined(USE_SDL_SURFACE)
#error "USE_SDL_SURFACE define needs to be enabled for GCW0 build !"
#endif

void Init_Video()
{
	SDL_Init( SDL_INIT_VIDEO );
	
	SDL_ShowCursor(0);
	
	sdl_screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE
	#ifdef SDL_TRIPLEBUF
	| SDL_TRIPLEBUF
	#endif
	);
	
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0,0,0,0);
	
	surf = SDL_CreateRGBSurface(SDL_SWSURFACE, INTERNAL_NGP_WIDTH, INTERNAL_NGP_HEIGHT+1, 16, 0,0,0,0);
	
	Set_Video_InGame();
}

void Set_Video_Menu()
{
	sdl_screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE);
}

void Set_Video_InGame()
{
	if (sdl_screen->w != HOST_WIDTH_RESOLUTION) sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
	Draw_to_Virtual_Screen = surf->pixels;
	width_of_surface = INTERNAL_NGP_WIDTH;
	
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
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);
	if (surf) SDL_FreeSurface(surf);
	SDL_Quit();
}

void Update_Video_Menu()
{
	SDL_BlitSurface(backbuffer, NULL, sdl_screen, NULL);
	SDL_Flip(sdl_screen);
}

void Update_Video_Ingame(
#ifdef FRAMESKIP
uint_fast8_t skip
#endif
)
{
	uint32_t y, pitch;
	uint16_t *src, *dst;
	uint32_t internal_width, internal_height, keep_aspect_width, keep_aspect_height;
	uint16_t* restrict source_graph;
	
	internal_width = INTERNAL_NGP_WIDTH;
	internal_height = INTERNAL_NGP_HEIGHT;
	source_graph = (uint16_t* restrict)surf->pixels;
	
	keep_aspect_width = ((HOST_HEIGHT_RESOLUTION / INTERNAL_NGP_HEIGHT) * INTERNAL_NGP_WIDTH) + HOST_WIDTH_RESOLUTION/4;
	if (keep_aspect_width > HOST_WIDTH_RESOLUTION) keep_aspect_width -= HOST_WIDTH_RESOLUTION/4;
	keep_aspect_height = HOST_HEIGHT_RESOLUTION;
	
	if (SDL_LockSurface(sdl_screen) == 0)
	{
		switch(option.fullscreen) 
		{
			// Fullscreen
			case 0:
				upscale_160x152_to_320x240((uint32_t* restrict)sdl_screen->pixels, (uint32_t* restrict)surf->pixels);
			break;
			// Keep Aspect
			case 1:
				bitmap_scale(0,0,internal_width,internal_height,keep_aspect_width,keep_aspect_height,internal_width, HOST_WIDTH_RESOLUTION - keep_aspect_width,(uint16_t* restrict)source_graph,(uint16_t* restrict)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-keep_aspect_width)/2+(HOST_HEIGHT_RESOLUTION-keep_aspect_height)/2*HOST_WIDTH_RESOLUTION);
			break;
			// Native
			case 2:
				pitch = HOST_WIDTH_RESOLUTION;
				src = (uint16_t* restrict)surf->pixels;
				dst = (uint16_t* restrict)sdl_screen->pixels
					+ ((HOST_WIDTH_RESOLUTION - internal_width) / 4) * sizeof(uint16_t)
					+ ((HOST_HEIGHT_RESOLUTION - internal_height) / 2) * pitch;
				for (y = 0; y < internal_height; y++)
				{
					memmove(dst, src, internal_width * sizeof(uint16_t));
					src += internal_width;
					dst += pitch;
				}
			break;
			// Hqx
			case 3:
			break;
		}
		SDL_UnlockSurface(sdl_screen);	
	}
	#ifdef FRAMESKIP
	if (!skip) SDL_Flip(sdl_screen);
	#else
	SDL_Flip(sdl_screen);
	#endif
}
