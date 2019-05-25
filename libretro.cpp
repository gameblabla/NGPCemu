#include "mednafen/mednafen.h"
#include "mednafen/mempatcher.h"
#include "mednafen/git.h"
#include "mednafen/general.h"
#include <libretro.h>
#include <streams/file_stream.h>

#include <SDL/SDL.h>
#include <portaudio.h>
#include <stdint.h>

static MDFNGI *game;

SDL_Surface* real_screen;

struct retro_perf_callback perf_cb;
retro_get_cpu_features_t perf_get_cpu_features_cb = NULL;
retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static bool overscan;

static MDFN_Surface *surf;

static bool failed_init;

static void hookup_ports(bool force);

static bool initial_ports_hookup = false;

extern "C" char retro_base_directory[1024];
std::string retro_base_name;
char retro_save_directory[1024];

static PaStream *apu_stream;

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

extern uint8 CPUExRAM[16384];

ngpgfx_t *NGPGfx;

COLOURMODE system_colour = COLOURMODE_AUTO;

uint8 NGPJoyLatch;

static uint8 *chee;

static int32 z80_runtime;

extern "C" bool NGPFrameSkip;
extern "C" int32_t ngpc_soundTS;

static void Emulate(EmulateSpecStruct *espec)
{
   bool MeowMeow        = false;

   espec->DisplayRect.x = 0;
   espec->DisplayRect.y = 0;
   espec->DisplayRect.w = 160;
   espec->DisplayRect.h = 152;

   NGPJoyLatch          = *chee;

   storeB(0x6F82, *chee);

   MDFNMP_ApplyPeriodicCheats();

   ngpc_soundTS         = 0;
   NGPFrameSkip         = espec->skip;

   do
   {
      int32 timetime = (uint8)TLCS900h_interpret();
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
   }while(!MeowMeow);


   espec->MasterCycles = ngpc_soundTS;
   espec->SoundBufSize = MDFNNGPCSOUND_Flush(espec->SoundBuf,
         espec->SoundBufMaxSize);
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
	printf("size %d\n", size);
	ngpc_rom.length = size;
	ngpc_rom.data = (uint8_t *)malloc(size);
	
	fseek (fp, 0, SEEK_SET);
	fread (ngpc_rom.data, sizeof(uint8_t), size, fp);
	fclose (fp);

	rom_loaded();

	MDFNMP_Init(1024, 1024 * 1024 * 16 / 1024);

	NGPGfx = (ngpgfx_t*)calloc(1, sizeof(*NGPGfx));
	NGPGfx->layer_enable = 1 | 2 | 4;

	MDFNGameInfo->fps = (uint32)((uint64)6144000 * 65536 * 256 / 515 / 198); // 3072000 * 2 * 10000 / 515 / 198

	MDFNNGPCSOUND_Init();

	MDFNMP_AddRAM(16384, 0x4000, CPUExRAM);

	SetFRM(); // Set up fast read memory mapping

	bios_install();

	z80_runtime = 0;

	reset();

	return(1);
}


