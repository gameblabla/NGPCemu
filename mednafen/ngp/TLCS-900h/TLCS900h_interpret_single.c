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

/*
//---------------------------------------------------------------------------
//=========================================================================

	TLCS900h_interpret_single.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

22 JUL 2002 - neopop_uk
=======================================
- Removed setting the register file pointer to 3 in the SWI instruction
	This has fixed "Metal Slug 2" and flash saving in many games.

26 JUL 2002 - neopop_uk
=======================================
- Prefixed all instruction functions with "sng" to avoid a repeat of the
	the 'EX' fiasco.

30 JUL 2002 - neopop_uk
=======================================
- Um... SWI doesn't cause a problem if IFF is set to 3... why did 
"Metal Slug 2" start working???

//---------------------------------------------------------------------------
*/

#include <stdio.h>
#include "TLCS900h_interpret.h"
#include "TLCS900h_registers.h"
#include "../mem.h"
#include "../interrupt.h"

//=========================================================================

//===== NOP
void sngNOP()
{
	cycles_cpu_interpreter = 2;
}

//===== NORMAL
void sngNORMAL()
{
	//Not supported
	cycles_cpu_interpreter = 4;
}

//===== PUSH SR
void sngPUSHSR()
{
	push16(sr);
	cycles_cpu_interpreter = 4;
}

//===== POP SR
void sngPOPSR()
{
	sr = pop16();	changedSP();
	cycles_cpu_interpreter = 6;
}

//===== MAX
void sngMAX()
{
	//Not supported
	cycles_cpu_interpreter = 4;
}

//===== HALT
void sngHALT()
{
	//MDFN_printf("CPU halt requested and ignored.\nPlease send me a saved state.");
	cycles_cpu_interpreter = 8;
}

//===== EI #3
void sngEI()
{
	setStatusIFF(FETCH8);
	int_check_pending();
	cycles_cpu_interpreter = 5;
}

//===== RETI
void sngRETI()
{
	uint16 temp = pop16();
	pc = pop32();
	sr = temp; changedSP();
	cycles_cpu_interpreter = 12;
}

//===== LD (n), n
void sngLD8_8()
{
	uint8 dst = FETCH8;
	uint8 src = FETCH8;
	storeB(dst, src);
	cycles_cpu_interpreter = 5;
}

//===== PUSH n
void sngPUSH8()
{
	uint8 data = FETCH8;
	push8(data);
	cycles_cpu_interpreter = 4;
}

//===== LD (n), nn
void sngLD8_16()
{
	uint8 dst = FETCH8;
	uint16 src = fetch16();
	storeW(dst, src);
	cycles_cpu_interpreter = 6;
}

//===== PUSH nn
void sngPUSH16()
{
	push16(fetch16());
	cycles_cpu_interpreter = 5;
}

//===== INCF
void sngINCF()
{
	setStatusRFP(((sr & 0x300) >> 8) + 1);
	cycles_cpu_interpreter = 2;
}

//===== DECF
void sngDECF()
{
	setStatusRFP(((sr & 0x300) >> 8) - 1);
	cycles_cpu_interpreter = 2;
}

//===== RET condition
void sngRET()
{
	pc = pop32();
	cycles_cpu_interpreter = 9;
}

//===== RETD dd
void sngRETD()
{
	int16 d = (int16)fetch16();
	pc = pop32();
	REGXSP += d;
	cycles_cpu_interpreter = 9;
}

//===== RCF
void sngRCF()
{
	SETFLAG_N0;
	SETFLAG_V0;
	SETFLAG_C0;
	cycles_cpu_interpreter = 2;
}

//===== SCF
void sngSCF()
{
	SETFLAG_H0;
	SETFLAG_N0;
	SETFLAG_C1;
	cycles_cpu_interpreter = 2;
}

//===== CCF
void sngCCF()
{
	SETFLAG_N0;
	SETFLAG_C(!FLAG_C);
	cycles_cpu_interpreter = 2;
}

//===== ZCF
void sngZCF()
{
	SETFLAG_N0;
	SETFLAG_C(!FLAG_Z);
	cycles_cpu_interpreter = 2;
}

//===== PUSH A
void sngPUSHA()
{
	push8(REGA);
	cycles_cpu_interpreter = 3;
}

