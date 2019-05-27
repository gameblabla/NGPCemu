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

#include <string.h>

#include "../mednafen.h"

#include "TLCS-900h/TLCS900h_registers.h"
#include "TLCS-900h/TLCS900h_interpret.h"
#include "mem.h"
#include "gfx.h"
#include "interrupt.h"
#include "Z80_interface.h"
#include "dma.h"
#include "system.h"
#include "neopop.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t timer_hint;
static uint32_t timer_clock[4];
static uint8_t timer[4];	//Up-counters
static uint8_t timer_threshold[4];

static uint8_t TRUN;
static uint8_t T01MOD, T23MOD;
static uint8_t TRDC;
static uint8_t TFFCR;
static uint8_t HDMAStartVector[4];

static int32_t ipending[24];
static int32_t IntPrio[0xB]; // 0070-007a
static bool h_int, timer0, timer2;

// The way interrupt processing is set up is still written towards BIOS HLE emulation, which assumes
// that the interrupt handler will immediately call DI, clear the interrupt latch(so the interrupt won't happen again when interrupts are re-enabled),
// and then call the game's interrupt handler.

// We automatically clear the interrupt latch when the interrupt is accepted, and the interrupt latch is not checked every instruction,
// but only when: an EI/DI, POP SR, or RETI instruction occurs; after a write to an interrupt priority register occurs; and when
// a device sets the virual interrupt latch register, signaling it wants an interrupt.

// FIXME in the future if we ever add real bios support?

void interrupt(uint8_t index, uint8_t level)
{
   push32(pc);
   push16(sr);

   //Up the IFF
   if (level < 7) level++;
   setStatusIFF(level);

   //Access the interrupt vector table to find the jump destination
   pc = loadL(0x6FB8 + index * 4);
}

void set_interrupt(uint8_t index, bool set)
{
#ifndef NDEBUG
   assert(index < 24);
#endif

   ipending[index] = set;
   int_check_pending();
}

void int_check_pending(void)
{
   uint8_t curIFF = statusIFF();

   /* Technically, the BIOS should clear the interrupt 
    * pending flag by writing with IxxC set to "0", but
    * we'll actually need to implement a BIOS to do that! */

   #define NGP_IRQ_1(x, y) \
       prio = (y); \
       if (ipending[x] && curIFF <= prio && prio && prio != 7) \
       { \
           ipending[x] = 0; \
           interrupt(x, prio); \
           return; \
       }
   
   #define NGP_IRQ_2(x, y, z) \
       if (0) {} \
       else if (z == 0) NGP_IRQ_1(x, IntPrio[y] & 0x07) \
       else if (z == 1) NGP_IRQ_1(x, (IntPrio[y] & 0x70) >> 4)


   uint8_t prio;

   NGP_IRQ_1(0, 6)       // INT0  (SWI3)
   NGP_IRQ_1(1, 6)       // INT1  (SWI4)
   NGP_IRQ_1(2, 6)       // INT2  (SWI5)
   NGP_IRQ_1(3, 6)       // INT3  (SWI6)
   NGP_IRQ_2(4, 0, 0)    // INT4  (RTC)
   NGP_IRQ_2(5, 1, 0)    // INT5  (VBlank)
   NGP_IRQ_2(6, 1, 1)    // INT6  (Z80)
   NGP_IRQ_2(7, 3, 0)    // INT7  (T0)
   NGP_IRQ_2(8, 3, 1)    // INT8  (T1)
   NGP_IRQ_2(9, 4, 0)    // INT9  (T2)
   NGP_IRQ_2(10, 4, 1)   // INT10 (T3)
   NGP_IRQ_2(11, 7, 0)   // INT11 (RX0)
   NGP_IRQ_2(12, 7, 1)   // INT12 (TX0)
   NGP_IRQ_1(13, 6)      // INT13 (RESERVED)
   NGP_IRQ_2(14, 9, 0)   // INT14 (DMA0)
   NGP_IRQ_2(15, 9, 1)   // INT15 (DMA1)
   NGP_IRQ_2(16, 10, 0)  // INT16 (DMA2)
   NGP_IRQ_2(17, 10, 1)  // INT17 (DMA3)
      
   #undef NGP_IRQ_1
   #undef NGP_IRQ_2
}