static int Load(const char *name, MDFNFILE *fp, const uint8_t *data, size_t size)
{
   if ((data != NULL) && (size != 0)) 
   {
      if (!(ngpc_rom.data = (uint8 *)malloc(size)))
         return(0);
      ngpc_rom.length = size;
      memcpy(ngpc_rom.data, data, size);
   }
   else
   {
      if(!(ngpc_rom.data = (uint8 *)malloc(fp->size)))
         return(0);

      ngpc_rom.length = fp->size;
      memcpy(ngpc_rom.data, fp->data, fp->size);
   }

   rom_loaded();

   MDFNMP_Init(1024, 1024 * 1024 * 16 / 1024);

   NGPGfx = (ngpgfx_t*)calloc(1, sizeof(*NGPGfx));
   NGPGfx->layer_enable = 1 | 2 | 4;

   MDFNGameInfo->fps = (uint32)((uint64)6144000 * 65536 * 256 / 515 / 198); // 3072000 * 2 * 10000 / 515 / 198

   MDFNNGPCSOUND_Init();

   MDFNMP_AddRAM(16384, 0x4000, CPUExRAM);

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

static void SetInput(int port, const char *type, void *ptr)
{
   if(!port)
      chee = (uint8 *)ptr;
}

int StateAction(StateMem *sm, int load, int data_only)
{
   SFORMAT StateRegs[] =
   {
      SFVAR(z80_runtime),
      SFARRAY(CPUExRAM, 16384),
      SFVAR(FlashStatusEnable),
      SFEND
   };

   SFORMAT TLCS_StateRegs[] =
   {
      SFVARN(pc, "PC"),
      SFVARN(sr, "SR"),
      SFVARN(f_dash, "F_DASH"),
      SFARRAY32N(gpr, 4, "GPR"),
      SFARRAY32N(gprBank[0], 4, "GPRB0"),
      SFARRAY32N(gprBank[1], 4, "GPRB1"),
      SFARRAY32N(gprBank[2], 4, "GPRB2"),
      SFARRAY32N(gprBank[3], 4, "GPRB3"),
      SFEND
   };

   if(!MDFNSS_StateAction(sm, load, data_only, StateRegs, "MAIN", false))
      return(0);

   if(!MDFNSS_StateAction(sm, load, data_only, TLCS_StateRegs, "TLCS", false))
      return(0);

   if(!MDFNNGPCDMA_StateAction(sm, load, data_only))
      return(0);

   if(!MDFNNGPCSOUND_StateAction(sm, load, data_only))
      return(0);

   if(!ngpgfx_StateAction(NGPGfx, sm, load, data_only))
      return(0);

   if(!MDFNNGPCZ80_StateAction(sm, load, data_only))
      return(0);

   if(!int_timer_StateAction(sm, load, data_only))
      return(0);

   if(!BIOSHLE_StateAction(sm, load, data_only))
      return(0);

   if(!FLASH_StateAction(sm, load, data_only))
      return(0);

   if(load)
   {
      RecacheFRM();
      changedSP();
   }
   return(1);
}

static void DoSimpleCommand(int cmd)
{
   switch(cmd)
   {
      case MDFN_MSC_POWER:
      case MDFN_MSC_RESET:
         reset();
         break;
   }
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

static const InputDeviceInputInfoStruct IDII[] =
{
 { "up", "UP ↑", 0, IDIT_BUTTON, "down" },
 { "down", "DOWN ↓", 1, IDIT_BUTTON, "up" },
 { "left", "LEFT ←", 2, IDIT_BUTTON, "right" },
 { "right", "RIGHT →", 3, IDIT_BUTTON, "left" },
 { "a", "A", 5, IDIT_BUTTON_CAN_RAPID,  NULL },
 { "b", "B", 6, IDIT_BUTTON_CAN_RAPID, NULL },
 { "option", "OPTION", 4, IDIT_BUTTON, NULL },
};
static InputDeviceInfoStruct InputDeviceInfo[] =
{
 {
  "gamepad",
  "Gamepad",
  NULL,
  NULL,
  sizeof(IDII) / sizeof(InputDeviceInputInfoStruct),
  IDII,
 }
};

static const InputPortInfoStruct PortInfo[] =
{
 { "builtin", "Built-In", sizeof(InputDeviceInfo) / sizeof(InputDeviceInfoStruct), InputDeviceInfo, "gamepad" }
};

static InputInfoStruct InputInfo =
{
	sizeof(PortInfo) / sizeof(InputPortInfoStruct),
	PortInfo
};

static const FileExtensionSpecStruct KnownExtensions[] =
{
	{ ".ngp", "Neo Geo Pocket ROM Image" },
	{ ".ngc", "Neo Geo Pocket Color ROM Image" },
	{ NULL, NULL }
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
	gameinfo->dummy_separator = NULL;

	gameinfo->nominal_width = 160;
	gameinfo->nominal_height = 152;

	gameinfo->fb_width = 160;
	gameinfo->fb_height = 152;

	gameinfo->soundchan = 2;
}

static MDFNGI *MDFNI_LoadGame(const char *name)
{
   MDFNFILE *GameFile = file_open(name);

   if(!GameFile)
      goto error;

   if(Load(name, GameFile, NULL, 0) <= 0)
      goto error;

   file_close(GameFile);
   GameFile     = NULL;

   return MDFNGameInfo;

error:
   if (GameFile)
      file_close(GameFile);
   GameFile     = NULL;
   MDFNGI_reset(MDFNGameInfo);
   return(0);
}

static MDFNGI *MDFNI_LoadGameData(const uint8_t *data, size_t size)
{
   if(Load("", NULL, data, size) <= 0)
      goto error;
   return MDFNGameInfo;

error:
   MDFNGI_reset(MDFNGameInfo);
   return(0);
}

static void MDFNI_CloseGame(void)
{
   if(!MDFNGameInfo)
      return;

   CloseGame();
   MDFNGI_reset(MDFNGameInfo);
}

static void set_basename(const char *path)
{
   const char *base = strrchr(path, '/');
   if (!base)
      base = strrchr(path, '\\');

   if (base)
      retro_base_name = base + 1;
   else
      retro_base_name = path;

   retro_base_name = retro_base_name.substr(0, retro_base_name.find_last_of('.'));
}

#define MEDNAFEN_CORE_NAME_MODULE "ngp"
#define MEDNAFEN_CORE_NAME "Beetle NeoPop"
#define MEDNAFEN_CORE_VERSION "v0.9.36.1"
#define MEDNAFEN_CORE_EXTENSIONS "ngp|ngc|ngpc|npc"
#define MEDNAFEN_CORE_TIMING_FPS 60.25
#define MEDNAFEN_CORE_GEOMETRY_BASE_W 160 
#define MEDNAFEN_CORE_GEOMETRY_BASE_H 152
#define MEDNAFEN_CORE_GEOMETRY_MAX_W 160
#define MEDNAFEN_CORE_GEOMETRY_MAX_H 152
#define MEDNAFEN_CORE_GEOMETRY_ASPECT_RATIO (20.0 / 19.0)
#define FB_WIDTH 160
#define FB_HEIGHT 152



#define FB_MAX_HEIGHT FB_HEIGHT

const char *mednafen_core_str = MEDNAFEN_CORE_NAME;

void retro_init(void)
{
   MDFNGI_reset(MDFNGameInfo);
}

void retro_reset(void)
{
   DoSimpleCommand(MDFN_MSC_RESET);
}

bool retro_load_game_special(unsigned, const struct retro_game_info *, size_t)
{
   return false;
}

static void set_volume (uint32_t *ptr, unsigned number)
{
   switch(number)
   {
      case 0:
      default:
         *ptr = number;
         break;
   }
}


static void check_variables(void)
{
	setting_ngp_language = 1;    
	retro_reset();
}

#define MAX_PLAYERS 1
#define MAX_BUTTONS 7
static uint8_t input_buf;


static void hookup_ports(bool force)
{
   if (initial_ports_hookup && !force)
      return;

   SetInput(0, "gamepad", &input_buf);

   initial_ports_hookup = true;
}

bool retro_load_game(char* path)
{
   overscan = false;

   set_basename(path);
   LoadGame_NGP(path);

   MDFN_LoadGameCheats(NULL);
   MDFNMP_InstallReadPatches();

   surf = (MDFN_Surface*)calloc(1, sizeof(*surf));
   
   if (!surf)
      return false;
   
   surf->width  = FB_WIDTH;
   surf->height = FB_HEIGHT;
   surf->pitch  = FB_WIDTH;

   surf->pixels = (uint16_t*)calloc(1, FB_WIDTH * FB_HEIGHT * 2);

   if (!surf->pixels)
   {
      free(surf);
      return false;
   }

   hookup_ports(true);

   check_variables();
   ngpgfx_set_pixel_format(NGPGfx);
   MDFNNGPC_SetSoundRate(44100);
   
   return true;
}

void retro_unload_game(void)
{
   if (!game)
      return;

   MDFN_FlushGameCheats(0);
   MDFNI_CloseGame();
   MDFNMP_Kill();
}

static void update_input(void)
{
   input_buf = 0;

   static unsigned map[] = {
      RETRO_DEVICE_ID_JOYPAD_UP, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_DOWN, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_LEFT, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_RIGHT, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_B,
      RETRO_DEVICE_ID_JOYPAD_A,
      RETRO_DEVICE_ID_JOYPAD_START,
   };

   for (unsigned i = 0; i < MAX_BUTTONS; i++)
      input_buf |= map[i] != -1u &&
         input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, map[i]) ? (1 << i) : 0;
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

   input_poll_cb();

   update_input();

   rects[0].w              = ~0;

   spec.surface            = surf;
   spec.SoundRate          = 44100;
   spec.SoundBuf           = sound_buf;
   spec.LineWidths         = rects;
   spec.SoundBufMaxSize    = sizeof(sound_buf) / 2;
   spec.SoundVolume        = 1.0;
   spec.soundmultiplier    = 1.0;
   spec.SoundBufSize       = 0;
   spec.VideoFormatChanged = false;
   spec.SoundFormatChanged = false;

   Emulate(&spec);

   SoundBufSize    = spec.SoundBufSize - spec.SoundBufSizeALMS;

   spec.SoundBufSize = spec.SoundBufSizeALMS + SoundBufSize;

   width  = spec.DisplayRect.w;
   height = spec.DisplayRect.h;

   video_cb(surf->pixels, width, height, FB_WIDTH << 1);

   video_frames++;
   audio_frames += spec.SoundBufSize;

   audio_batch_cb(spec.SoundBuf, spec.SoundBufSize);

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
}

void retro_set_controller_port_device(unsigned in_port, unsigned device)
{
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;
   environ_cb = cb;

   static const struct retro_variable vars[] = {
      { "ngp_language", "Language (restart); english|japanese" },
      { NULL, NULL },
   };
   cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);

   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
	   filestream_vfs_init(&vfs_iface_info);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

static size_t serialize_size;

size_t retro_serialize_size(void)
{
   StateMem st;

   st.data           = NULL;
   st.loc            = 0;
   st.len            = 0;
   st.malloced       = 0;
   st.initial_malloc = 0;

   if (!MDFNSS_SaveSM(&st, 0, 0, NULL, NULL, NULL))
      return 0;

   free(st.data);

   return serialize_size = st.len;
}

bool retro_serialize(void *data, size_t size)
{
   StateMem st;
   bool ret          = false;
   uint8_t *_dat     = (uint8_t*)malloc(size);

   if (!_dat)
      return false;

   /* Mednafen can realloc the buffer so we need to ensure this is safe. */
   st.data           = _dat;
   st.loc            = 0;
   st.len            = 0;
   st.malloced       = size;
   st.initial_malloc = 0;

   ret = MDFNSS_SaveSM(&st, 0, 0, NULL, NULL, NULL);

   memcpy(data, st.data, size);
   free(st.data);

   return ret;
}

bool retro_unserialize(const void *data, size_t size)
{
   StateMem st;

   st.data           = (uint8_t*)data;
   st.loc            = 0;
   st.len            = size;
   st.malloced       = 0;
   st.initial_malloc = 0;

   return MDFNSS_LoadSM(&st, 0, 0);
}

void *retro_get_memory_data(unsigned type)
{
   if(type == RETRO_MEMORY_SYSTEM_RAM)
      return CPUExRAM;
   else return NULL;
}

size_t retro_get_memory_size(unsigned type)
{
   if(type == RETRO_MEMORY_SYSTEM_RAM)
      return 16384;
   else return 0;
}



// Use a simpler approach to make sure that things go right for libretro.
std::string MDFN_MakeFName(MakeFName_Type type, int id1, const char *cd1)
{
   char slash;
   slash = '/';
   std::string ret;
   switch (type)
   {
      case MDFNMKF_SAV:
         ret = std::string(retro_save_directory) + slash + std::string(retro_base_name) +
            std::string(".") + std::string(cd1);
         break;
      case MDFNMKF_FIRMWARE:
         ret = std::string(retro_base_directory) + slash + std::string(cd1);
         break;
      default:	  
         break;
   }

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "MDFN_MakeFName: %s\n", ret.c_str());
   return ret;
}