//===== POP A
void sngPOPA()
{
	REGA = pop8();
	cycles_cpu_interpreter = 4;
}

//===== EX F,F'
void sngEX()
{
	uint8 f = sr & 0xFF;
	sr = (sr & 0xFF00) | f_dash;
	f_dash = f;
	cycles_cpu_interpreter = 2;
}

//===== LDF #3
void sngLDF()
{
	setStatusRFP(FETCH8);
	cycles_cpu_interpreter = 2;
}

//===== PUSH F
void sngPUSHF()
{
	push8(sr & 0xFF);
	cycles_cpu_interpreter = 3;
}

//===== POP F
void sngPOPF()
{
	sr = (sr & 0xFF00) | pop8();
	cycles_cpu_interpreter = 4;
}

//===== JP nn
void sngJP16()
{
	pc = fetch16();
	cycles_cpu_interpreter = 7;
}

//===== JP nnn
void sngJP24()
{
	pc = fetch24();
	cycles_cpu_interpreter = 7;
}

//===== CALL #16
void sngCALL16()
{
	uint32 target = fetch16();
	push32(pc);
	pc = target;
	cycles_cpu_interpreter = 12;
}

//===== CALL #24
void sngCALL24()
{
	uint32 target = fetch24();
	push32(pc);
	pc = target;
	cycles_cpu_interpreter = 12;
}

//===== CALR $+3+d16
void sngCALR()
{
	int16 displacement = (int16)fetch16();
	uint32 target = pc + displacement;
	push32(pc);
	pc = target;
	cycles_cpu_interpreter = 12;
}

//===== LD R, n
void sngLDB()
{
	regB(first & 7) = FETCH8;
	cycles_cpu_interpreter = 2;
}

//===== PUSH RR
void sngPUSHW()
{
	push16(regW(first & 7));
	cycles_cpu_interpreter = 3;
}

//===== LD RR, nn
void sngLDW()
{
	regW(first & 7) = fetch16();
	cycles_cpu_interpreter = 3;
}

//===== PUSH XRR
void sngPUSHL()
{
	push32(regL(first & 7));
	cycles_cpu_interpreter = 5;
}

//===== LD XRR, nnnn
void sngLDL()
{
	regL(first & 7) = fetch32();
	cycles_cpu_interpreter = 5;
}

//===== POP RR
void sngPOPW()
{
	regW(first & 7) = pop16();
	cycles_cpu_interpreter = 4;
}

//===== POP XRR
void sngPOPL()
{
	regL(first & 7) = pop32();
	cycles_cpu_interpreter = 6;
}

//===== JR cc,PC + d
void sngJR()
{
	if (conditionCode(first & 0xF))
	{
		int8 displacement = (int8)FETCH8;

		cycles_cpu_interpreter = 8;
		pc += displacement;
	}
	else
	{
		cycles_cpu_interpreter = 4;
		FETCH8;
	}
}

//===== JR cc,PC + dd
void sngJRL()
{
	if (conditionCode(first & 0xF))
	{
		int16 displacement = (int16)fetch16();
		cycles_cpu_interpreter = 8;
		pc += displacement;
	}
	else
	{
		cycles_cpu_interpreter = 4;
		fetch16();
	}
}

//===== LDX dst,src
void sngLDX()
{
	uint8 dst, src;

	FETCH8;			//00
	dst = FETCH8;	//#8
	FETCH8;			//00
	src = FETCH8;	//#
	FETCH8;			//00

	storeB(dst, src);
	cycles_cpu_interpreter = 9;
}

//===== SWI num
void sngSWI()
{
	cycles_cpu_interpreter = 16;

	/*printf("SWI: %02x\n", first & 0x7);*/
	
	switch(first & 7)
	{
	//System Call
	case 1: push32(pc);
		pc = loadL(0xFFFE00 + ((rCodeB(0x31) & 0x1F) << 2));
		break;

	case 3: set_interrupt(0, true); break;  //SWI 3
	case 4: set_interrupt(1, true); break;  //SWI 4
	case 5: set_interrupt(2, true); break;  //SWI 5
	case 6: set_interrupt(3, true); break;  //SWI 6

	default:
	#ifdef TLCS_ERRORS
		instruction_error("SWI %d is not valid.", first & 7);
	#endif
	break;
	}
}
//=============================================================================
