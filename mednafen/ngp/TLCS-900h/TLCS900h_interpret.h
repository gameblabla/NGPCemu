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

	TLCS900h_interpret.h

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

21 JUL 2002 - neopop_uk
=======================================
- Added the 'instruction_error' function declaration here.

28 JUL 2002 - neopop_uk
=======================================
- Removed CYCLE_WARNING as it is now obsolete.
- Added generic DIV prototypes.

//---------------------------------------------------------------------------
*/

#ifndef __TLCS900H_INTERPRET__
#define __TLCS900H_INTERPRET__

#include <boolean.h>
#include "../../mednafen-types.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================

//Interprets a single instruction from 'pc', 
//pc is incremented to the start of the next instruction.
//Returns the number of cycles taken for this instruction
int32_t TLCS900h_interpret(void);

//=============================================================================

extern uint32_t mem;	
extern uint8_t first;			//First byte
extern uint8_t second;			//Second byte
extern uint8_t R;				//(second & 7)
extern uint8_t rCode;
extern int32_t cycles_cpu_interpreter;
extern uint_fast8_t size_cpu_interpreter;
extern bool brCode;

//=============================================================================

extern void (*instruction_error)(const char* vaMessage,...);

//=============================================================================

#define FETCH8		loadB(pc++)

uint16_t fetch16(void);
uint32_t fetch24(void);
uint32_t fetch32(void);

//=============================================================================

void parityB(uint8_t value);
void parityW(uint16_t value);

//=============================================================================

void push8(uint8_t data);
void push16(uint16_t data);
void push32(uint32_t data);

uint8_t pop8(void);
uint16_t pop16(void);
uint32_t pop32(void);

//=============================================================================

//DIV ===============
uint16_t generic_DIV_B(uint16_t val, uint8_t div);
uint32_t generic_DIV_W(uint32_t val, uint16_t div);

//DIVS ===============
uint16_t generic_DIVS_B(int16 val, int8 div);
uint32_t generic_DIVS_W(int32 val, int16 div);

//ADD ===============
uint8_t	generic_ADD_B(uint8_t dst, uint8_t src);
uint16_t generic_ADD_W(uint16_t dst, uint16_t src);
uint32_t generic_ADD_L(uint32_t dst, uint32_t src);

//ADC ===============
uint8_t	generic_ADC_B(uint8_t dst, uint8_t src);
uint16_t generic_ADC_W(uint16_t dst, uint16_t src);
uint32_t generic_ADC_L(uint32_t dst, uint32_t src);

//SUB ===============
uint8_t	generic_SUB_B(uint8_t dst, uint8_t src);
uint16_t generic_SUB_W(uint16_t dst, uint16_t src);
uint32_t generic_SUB_L(uint32_t dst, uint32_t src);

//SBC ===============
uint8_t	generic_SBC_B(uint8_t dst, uint8_t src);
uint16_t generic_SBC_W(uint16_t dst, uint16_t src);
uint32_t generic_SBC_L(uint32_t dst, uint32_t src);

//=============================================================================

//Confirms a condition code check
bool conditionCode(uint_fast8_t cc);

//=============================================================================

//Translate an rr or RR value for MUL/MULS/DIV/DIVS
uint8_t get_rr_Target(void);
uint8_t get_RR_Target(void);

#ifdef __cplusplus
}
#endif

//=============================================================================
#endif