/***
 * Callback for updating the libretro environment.
 */
bool retro_environment_callback(unsigned cmd, void *data)
{
    // TODO: do something with this data...
    // Also, retro_init() doesn't work if I just
    // put a return here, hence the stupid printf.
    return 0;
}

int16_t retro_input_state_callback(unsigned port, unsigned device, unsigned index, unsigned id)
{
	return 0;
}

void retro_input_poll_callback()
{

}

size_t retro_audio_sample_batch_callback(const int16_t *data, size_t frames)
{
	PaError err = Pa_WriteStream( apu_stream, data, frames);
	return frames;
}


void retro_video_refresh_callback(const void *data, unsigned width, unsigned height, size_t pitch)
{
	memcpy(real_screen->pixels, data, (width*height)*2);
	SDL_Flip(real_screen);
	
	return;
}


void setup_callbacks()
{
    retro_set_environment(&retro_environment_callback);
    retro_set_video_refresh(&retro_video_refresh_callback);
    retro_set_input_poll(&retro_input_poll_callback);
    retro_set_input_state(&retro_input_state_callback);
    retro_set_audio_sample_batch(&retro_audio_sample_batch_callback);
}



int main(int argc, char** argv)
{
	uint8_t done = 0;
	uint8_t* keystate;
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
	
	setup_callbacks();
	retro_load_game(argv[1]);
	retro_init();
	
	while(!done)
	{
		keystate = SDL_GetKeyState(NULL);
		if (keystate[27]) done = 1;
		retro_run();
		
        SDL_Event event;
        SDL_PollEvent(&event);
	}
}
