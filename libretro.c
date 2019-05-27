//---------------------------------------------------------------------------
// NEOPOP : Emulator as in Dreamland
//
// Copyright (c) 2001-2002 by neopop_uk
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------

#include <SDL/SDL.h>
#include <portaudio.h>
#include <stdint.h>

#include "mednafen/ngp/neopop.h"
#include "mednafen/general.h"

#include "mednafen/ngp/TLCS-900h/TLCS900h_interpret.h"
#include "mednafen/ngp/TLCS-900h/TLCS900h_registers.h"
#include "mednafen/ngp/Z80_interface.h"
#include "mednafen/ngp/interrupt.h"
#include "mednafen/ngp/mem.h"
#include "mednafen/ngp/gfx.h"
#include "mednafen/ngp/sound.h"
#include "mednafen/ngp/dma.h"
#include "mednafen/ngp/bios.h"
#include "mednafen/ngp/flash.h"
#include "mednafen/ngp/system.h"
#include "mednafen/git.h"
#include "mednafen/general.h"

SDL_Surface* real_screen;

static MDFN_Surface *surf;

static bool failed_init;

static PaStream *apu_stream;

extern uint8_t CPUExRAM[16384];

ngpgfx_t *NGPGfx;

COLOURMODE system_colour = COLOURMODE_AUTO;

uint8_t NGPJoyLatch;

static uint8_t input_buf;

static int32_t z80_runtime;

char retro_base_directory[1024];
char retro_save_directory[512];
char retro_base_name[512];

uint_fast8_t NGPFrameSkip;
int32_t ngpc_soundTS;

static void Emulate(EmulateSpecStruct *espec)
{
	bool MeowMeow        = false;

	espec->DisplayRect.x = 0;
	espec->DisplayRect.y = 0;
	espec->DisplayRect.w = 160;
	espec->DisplayRect.h = 152;

	NGPJoyLatch	= input_buf;

	storeB(0x6F82, input_buf);
   
	ngpc_soundTS         = 0;
	NGPFrameSkip         = espec->skip;

	do
	{
		int32_t timetime = (uint8_t)TLCS900h_interpret();
		MeowMeow |= updateTimers(espec->surface, timetime);
		z80_runtime += timetime;

		while(z80_runtime > 0)
		{
			int z80rantime = Z80_RunOP();

			if (z80rantime < 0) // Z80 inactive, so take up all run time!
			{
				z80_runtime = 0;
				break;
			}
			z80_runtime -= z80rantime << 1;
		}
	} while(!MeowMeow);

	espec->MasterCycles = ngpc_soundTS;
	espec->SoundBufSize = MDFNNGPCSOUND_Flush(espec->SoundBuf, espec->SoundBufMaxSize);
}

void reset(void)
{
	ngpgfx_power(NGPGfx);
	Z80_reset();
	reset_int();
	reset_timers();

	reset_memory();
	BIOSHLE_Reset();
	reset_registers();	/* TLCS900H registers */
	reset_dma();
}

static int LoadGame_NGP(const char *name)
{
	long size;
	FILE* fp;
   
	fp = fopen(name, "rb");
	if (!fp) return 0;
	
	fseek (fp, 0, SEEK_END);   // non-portable
	
	size = ftell (fp);
	ngpc_rom.length = size;
	ngpc_rom.data = (uint8_t *)malloc(size);
	
	fseek (fp, 0, SEEK_SET);
	fread (ngpc_rom.data, sizeof(uint8_t), size, fp);
	fclose (fp);

	rom_loaded();
	
	NGPGfx = (ngpgfx_t*)calloc(1, sizeof(*NGPGfx));
	NGPGfx->layer_enable = 1 | 2 | 4;

	//MDFNGameInfo->fps = (uint32)((uint64)6144000 * 65536 * 256 / 515 / 198); // 3072000 * 2 * 10000 / 515 / 198

	MDFNNGPCSOUND_Init();

	SetFRM(); // Set up fast read memory mapping

	bios_install();

	z80_runtime = 0;

	reset();

	return(1);
}

static void CloseGame(void)
{
	rom_unload();
	if (NGPGfx)
		free(NGPGfx);
	NGPGfx = NULL;
}