void int_write8(uint32_t address, uint8_t data)
{
   switch(address)
   {
      case 0x70:
         if(!(data & 0x08))
            ipending[4] = 0;
         break;
      case 0x71:
         if(!(data & 0x08))
            ipending[5] = 0;
         if(!(data & 0x80))
            ipending[6] = 0;
         break;
      case 0x73:
         if(!(data & 0x08))
            ipending[7] = 0; 
         if(!(data & 0x80))
            ipending[8] = 0;
         break;
      case 0x74:
         if(!(data & 0x08))
            ipending[9] = 0;
         if(!(data & 0x80))
            ipending[10] = 0;
         break;
      case 0x77:
         if(!(data & 0x08))
            ipending[11] = 0;
         if(!(data & 0x80))
            ipending[12] = 0;
         break;
      case 0x7C:
         HDMAStartVector[0] = data;
         break;
      case 0x7D:
         HDMAStartVector[1] = data;
         break;
      case 0x7E:
         HDMAStartVector[2] = data;
         break;
      case 0x7F:
         HDMAStartVector[3] = data;
         break;
   }

   if(address >= 0x70 && address <= 0x7A)
   {
      IntPrio[address - 0x70] = data;
      int_check_pending();
   }
}

uint8_t int_read8(uint32_t address)
{
   switch(address)
   {
      case 0x70:
         return (ipending[4] ? 0x08 : 0x00);
      case 0x71:
         return ((ipending[5] ? 0x08 : 0x00) | (ipending[6] ? 0x80 : 0x00));
      case 0x73:
         return ((ipending[7] ? 0x08 : 0x00) | (ipending[8] ? 0x80 : 0x00));
      case 0x74:
         return ((ipending[9] ? 0x08 : 0x00) | (ipending[10] ? 0x80 : 0x00));
      case 0x77:
         return ((ipending[11] ? 0x08 : 0x00) | (ipending[12] ? 0x80 : 0x00));
      default:
         break;
   }

   return 0;
}

void TestIntHDMA(int bios_num, int vec_num)
{
   bool WasDMA = 0;

   if (HDMAStartVector[0] == vec_num)
   {
      WasDMA = 1;
      DMA_update(0);
   }
   else
   {
      if (HDMAStartVector[1] == vec_num)
      {
         WasDMA = 1;
         DMA_update(1);
      }
      else
      {
         if (HDMAStartVector[2] == vec_num)
         {
            WasDMA = 1;
            DMA_update(2);
         }
         else
         {
            if (HDMAStartVector[3] == vec_num)
            {
               WasDMA = 1;
               DMA_update(3);
            }
         }
      }
   }
   if(!WasDMA)
      set_interrupt(bios_num, true);
}


