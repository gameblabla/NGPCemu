#include "neopop.h"
#include "sound.h"

#include "../include/blip/Blip_Buffer.h"
#include "../include/blip/Stereo_Buffer.h"
#include "T6W28_Apu.h"
#include "shared.h"

static T6W28_Apu apu;

static Stereo_Buffer buf;

static uint8_t LastDACLeft = 0, LastDACRight = 0;
static uint8_t CurrentDACLeft = 0, CurrentDACRight = 0;

typedef Blip_Synth<blip_good_quality, 0xFF> Synth;
static Synth synth;
static bool schipenable = 0;

#ifdef __cplusplus
extern "C" {
#endif

void MDFNNGPCSOUND_SetEnable(bool set)
{
   schipenable = set;
   if(!set)
      apu.reset();
}

void Write_SoundChipLeft(uint8_t data)
{
   if(schipenable)
      apu.write_data_left(ngpc_soundTS >> 1, data);
}

void Write_SoundChipRight(uint8_t data)
{
   if(schipenable)
      apu.write_data_right(ngpc_soundTS >> 1, data);
}

void dac_write_left(uint8_t data)
{
   CurrentDACLeft = data;

   synth.offset_inline(ngpc_soundTS >> 1, CurrentDACLeft - LastDACLeft, buf.left());

   LastDACLeft = data;
}

void dac_write_right(uint8_t data)
{
   CurrentDACRight = data;

   synth.offset_inline(ngpc_soundTS >> 1, CurrentDACRight - LastDACRight, buf.right());

   LastDACRight = data;
}

int32_t MDFNNGPCSOUND_Flush(int16_t *SoundBuf, const int32_t MaxSoundFrames)
{
   int32_t FrameCount = 0;

   apu.end_frame(ngpc_soundTS >> 1);

   buf.end_frame(ngpc_soundTS >> 1);

   if(SoundBuf)
      FrameCount = buf.read_samples(SoundBuf, MaxSoundFrames * 2) / 2;
   else
      buf.clear();

   return(FrameCount);
}

static void RedoVolume(void)
{
   apu.output(buf.center(), buf.left(), buf.right());
   apu.volume(0.30);
   synth.volume(0.40);
}

void MDFNNGPCSOUND_Init(void)
{
   MDFNNGPC_SetSoundRate(0);
   buf.clock_rate((long)(3072000));

   RedoVolume();
   buf.bass_freq(20);
}

bool MDFNNGPC_SetSoundRate(uint32_t rate)
{
   buf.set_sample_rate(rate ? rate : SOUND_OUTPUT_FREQUENCY, 60);
   return(true);
}

void NGP_APUSaveState(uint_fast8_t load, FILE* fp)
{
	T6W28_ApuState *sn_state;
	
	/* Load state */
	if (load == 1)
	{
		sn_state = (T6W28_ApuState *)malloc(sizeof(T6W28_ApuState));
		
		fread(&CurrentDACLeft, sizeof(uint8_t), sizeof(CurrentDACLeft), fp);
		fread(&CurrentDACRight, sizeof(uint8_t), sizeof(CurrentDACRight), fp);
		fread(&schipenable, sizeof(uint8_t), sizeof(schipenable), fp);
		fread(&sn_state->delay, sizeof(uint8_t), sizeof(sn_state->delay), fp);
		fread(&sn_state->volume_left, sizeof(uint8_t), sizeof(sn_state->volume_left), fp);
		fread(&sn_state->volume_right, sizeof(uint8_t), sizeof(sn_state->volume_right), fp);
		fread(&sn_state->sq_period, sizeof(uint8_t), sizeof(sn_state->sq_period), fp);
		fread(&sn_state->sq_phase, sizeof(uint8_t), sizeof(sn_state->sq_phase), fp);

		fread(&sn_state->noise_period, sizeof(uint8_t), sizeof(sn_state->noise_period), fp);
		fread(&sn_state->noise_shifter, sizeof(uint8_t), sizeof(sn_state->noise_shifter), fp);
		fread(&sn_state->noise_tap, sizeof(uint8_t), sizeof(sn_state->noise_tap), fp);
		fread(&sn_state->noise_period_extra, sizeof(uint8_t), sizeof(sn_state->noise_period_extra), fp);
		fread(&sn_state->latch_left, sizeof(uint8_t), sizeof(sn_state->latch_left), fp);
		fread(&sn_state->latch_right, sizeof(uint8_t), sizeof(sn_state->latch_right), fp);
		
		apu.load_state(sn_state);
		synth.offset(ngpc_soundTS >> 1, CurrentDACLeft - LastDACLeft, buf.left());
		synth.offset(ngpc_soundTS >> 1, CurrentDACRight - LastDACRight, buf.right());
		LastDACLeft = CurrentDACLeft;
		LastDACRight = CurrentDACRight;
	}
	/* Save State */
	else
	{
		sn_state = apu.save_state();
		
		fwrite(&CurrentDACLeft, sizeof(uint8_t), sizeof(CurrentDACLeft), fp);
		fwrite(&CurrentDACRight, sizeof(uint8_t), sizeof(CurrentDACRight), fp);
		fwrite(&schipenable, sizeof(uint8_t), sizeof(schipenable), fp);
		fwrite(&sn_state->delay, sizeof(uint8_t), sizeof(sn_state->delay), fp);
		fwrite(&sn_state->volume_left, sizeof(uint8_t), sizeof(sn_state->volume_left), fp);
		fwrite(&sn_state->volume_right, sizeof(uint8_t), sizeof(sn_state->volume_right), fp);
		fwrite(&sn_state->sq_period, sizeof(uint8_t), sizeof(sn_state->sq_period), fp);
		fwrite(&sn_state->sq_phase, sizeof(uint8_t), sizeof(sn_state->sq_phase), fp);

		fwrite(&sn_state->noise_period, sizeof(uint8_t), sizeof(sn_state->noise_period), fp);
		fwrite(&sn_state->noise_shifter, sizeof(uint8_t), sizeof(sn_state->noise_shifter), fp);
		fwrite(&sn_state->noise_tap, sizeof(uint8_t), sizeof(sn_state->noise_tap), fp);
		fwrite(&sn_state->noise_period_extra, sizeof(uint8_t), sizeof(sn_state->noise_period_extra), fp);
		fwrite(&sn_state->latch_left, sizeof(uint8_t), sizeof(sn_state->latch_left), fp);
		fwrite(&sn_state->latch_right, sizeof(uint8_t), sizeof(sn_state->latch_right), fp);
	}
	
	free(sn_state);
}

#ifdef __cplusplus
}
#endif
