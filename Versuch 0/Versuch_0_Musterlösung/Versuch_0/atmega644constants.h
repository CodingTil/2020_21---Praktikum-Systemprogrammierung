/*! \file
 *  \brief Constants for ATMega644 and EVA board.
 *  Defines several useful constants for the ATMega644 and our AVR evaluation.
 *
 *  \author  Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date    2008, 2009, 2010, 2011, 2012
 *  \version 2.0
 */

#ifndef _ATMEGA644CONSTANTS_H
#define _ATMEGA644CONSTANTS_H

//----------------------------------------------------------------------------
// Board properties
//----------------------------------------------------------------------------

//! Clock fequency on evaluation board in HZ (20 MHZ)
#define AVR_CLOCK_FREQUENCY 20000000ul

#ifdef F_CPU
#undef F_CPU
#endif
//! Clock fequency for WinAVR delay function
#define F_CPU AVR_CLOCK_FREQUENCY


//----------------------------------------------------------------------------
// MC properties
//----------------------------------------------------------------------------

//! Flash memory available on AVR (in bytes) (64 KB)
#define AVR_MEMORY_FLASH (1ul << 16)

//! Flash memory available on AVR (in bytes) (4 KB)
#define AVR_MEMORY_SRAM (1ul << 12)

//! Flash memory available on AVR (in bytes) (2 KB)
#define AVR_MEMORY_EEPROM (1ul << 11)

//! Starting address of SRAM memory
#define AVR_SRAM_START 0x0100
//! Ending address of SRAM memory -- First invalid address
#define AVR_SRAM_END (AVR_SRAM_START + AVR_MEMORY_SRAM)
//! Last address of SRAM memory
#define AVR_SRAM_LAST (AVR_SRAM_END - 1)

#endif