bool updateTimers(void *data, int cputicks)
{
   bool ret = false;

   ngpc_soundTS += cputicks;

   /* increment H-INT timer */
   timer_hint += cputicks;

   /*End of scanline / Start of Next one */
   if (timer_hint >= TIMER_HINT_RATE)
   {
      uint8_t _data;

      h_int = ngpgfx_hint(NGPGfx);	
      ret   = ngpgfx_draw(NGPGfx, data, NGPFrameSkip);

      timer_hint -= TIMER_HINT_RATE;	/* Start of next scanline */

      /* Comms. Read interrupt */
      if ((COMMStatus & 1) == 0 && system_comms_poll(&_data))
      {
         storeB(0x50, _data);
         TestIntHDMA(12, 0x19);
      }
   }

   /* Tick the Clock Generator */
   timer_clock[0] += cputicks;
   timer_clock[1] += cputicks;

   timer0 = false;	/* Clear the timer0 tick, for timer1 chain mode. */

   /* Run Timer 0 (TRUN)? */
   if ((TRUN & 0x01))
   {
      /* T01MOD */
      switch(T01MOD & 0x03)
      {
         case 0:
            /* Horizontal interrupt trigger */
            if (h_int) 
            {
               timer[0]++;

               timer_clock[0] = 0;
               h_int = false;	// Stop h_int remaining active
            }
            break;

         case 1:
            while (timer_clock[0] >= TIMER_T1_RATE)
            {
               timer[0]++;
               timer_clock[0] -= TIMER_T1_RATE;
            }
            break;
         case 2:
            while(timer_clock[0] >= TIMER_T4_RATE)
            {
               timer[0]++;
               timer_clock[0] -= TIMER_T4_RATE;
            }
            break;
         case 3:
            while (timer_clock[0] >= TIMER_T16_RATE)
            {
               timer[0]++;
               timer_clock[0] -= TIMER_T16_RATE;
            }
            break;
      }

      /* Threshold check */
      if (timer_threshold[0] && timer[0] >= timer_threshold[0])
      {
         timer[0] = 0;
         timer0 = true;

         TestIntHDMA(7, 0x10);
      }
   }

   /*Run Timer 1 (TRUN)? */
   if ((TRUN & 0x02))
   {
      /* T01MOD */
      switch((T01MOD & 0x0C) >> 2)
      {
         case 0:
            if (timer0)	//Timer 0 chain mode.
            {
               timer[1] += timer0;
               timer_clock[1] = 0;
            }
            break;
         case 1:
            while (timer_clock[1] >= TIMER_T1_RATE)
            {
               timer[1]++;
               timer_clock[1] -= TIMER_T1_RATE;
            }
            break;
         case 2:
            while (timer_clock[1] >= TIMER_T16_RATE)
            {
               timer[1]++;
               timer_clock[1] -= TIMER_T16_RATE;
            }
            break;
         case 3:
            while (timer_clock[1] >= TIMER_T256_RATE)
            {
               timer[1]++;
               timer_clock[1] -= TIMER_T256_RATE;
            }
            break;
      }

      //Threshold check
      if (timer_threshold[1] && timer[1] >= timer_threshold[1])
      {
         timer[1] = 0;

         TestIntHDMA(8, 0x11);
      }
   }

   /* Tick the Clock Generator */
   timer_clock[2] += cputicks;
   timer_clock[3] += cputicks;

   timer2 = false;	/* Clear the timer2 tick, for timer3 chain mode. */

   /* Run Timer 2 (TRUN)? */
   if ((TRUN & 0x04))
   {
      /* T23MOD */
      switch(T23MOD & 0x03)
      {
         case 0:
            break;
         case 1:
            while (timer_clock[2] >= TIMER_T1_RATE / 2) // Kludge :(
            {
               timer[2]++;
               timer_clock[2] -= TIMER_T1_RATE / 2;
            }
            break;
         case 2:
            while (timer_clock[2] >= TIMER_T4_RATE)
            {
               timer[2]++;
               timer_clock[2] -= TIMER_T4_RATE;
            }
            break;

         case 3:
            while (timer_clock[2] >= TIMER_T16_RATE)
            {
               timer[2]++;
               timer_clock[2] -= TIMER_T16_RATE;
            }
            break;
      }

      //Threshold check
      if (timer_threshold[2] && timer[2] >= timer_threshold[2])
      {
         timer[2] = 0;
         timer2 = true;

         TestIntHDMA(9, 0x12);
      }
   }

   /* Run Timer 3 (TRUN)? */
   if ((TRUN & 0x08))
   {
      /* T23MOD */
      switch((T23MOD & 0x0C) >> 2)
      {
         case 0:
            if(timer2)	//Timer 2 chain mode.
            {
               timer[3] += timer2;
               timer_clock[3] = 0;
            }
            break;
         case 1:
            while (timer_clock[3] >= TIMER_T1_RATE)
            {
               timer[3]++;
               timer_clock[3] -= TIMER_T1_RATE;
            }
            break;
         case 2:
            while (timer_clock[3] >= TIMER_T16_RATE)
            {
               timer[3]++;
               timer_clock[3] -= TIMER_T16_RATE;
            }
            break;

         case 3:
            while (timer_clock[3] >= TIMER_T256_RATE)
            {
               timer[3]++;
               timer_clock[3] -= TIMER_T256_RATE;
            }
            break;
      }

      //Threshold check
      if (timer_threshold[3] && timer[3] >= timer_threshold[3])
      {
         timer[3] = 0;

         Z80_irq();
         TestIntHDMA(10, 0x13);
      }
   }

   return ret;
}

void reset_timers(void)
{
   timer_hint = 0;

   memset(timer, 0, sizeof(timer));
   memset(timer_clock, 0, sizeof(timer_clock));
   memset(timer_threshold, 0, sizeof(timer_threshold));

   timer0 = false;
   timer2 = false;
}

void reset_int(void)
{
   TRUN = 0;
   T01MOD = 0;
   T23MOD = 0;
   TRDC = 0;
   TFFCR = 0;

   memset(HDMAStartVector, 0, sizeof(HDMAStartVector));
   memset(ipending, 0, sizeof(ipending));
   memset(IntPrio, 0, sizeof(IntPrio));

   h_int = false;
}

void timer_write8(uint32_t address, uint8_t data)
{
   switch(address)
   {
      case 0x20:
         TRUN = data;
         if ((TRUN & 0x01) == 0)
            timer[0] = 0;
         if ((TRUN & 0x02) == 0)
            timer[1] = 0;
         if ((TRUN & 0x04) == 0)
            timer[2] = 0;
         if ((TRUN & 0x08) == 0)
            timer[3] = 0;
         break;
      case 0x22:
         timer_threshold[0] = data;
         break;
      case 0x23:
         timer_threshold[1] = data;
         break;
      case 0x24:
         T01MOD = data;
         break;
      case 0x25:
         TFFCR = data & 0x33;
         break;
      case 0x26:
         timer_threshold[2] = data;
         break;
      case 0x27:
         timer_threshold[3] = data;
         break;
      case 0x28:
         T23MOD = data;
         break;
      case 0x29:
         TRDC = data & 0x3;
         break;
   }
}

