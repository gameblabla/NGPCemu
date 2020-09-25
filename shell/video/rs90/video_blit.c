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

SDL_Surface *sdl_screen, *backbuffer, *ngp_vs;

uint32_t width_of_surface;
uint16_t* Draw_to_Virtual_Screen;

void Init_Video()
{
	SDL_Init( SDL_INIT_VIDEO );
	
	SDL_ShowCursor(0);
	
	sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
	
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0,0,0,0);
	
	ngp_vs = SDL_CreateRGBSurface(SDL_SWSURFACE, INTERNAL_NGP_WIDTH, INTERNAL_NGP_HEIGHT+1, 16, 0,0,0,0);
	
	Set_Video_InGame();
}

void Set_Video_Menu()
{
	if (sdl_screen->w != HOST_WIDTH_RESOLUTION)
	{
		memcpy(ngp_vs->pixels, sdl_screen->pixels, (INTERNAL_NGP_WIDTH * INTERNAL_NGP_HEIGHT)*2);
		sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
	}
}

void Set_Video_InGame()
{
	if (sdl_screen->w != HOST_WIDTH_RESOLUTION) sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
	Draw_to_Virtual_Screen = ngp_vs->pixels;
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
	if (ngp_vs) SDL_FreeSurface(ngp_vs);
	SDL_Quit();
}

void Update_Video_Menu()
{
	bitmap_scale(0,0,240,160,HOST_WIDTH_RESOLUTION,HOST_HEIGHT_RESOLUTION,backbuffer->w,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
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
	source_graph = (uint16_t* restrict)ngp_vs->pixels;
	
	keep_aspect_width = 192;
	keep_aspect_height = HOST_HEIGHT_RESOLUTION;
	
	SDL_LockSurface(sdl_screen);
	
	switch(option.fullscreen) 
	{
		// Fullscreen
		case 0:
			bitmap_scale(0, 0, internal_width, internal_height, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, internal_width, 0, (uint16_t* restrict)source_graph, (uint16_t* restrict)sdl_screen->pixels);
		break;
		// Keep Aspect
		case 1:
			bitmap_scale(0,0,internal_width,internal_height,keep_aspect_width,keep_aspect_height,internal_width, HOST_WIDTH_RESOLUTION - keep_aspect_width,(uint16_t* restrict)source_graph,(uint16_t* restrict)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-keep_aspect_width)/2+(HOST_HEIGHT_RESOLUTION-keep_aspect_height)/2*HOST_WIDTH_RESOLUTION);
		break;
		// Native
		case 2:
			pitch = HOST_WIDTH_RESOLUTION;
			src = (uint16_t* restrict)ngp_vs->pixels;
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
	#ifdef FRAMESKIP
	if (!skip) SDL_Flip(sdl_screen);
	#else
	SDL_Flip(sdl_screen);
	#endif
}
