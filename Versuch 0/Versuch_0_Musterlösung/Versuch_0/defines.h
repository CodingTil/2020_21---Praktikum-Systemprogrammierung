/*! \file
 *  \brief Simple definitions and assembler-macros, mostly 8/16-Bit values.
 *
 *  All constant values that can be simply parsed into the code at compile
 *  time are stored in here. These will reside in program memory at runtime.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2008, 2009, 2010, 2011, 2012
 *  \version  2.0
 */

#ifndef _DEFINES_H
#define _DEFINES_H

#include "atmega644constants.h"

//----------------------------------------------------------------------------
// Programming reliefs
//----------------------------------------------------------------------------

//! null-pointer
//! The rationale for this is to be compliant with stddef::NULL without
//! actually including it.
#ifndef NULL
#define NULL ((void*)0)
#endif

//----------------------------------------------------------------------------
// Debug/Version settings
//----------------------------------------------------------------------------

//! The current id of the exercise (this must be changed every two weeks).
#define VERSUCH 3

//----------------------------------------------------------------------------
// System constants
//----------------------------------------------------------------------------

//! Maximum number of processes that can be running at the same time
//! (may be nothing > 8).
//! This number includes the idle proc, although it is considered a system proc.
//! The idle proc. has always id 0. The highest ID is MAX_NUMBER_OF_PROCESSES-1.
#define MAX_NUMBER_OF_PROCESSES 8

//! Maximum number of programs that can be known by the os (<17, 255 is invalid).
#define MAX_NUMBER_OF_PROGRAMS 16

//! Standard priority for newly created processes
#define DEFAULT_PRIORITY 2

//! Default delay to read display values (in ms)
#define DEFAULT_OUTPUT_DELAY 100

//----------------------------------------------------------------------------
// Scheduler constants
//----------------------------------------------------------------------------

//! Number to specify an invalid process
#define INVALID_PROCESS 255

//! Number to specify an invalid program.
#define INVALID_PROGRAM 255


//----------------------------------------------------------------------------
// Stack constants
//----------------------------------------------------------------------------

// \cond SHOW_STACK_DEFINES

//! The stack size available for initialization and globals
#define STACK_SIZE_MAIN 0x100

//! The scheduler's stack size
#define STACK_SIZE_ISR 0x100

//! The stack size of a process
#define STACK_SIZE_PROC (((AVR_MEMORY_SRAM/2)-STACK_SIZE_MAIN-STACK_SIZE_ISR)/MAX_NUMBER_OF_PROCESSES)

//! The bottom of the main stack. That is the highest address.
#define BOTTOM_OF_MAIN_STACK (AVR_SRAM_LAST)

//! The bottom of the scheduler-stack. That is the highest address.
#define BOTTOM_OF_ISR_STACK (BOTTOM_OF_MAIN_STACK-STACK_SIZE_MAIN)

//! The bottom of the memory chunks for all process stacks. That is the highest address.
#define BOTTOM_OF_PROCS_STACK (BOTTOM_OF_ISR_STACK-STACK_SIZE_ISR)

//! The bottom of the memory chunk with number PID.
#define PROCESS_STACK_BOTTOM(PID) (BOTTOM_OF_PROCS_STACK-(PID*(STACK_SIZE_PROC)))

//! The end of the last process stack, i.e. the first invalid address
#define TOP_OF_PROCESS_STACK (BOTTOM_OF_PROCS_STACK - MAX_NUMBER_OF_PROCESSES*STACK_SIZE_PROC)

// \endcond

//----------------------------------------------------------------------------
// Heap constants
//----------------------------------------------------------------------------

// \cond SHOW_HEAP_DEFINES

//! Offset needed before the Heap starts, because global variables are put on the low addresses of the SRAM
#define HEAPOFFSET (AVR_SRAM_START + 200)

// \endcond


#endif
