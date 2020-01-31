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

#include "neopop.h"
#include "bios.h"
#include "TLCS-900h/TLCS900h_registers.h"
#include "TLCS-900h/TLCS900h_interpret.h"
#include "mem.h"
#include "flash.h"
#include "interrupt.h"

#include <boolean.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Interrupt prio registers at 0x0070-0x007a don't have priority readable. */
/* This should probably be stored in BIOS work RAM somewhere instead of a 
 * separate array, but I don't know where! */
static uint8_t CacheIntPrio[0xB]; 

void BIOSHLE_Reset(void)
{
   uint8_t x;

   memset(CacheIntPrio, 0, sizeof(CacheIntPrio));
   CacheIntPrio[0] = 0x02;
   CacheIntPrio[1] = 0x32;

   for(x = 0; x < 0xB; x++)
      storeB(0x70 + x, CacheIntPrio[x]);
}

#define VECT_SHUTDOWN         0xFF27A2
#define VECT_CLOCKGEARSET     0xFF1030
#define VECT_COMGETBUFDATA    0xFF2D85
#define VECT_COMCREATEBUFDATA 0xFF2D6C

/* This is the custom HLE instruction. I considered it was the fastest and
most streamlined way of intercepting a bios call. The operation performed
is dependant on the current program counter. */

