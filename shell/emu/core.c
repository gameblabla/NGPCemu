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

#include <libgen.h>
#include <stdint.h>
#include <sys/time.h>

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

#include "video_blit.h"
#include "sound_output.h"
#include "input.h"
#include "menu.h"

static MDFN_Surface *surf;

extern uint8_t CPUExRAM[16384];

ngpgfx_t *NGPGfx;

COLOURMODE system_colour = COLOURMODE_AUTO;

uint8_t NGPJoyLatch;
uint8_t input_buf;

static int32_t z80_runtime;

char GameName_emu[512];

uint_fast8_t NGPFrameSkip;
uint_fast8_t emulator_state = 0;
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

void SaveState(const char* path, uint_fast8_t load)
{
	FILE* fp;
	if (load)
		fp = fopen(path, "rb");
	else
		fp = fopen(path, "wb");
		
	if (!fp) return;
	
	NGP_CPUSaveState(load, fp);
	NGP_HLESaveState(load, fp);
	NGP_GFXSaveState(NGPGfx, load, fp);
	NGP_DMASaveState(load, fp);
	NGP_Z80SaveState(load, fp);
	NGP_APUSaveState(load, fp);
	NGP_INTSaveState(load, fp);
	/* Do the flash memory last as it can increase in size over the time. */
	NGP_FLASHSaveState(load, fp);
	
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

   surf->pixels = Draw_to_Virtual_Screen;

   Init_NGP();
   ngpgfx_set_pixel_format(NGPGfx);
   MDFNNGPC_SetSoundRate(SOUND_OUTPUT_FREQUENCY);
   
   return 0;
}

static uint64_t video_frames, audio_frames;

#ifdef FRAMESKIP
static uint32_t Timer_Read(void) 
{
	/* Timing. */
	struct timeval tval;
  	gettimeofday(&tval, 0);
	return (((tval.tv_sec*1000000) + (tval.tv_usec)));
}
static long lastTick = 0, newTick;
static uint32_t SkipCnt = 0, FPS = 60, FrameSkip = 0;
static const uint32_t TblSkip[5][5] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1},
    {0, 0, 0, 1, 1},
    {0, 0, 1, 1, 1},
    {0, 1, 1, 1, 1},
};
#endif


void Run_Emu(void)
{
	int32_t SoundBufSize;
	static int16_t sound_buf[0x10000];
	static MDFN_Rect rects[FB_MAX_HEIGHT];
	EmulateSpecStruct spec = {0};

	rects[0].w              = ~0;
	
#ifdef FRAMESKIP
	SkipCnt++;
	if (SkipCnt > 4) SkipCnt = 0;
	spec.skip = TblSkip[FrameSkip][SkipCnt];
#else
	spec.skip = 0;
#endif

	spec.surface            = surf;
	spec.SoundRate          = SOUND_OUTPUT_FREQUENCY;
	spec.SoundBuf           = sound_buf;
	spec.LineWidths         = rects;
	spec.SoundBufMaxSize    = sizeof(sound_buf) / 2;
	spec.SoundVolume        = 1.0;
	spec.soundmultiplier    = 1.0;
	spec.SoundBufSize       = 0;

	Emulate(&spec);

	SoundBufSize    = spec.SoundBufSize - spec.SoundBufSizeALMS;

	spec.SoundBufSize = spec.SoundBufSizeALMS + SoundBufSize;

	Update_Video_Ingame();

	video_frames++;
	
#ifdef FRAMESKIP
	newTick = Timer_Read();
	if ( (newTick) - (lastTick) > 1000000) 
	{
		FPS = video_frames;
		video_frames = 0;
		lastTick = newTick;
		if (FPS >= 60)
		{
			FrameSkip = 0;
		}
		else
		{
			FrameSkip = 60 / FPS;
			if (FrameSkip > 4) FrameSkip = 4;
		}
	}
#endif
	
	audio_frames += spec.SoundBufSize;

	Audio_Write((int16_t* restrict) spec.SoundBuf, spec.SoundBufSize);
}

uint_fast8_t done = 0;

void Input_Poll()
{
	input_buf = update_input();
}

int main(int argc, char* argv[])
{
    printf("Starting NGPCEmu\n");
    
    if (argc < 2)
	{
		printf("Specify a ROM to load in memory\n");
		return 0;
	}
	snprintf(GameName_emu, sizeof(GameName_emu), "%s", basename(argv[1]));
	
	Init_Video();
	
	Audio_Init();
	
	Init_Configuration();
	
	Load_Game(argv[1]);
	
	while(!done)
	{
		switch(emulator_state)
		{
			case 0:
				Run_Emu();
				Input_Poll();
			break;
			case 1:
				Menu();
			break;
		}

	}
	Close_Video();
	Audio_Close();
	MDFNI_CloseGame();
}