void NGP_CPUSaveState(uint_fast8_t load, FILE* fp)
{
	if (load == 1)
	{
		fread(&z80_runtime, sizeof(uint8_t), sizeof(z80_runtime), fp);
		fread(&CPUExRAM, sizeof(uint8_t), 16384, fp);
		fread(&FlashStatusEnable, sizeof(uint8_t), sizeof(FlashStatusEnable), fp);
		fread(&pc, sizeof(uint8_t), sizeof(pc), fp);
		fread(&sr, sizeof(uint8_t), sizeof(sr), fp);
		fread(&f_dash, sizeof(uint8_t), sizeof(f_dash), fp);
		fread(&gpr, sizeof(uint8_t), sizeof(gpr), fp);
		fread(&gprBank[0], sizeof(uint8_t), sizeof(gprBank[0]), fp);
		fread(&gprBank[1], sizeof(uint8_t), sizeof(gprBank[1]), fp);
		fread(&gprBank[2], sizeof(uint8_t), sizeof(gprBank[2]), fp);
		fread(&gprBank[3], sizeof(uint8_t), sizeof(gprBank[3]), fp);
	}
	else
	{
		fwrite(&z80_runtime, sizeof(uint8_t), sizeof(z80_runtime), fp);
		fwrite(&CPUExRAM, sizeof(uint8_t), 16384, fp);
		fwrite(&FlashStatusEnable, sizeof(uint8_t), sizeof(FlashStatusEnable), fp);
		fwrite(&pc, sizeof(uint8_t), sizeof(pc), fp);
		fwrite(&sr, sizeof(uint8_t), sizeof(sr), fp);
		fwrite(&f_dash, sizeof(uint8_t), sizeof(f_dash), fp);
		fwrite(&gpr, sizeof(uint8_t), sizeof(gpr), fp);
		fwrite(&gprBank[0], sizeof(uint8_t), sizeof(gprBank[0]), fp);
		fwrite(&gprBank[1], sizeof(uint8_t), sizeof(gprBank[1]), fp);
		fwrite(&gprBank[2], sizeof(uint8_t), sizeof(gprBank[2]), fp);
		fwrite(&gprBank[3], sizeof(uint8_t), sizeof(gprBank[3]), fp);
	}
}


extern void NGP_HLESaveState(uint_fast8_t load, FILE* fp);
extern void NGP_GFXSaveState(ngpgfx_t *gfx, uint_fast8_t load, FILE* fp);
extern void NGP_DMASaveState(uint_fast8_t load, FILE* fp);
extern void NGP_Z80SaveState(uint_fast8_t load, FILE* fp);
extern void NGP_APUSaveState(uint_fast8_t load, FILE* fp);
extern void NGP_FLASHSaveState(uint_fast8_t load, FILE* fp);
extern void NGP_INTSaveState(uint_fast8_t load, FILE* fp);

void Save_State(uint_fast8_t load)
{
	FILE* fp;
	if (load)
		fp = fopen("./game.sts", "rb");
	else
		fp = fopen("./game.sts", "wb");
		
	if (!fp) return;
	
	NGP_CPUSaveState(load, fp);
	NGP_HLESaveState(load, fp);
	NGP_GFXSaveState(NGPGfx, load, fp);
	NGP_DMASaveState(load, fp);
	NGP_Z80SaveState(load, fp);
	NGP_APUSaveState(load, fp);
	NGP_FLASHSaveState(load, fp);
	NGP_INTSaveState(load, fp);
	
	if(load)
	{
		RecacheFRM();
		changedSP();
	}
	
	fclose(fp);
}

static const MDFNSetting_EnumList LanguageList[] =
{
 { "japanese", 0, "Japanese" },
 { "0", 0 },

 { "english", 1, "English" },
 { "1", 1 },

 { NULL, 0 },
};

static MDFNSetting NGPSettings[] =
{
 { "ngp.language", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, "Language games should display text in.", NULL, MDFNST_ENUM, "english", NULL, NULL, NULL, NULL, LanguageList },
 { NULL }
};

static void SetLayerEnableMask(uint64 mask)
{
 ngpgfx_SetLayerEnableMask(NGPGfx, mask);
}

MDFNGI EmulatedNGP = {};

MDFNGI *MDFNGameInfo = &EmulatedNGP;

static void MDFNGI_reset(MDFNGI *gameinfo)
{
	gameinfo->Settings = NGPSettings;
	gameinfo->MasterClock = MDFN_MASTERCLOCK_FIXED(6144000);
	gameinfo->fps = 0;
	gameinfo->multires = false; // Multires possible?

	gameinfo->lcm_width = 160;
	gameinfo->lcm_height = 152;

	gameinfo->nominal_width = 160;
	gameinfo->nominal_height = 152;

	gameinfo->fb_width = 160;
	gameinfo->fb_height = 152;

	gameinfo->soundchan = 2;
}