void iBIOSHLE(void)
{
   /* Only works within the bios */
   if ((pc & 0xFF0000) != 0xFF0000)
      return;

   pc      --;	    /* Compensate for processing this instruction. */
   cycles_cpu_interpreter = 8;		/* TODO: Correct cycle counts (or approx?) */

   switch (pc & 0xffffff)
   {	
      case VECT_SHUTDOWN:
         {
            //Cheap bit of code to stop the message appearing repeatedly.
            uint32 a = pop32();
            if (a != 0xBAADC0DE)
               printf("IDS_POWER");
            push32(0xBAADC0DE); //Sure is!
         }

         return;	//Don't pop a return address, stay here
      case VECT_CLOCKGEARSET:
         break;

         //VECT_RTCGET
      case 0xFF1440:

         if (rCodeL(0x3C) < 0xC000)
         {
            //Copy data from hardware area
            storeB(rCodeL(0x3C) + 0, loadB(0x91));
            storeB(rCodeL(0x3C) + 1, loadB(0x92));
            storeB(rCodeL(0x3C) + 2, loadB(0x93));
            storeB(rCodeL(0x3C) + 3, loadB(0x94));
            storeB(rCodeL(0x3C) + 4, loadB(0x95));
            storeB(rCodeL(0x3C) + 5, loadB(0x96));
            storeB(rCodeL(0x3C) + 6, loadB(0x97));
         }

         break; 

         //VECT_INTLVSET
      case 0xFF1222:
         {

            uint8 level = rCodeB(0x35); //RB3
            uint8 interrupt = rCodeB(0x34);	//RC3

            //   0 - Interrupt from RTC alarm
            //   1 - Interrupt from the Z80 CPU
            //   2 - Interrupt from the 8 bit timer 0
            //   3 - Interrupt from the 8 bit timer 1
            //   4 - Interrupt from the 8 bit timer 2
            //   5 - Interrupt from the 8 bit timer 3
            //   6 - End of transfer interrupt from DMA channel 0
            //   7 - End of transfer interrupt from DMA channel 1
            //   8 - End of transfer interrupt from DMA channel 2
            //   9 - End of transfer interrupt from DMA channel 3

            switch(interrupt)
            {
               case 0x00:
                  CacheIntPrio[0x0] = (CacheIntPrio[0x0] & 0xf0) |  (level & 0x07);
                  storeB(0x70, CacheIntPrio[0x0]);
                  break;
               case 0x01:
                  CacheIntPrio[0x1] = (CacheIntPrio[0x1] & 0x0f) | ((level & 0x07)<<4);
                  storeB(0x71, CacheIntPrio[0x1]);
                  break;
               case 0x02:
                  CacheIntPrio[0x3] = (CacheIntPrio[0x3] & 0xf0) |  (level & 0x07);
                  storeB(0x73, CacheIntPrio[0x3]);
                  break;
               case 0x03:
                  CacheIntPrio[0x3] = (CacheIntPrio[0x3] & 0x0f) | ((level & 0x07)<<4);
                  storeB(0x73, CacheIntPrio[0x3]);
                  break;
               case 0x04:
                  CacheIntPrio[0x4] = (CacheIntPrio[0x4] & 0xf0) |  (level & 0x07);
                  storeB(0x74, CacheIntPrio[0x4]);
                  break;
               case 0x05:
                  CacheIntPrio[0x4] = (CacheIntPrio[0x4] & 0x0f) | ((level & 0x07)<<4);
                  storeB(0x74, CacheIntPrio[0x4]);
                  break;
               case 0x06:
                  CacheIntPrio[0x9] = (CacheIntPrio[0x9] & 0xf0) |  (level & 0x07);
                  storeB(0x79, CacheIntPrio[0x9]);
                  break;
               case 0x07:
                  CacheIntPrio[0x9] = (CacheIntPrio[0x9] & 0x0f) | ((level & 0x07)<<4);
                  storeB(0x79, CacheIntPrio[0x9]);
                  break;
               case 0x08:
                  CacheIntPrio[0xa] = (CacheIntPrio[0xa] & 0xf0) |  (level & 0x07);
                  storeB(0x7a, CacheIntPrio[0xa]);
                  break;
               case 0x09:
                  CacheIntPrio[0xa] = (CacheIntPrio[0xa] & 0x0f) | ((level & 0x07)<<4);
                  storeB(0x7a, CacheIntPrio[0xa]);
                  break;
               default: 
                  puts("DOH");
                  break;
            }
         }
         break;	

         //VECT_SYSFONTSET
      case 0xFF8D8A:
         {
            uint8 c, j;
            uint16 i, dst = 0xA000;

            uint8 b = rCodeB(0x30) >> 4;
            uint8 a = rCodeB(0x30) & 3;

            for (i = 0; i < 0x800; i++)
            {
               c = ngpc_bios[0x8DCF + i];

               for (j = 0; j < 8; j++, c<<=1)
               {
                  uint16 data16;

                  data16 = loadW(dst);
                  data16 <<= 2;
                  storeW(dst, data16);

                  if (c & 0x80)	storeB(dst, loadB(dst) | a);
                  else			storeB(dst, loadB(dst) | b);
               }

               dst += 2;
            }
         }

         break;

         //VECT_FLASHWRITE
      case 0xFF6FD8:
         {
            uint32 i, bank = 0x200000;

            //Select HI rom?
            if (rCodeB(0x30) == 1)
               bank = 0x800000;

            memory_flash_error = false;
            memory_unlock_flash_write = true;
            //Copy as 32 bit values for speed
            for (i = 0; i < rCodeW(0x34) * 64ul; i++)
               storeL(rCodeL(0x38) + bank + (i * 4), loadL(rCodeL(0x3C) + (i * 4)));
            memory_unlock_flash_write = false;

            if (memory_flash_error)
            {
               rCodeB(0x30) = 0xFF;	//RA3 = SYS_FAILURE
            }
            else
            {
               uint32 address = rCodeL(0x38);
               if (rCodeB(0x30) == 1)
                  address += 0x800000;
               else
                  address += 0x200000;

               //Save this data to an external file
               flash_write(address, rCodeW(0x34) * 256);

               rCodeB(0x30) = 0;		//RA3 = SYS_SUCCESS
            }
         }

         break;

         //VECT_FLASHALLERS
      case 0xFF7042:
         //TODO
         rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
         break;

         //VECT_FLASHERS
      case 0xFF7082:
         {
            uint32 i, bank = 0x200000, size = 0x10000, dst;

            //Select HI rom?
            if (rCodeB(0x30) == 1)
               bank = 0x800000;

            //TODO:check rom size to determine final dst block
            // 4 MBit - block  7, 270000-27FFFF
            // 8 MBit - block 15, 2F0000-2FFFFF
            //16 MBit - block 31, 3F0000-3F7FFF
            if (rCodeB(0x35) == 31)
               size = 0x8000;
            dst = rCodeB(0x35) * 0x10000;

            memory_flash_error = false;
            memory_unlock_flash_write = true;
            //Copy as 32 bit values for speed
            for (i = 0; i < size / 4; i++)
               storeL(bank + dst + (i * 4), 0xFFFFFFFF);
            memory_unlock_flash_write = false;

            rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
         }
         break;

         //VECT_ALARMSET
      case 0xFF149B:
         //TODO
         rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
         break;

         //VECT_ALARMDOWNSET
      case 0xFF1487:
         //TODO
         rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
         break;

         //VECT_FLASHPROTECT
      case 0xFF70CA:
         //TODO
         rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
         break;

         //VECT_GEMODESET
      case 0xFF17C4:
         //TODO
         break;

         //VECT_COMINIT
      case 0xFF2BBD:
         // Nothing to do.
         rCodeB(0x30) = 0;	//RA3 = COM_BUF_OK
         break;

         //VECT_COMSENDSTART
      case 0xFF2C0C:
         // Nothing to do.
         break;

         //VECT_COMRECIVESTART
      case 0xFF2C44:
         // Nothing to do.
         break;

         //VECT_COMCREATEDATA
      case 0xFF2C86:
         {
            //Write the byte
            /*
            uint8 data = rCodeB(0x35);
            system_comms_write(data);
            */
         }

         //Restore $PC after BIOS-HLE instruction
         pc = pop32();

         TestIntHDMA(11, 0x18);

         //Always COM_BUF_OK because the write call always succeeds.
         rCodeB(0x30) = 0x0;			//RA3 = COM_BUF_OK
         return;

         //VECT_COMGETDATA
      case 0xFF2CB4:
         {
            /*
            uint8 data;

            if (system_comms_read(&data))
            {
               rCodeB(0x30) = 0;	//COM_BUF_OK
               rCodeB(0x35) = data;

               pc = pop32();

               //Comms. Read interrupt
               storeB(0x50, data);
               TestIntHDMA(12, 0x19);

               return;
            }
            else
            */
            {
               rCodeB(0x30) = 1;	//COM_BUF_EMPTY
            }
         }

         break;

         //VECT_COMONRTS
      case 0xFF2D27:
         storeB(0xB2, 0);
         break;

         //VECT_COMOFFRTS
      case 0xFF2D33: 	
         storeB(0xB2, 1);
         break;	

         //VECT_COMSENDSTATUS
      case 0xFF2D3A:
         // Nothing to do.
         rCodeW(0x30) = 0;	//Send Buffer Count, never any pending data!
         break;

         //VECT_COMRECIVESTATUS
      case 0xFF2D4E:

         // Receive Buffer Count
         /*rCodeW(0x30) = system_comms_read(NULL);*/
		 rCodeW(0x30) = 0;
         break;

      case VECT_COMCREATEBUFDATA:
         pc = pop32();

         while(rCodeB(0x35) > 0)
         {
            /*
            uint8 data;
            data = loadB(rCodeL(0x3C));

            //Write data from (XHL3++)
            system_comms_write(data);
            */
            rCodeL(0x3C)++; //Next data

            rCodeB(0x35)--;	//RB3 = Count Left
         }

         TestIntHDMA(11, 0x18);
         return;
      case VECT_COMGETBUFDATA:
         pc = pop32();

		 /*while(rCodeB(0x35) > 0)
         {
            uint8 data;

            if (system_comms_read(&data))
            {
               //Read data into (XHL3++)
               storeB(rCodeL(0x3C), data);
               rCodeL(0x3C)++; //Next data
               rCodeB(0x35)--;	//RB3 = Count Left

               //Comms. Read interrupt
               storeB(0x50, data);
               TestIntHDMA(12, 0x19);
               return;
            }
            break;
         }*/

         return;
   }

   //RET
   pc = pop32();
}

#ifdef __cplusplus
}
#endif

void NGP_HLESaveState(uint_fast8_t load, FILE* fp)
{
	if (load == 1)
	{
		fread(&CacheIntPrio, sizeof(uint8_t), sizeof(CacheIntPrio), fp);
	}
	else
	{
		fwrite(&CacheIntPrio, sizeof(uint8_t), sizeof(CacheIntPrio), fp);
	}
}