uint8_t timer_read8(uint32_t address)
{
   switch(address)
   {
      //default: printf("Baaaad: %08x\n", address); break;
      // Cool boarders is stupid and tries to read from a write-only register >_<
      // Returning 4 makes the game run ok, so 4 it is!
      case 0x20:
         return TRUN;
      case 0x29:
         return TRDC;
      default:
         break;
   }

   //printf("UNK B R: %08x\n", address);
   return 0x4;
}



void NGP_INTSaveState(uint_fast8_t load, FILE* fp)
{
	/* Load state */
	if (load == 1)
	{
		fread(&timer_hint, sizeof(uint8_t), sizeof(timer_hint), fp);
		fread(&timer_clock, sizeof(uint8_t), sizeof(timer_clock), fp);
		fread(&timer, sizeof(uint8_t), sizeof(timer), fp);
		fread(&timer_threshold, sizeof(uint8_t), sizeof(timer_threshold), fp);
		
		fread(&TRUN, sizeof(uint8_t), sizeof(TRUN), fp);
		fread(&T01MOD, sizeof(uint8_t), sizeof(T01MOD), fp);
		fread(&T23MOD, sizeof(uint8_t), sizeof(T23MOD), fp);
		fread(&TRDC, sizeof(uint8_t), sizeof(TRDC), fp);
		
		fread(&TFFCR, sizeof(uint8_t), sizeof(TFFCR), fp);
		fread(&HDMAStartVector, sizeof(uint8_t), sizeof(HDMAStartVector), fp);
		fread(&ipending, sizeof(uint8_t), sizeof(ipending), fp);
		fread(&IntPrio, sizeof(uint8_t), sizeof(IntPrio), fp);
		
		fread(&h_int, sizeof(uint8_t), sizeof(h_int), fp);
		fread(&timer0, sizeof(uint8_t), sizeof(timer0), fp);
		fread(&timer2, sizeof(uint8_t), sizeof(timer2), fp);
	}
	/* Save State */
	else
	{
		fwrite(&timer_hint, sizeof(uint8_t), sizeof(timer_hint), fp);
		fwrite(&timer_clock, sizeof(uint8_t), sizeof(timer_clock), fp);
		fwrite(&timer, sizeof(uint8_t), sizeof(timer), fp);
		fwrite(&timer_threshold, sizeof(uint8_t), sizeof(timer_threshold), fp);
		
		fwrite(&TRUN, sizeof(uint8_t), sizeof(TRUN), fp);
		fwrite(&T01MOD, sizeof(uint8_t), sizeof(T01MOD), fp);
		fwrite(&T23MOD, sizeof(uint8_t), sizeof(T23MOD), fp);
		fwrite(&TRDC, sizeof(uint8_t), sizeof(TRDC), fp);
		
		fwrite(&TFFCR, sizeof(uint8_t), sizeof(TFFCR), fp);
		fwrite(&HDMAStartVector, sizeof(uint8_t), sizeof(HDMAStartVector), fp);
		fwrite(&ipending, sizeof(uint8_t), sizeof(ipending), fp);
		fwrite(&IntPrio, sizeof(uint8_t), sizeof(IntPrio), fp);
		
		fwrite(&h_int, sizeof(uint8_t), sizeof(h_int), fp);
		fwrite(&timer0, sizeof(uint8_t), sizeof(timer0), fp);
		fwrite(&timer2, sizeof(uint8_t), sizeof(timer2), fp);
	}
}
/*
int int_timer_StateAction(void *data, int load, int data_only)
{
   SFORMAT StateRegs[] =
   {
      SFVAR(timer_hint),
      SFARRAY32(timer_clock, 4),
      SFARRAY(timer, 4),
      SFARRAY(timer_threshold, 4),
      SFVAR(TRUN),
      SFVAR(T01MOD), SFVAR(T23MOD),
      SFVAR(TRDC),
      SFVAR(TFFCR),
      SFARRAY(HDMAStartVector, 4),
      SFARRAY32(ipending, 24),
      SFARRAY32(IntPrio, 0xB),
      SFVAR(h_int),
      SFVAR(timer0),
      SFVAR(timer2),
      SFEND
   };
   if(!MDFNSS_StateAction(data, load, data_only, StateRegs, "INTT", false))
      return 0;

   return 1;
}*/

#ifdef __cplusplus
}
#endif