static void MDFNI_CloseGame(void)
{
   if(!MDFNGameInfo)
      return;

   CloseGame();
   MDFNGI_reset(MDFNGameInfo);
}

#define FB_WIDTH 160
#define FB_HEIGHT 152
#define FB_MAX_HEIGHT FB_HEIGHT


static void Init_NGP(void)
{
	reset();
}


uint_fast8_t Load_Game(char* path)
{
   LoadGame_NGP(path);

   surf = (MDFN_Surface*)calloc(1, sizeof(*surf));
   
   if (!surf)
      return 1;
   
   surf->width  = FB_WIDTH;
   surf->height = FB_HEIGHT;
   surf->pitch  = FB_WIDTH;

   surf->pixels = (uint16_t*)calloc(1, FB_WIDTH * FB_HEIGHT * 2);

   if (!surf->pixels)
   {
      free(surf);
      return 1;
   }

   Init_NGP();
   ngpgfx_set_pixel_format(NGPGfx);
   MDFNNGPC_SetSoundRate(44100);
   
   return 0;
}

void retro_unload_game(void)
{
   MDFNI_CloseGame();
}


static uint64_t video_frames, audio_frames;

void retro_run(void)
{
	int32 SoundBufSize;
	unsigned width, height;
	static int16_t sound_buf[0x10000];
	static MDFN_Rect rects[FB_MAX_HEIGHT];
	EmulateSpecStruct spec = {0};
	bool updated = false;

	rects[0].w              = ~0;

	spec.surface            = surf;
	spec.SoundRate          = 44100;
	spec.SoundBuf           = sound_buf;
	spec.LineWidths         = rects;
	spec.SoundBufMaxSize    = sizeof(sound_buf) / 2;
	spec.SoundVolume        = 1.0;
	spec.soundmultiplier    = 1.0;
	spec.SoundBufSize       = 0;

	Emulate(&spec);

	SoundBufSize    = spec.SoundBufSize - spec.SoundBufSizeALMS;

	spec.SoundBufSize = spec.SoundBufSizeALMS + SoundBufSize;

	width  = spec.DisplayRect.w;
	height = spec.DisplayRect.h;

	memcpy(real_screen->pixels, surf->pixels, (FB_WIDTH * FB_HEIGHT)*2);
	SDL_Flip(real_screen);

	video_frames++;
	audio_frames += spec.SoundBufSize;

	Pa_WriteStream( apu_stream, spec.SoundBuf, spec.SoundBufSize);
}

static uint8_t done = 0;

void Input_Poll()
{
	SDL_Event event;
	uint8_t* keystate = SDL_GetKeyState(NULL);
	if (keystate[27]) done = 1;
		
	input_buf = 0;
	
	if (keystate[SDLK_UP])	input_buf |= 0x01;
	else if (keystate[SDLK_DOWN]) input_buf |= 0x02;
		
	if (keystate[SDLK_LEFT]) input_buf |= 0x04;
	else if (keystate[SDLK_RIGHT]) input_buf |= 0x08;
		
	if (keystate[SDLK_LCTRL]) input_buf |= 0x10;
	if (keystate[SDLK_LALT]) input_buf |= 0x20;
	if (keystate[SDLK_RETURN]) input_buf |= 0x40;
        
	while (SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym)
				{
					case SDLK_l:
						printf("Load state\n");
						Save_State(1);
					break;
					case SDLK_s:
						printf("Save state\n");
						Save_State(0);
					break;
				}
			break;
		}
	}
}

uint_fast8_t menu_state = 0;

int main(int argc, char** argv)
{
	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO );
	SDL_ShowCursor(0);
	real_screen = SDL_SetVideoMode(160, 152, 16, SDL_HWSURFACE);
	
	Pa_Initialize();
	
	PaStreamParameters outputParameters;
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice) 
	{
		printf("No sound output\n");
		return EXIT_FAILURE;
	}

	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paInt16;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	Pa_OpenStream( &apu_stream, NULL, &outputParameters, 44100, 1024, paNoFlag, NULL, NULL);
	Pa_StartStream( apu_stream );
	
	Load_Game(argv[1]);
	
	while(!done)
	{
		switch(menu_state)
		{
			case 0:
				retro_run();
				Input_Poll();
			break;
			case 1:
				// Menu();
			break;
		}

	}
	CloseGame();
}
