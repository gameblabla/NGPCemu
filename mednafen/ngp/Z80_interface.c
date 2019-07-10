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
#include "mem.h"
#include "sound.h"
#include "Z80_interface.h"
#include "TLCS-900h/TLCS900h_registers.h"
#include "interrupt.h"
#include "dma.h"
#include "../hw_cpu/z80-fuse/z80.h"
#include "../hw_cpu/z80-fuse/z80_macros.h"

static uint8_t CommByte;
static bool Z80Enabled;

void NGP_Z80SaveState(uint_fast8_t load, FILE* fp)
{
	uint8_t r_register = 0;
	
	/* Load state */
	if (load == 1)
	{
		fread(&z80, sizeof(uint8_t), sizeof(z80), fp);
		
		fread(&r_register, sizeof(uint8_t), sizeof(r_register), fp);

		fread(&z80_tstates, sizeof(uint8_t), sizeof(z80_tstates), fp);
		fread(&last_z80_tstates, sizeof(uint8_t), sizeof(last_z80_tstates), fp);
		
		fread(&CommByte, sizeof(uint8_t), sizeof(CommByte), fp);
		fread(&Z80Enabled, sizeof(uint8_t), sizeof(Z80Enabled), fp);
		
		z80.r7 = r_register & 0x80;
		z80.r = r_register & 0x7F;
	}
	/* Save State */
	else
	{
		fwrite(&z80, sizeof(uint8_t), sizeof(z80), fp);

		fwrite(&r_register, sizeof(uint8_t), sizeof(r_register), fp);

		fwrite(&z80_tstates, sizeof(uint8_t), sizeof(z80_tstates), fp);
		fwrite(&last_z80_tstates, sizeof(uint8_t), sizeof(last_z80_tstates), fp);
		
		fwrite(&CommByte, sizeof(uint8_t), sizeof(CommByte), fp);
		fwrite(&Z80Enabled, sizeof(uint8_t), sizeof(Z80Enabled), fp);
		
		r_register = (z80.r7 & 0x80) | (z80.r & 0x7f);
	}
}

uint8_t Z80_ReadComm(void)
{
	return CommByte;
}

void Z80_WriteComm(uint8_t data)
{
	CommByte = data;
}

static uint8_t NGP_z80_readbyte(uint16_t address)
{
	switch (address)
	{
		case 0x8000:
			return CommByte;
		//  if (address <= 0x0FFF)
		default:
			return loadB(0x7000 + address);
	}
	return 0;
}

static void NGP_z80_writebyte(uint16_t address, uint8_t value)
{
	switch (address)
	{
		case 0x8000:
			CommByte = value;
		break;
		case 0x4001:
			Write_SoundChipLeft(value);
		break;
		case 0x4000:
			Write_SoundChipRight(value);
		break;
		case 0xC000:
			TestIntHDMA(6, 0x0C);
		break;
		// if (address <= 0x0FFF)
		default:
			storeB(0x7000 + address, value);
		break;
	}
}

static void NGP_z80_writeport(uint16_t port, uint8_t value)
{
	//printf("Portout: %04x %02x\n", port, value);
	z80_set_interrupt(0);
}

void Z80_nmi(void)
{
	z80_nmi();
}

void Z80_irq(void)
{
	z80_set_interrupt(1);
}

void Z80_reset(void)
{
	Z80Enabled = 0;

	z80_writebyte = NGP_z80_writebyte;
	z80_readbyte = NGP_z80_readbyte;
	z80_writeport = NGP_z80_writeport;

	z80_init();
	z80_reset();
}

void Z80_SetEnable(bool set)
{
   Z80Enabled = set;
   if(!set)
      z80_reset();
}

bool Z80_IsEnabled(void)
{
   return(Z80Enabled);
}

int Z80_RunOP(void)
{
   if(!Z80Enabled)
      return -1;

   return(z80_do_opcode());
}
