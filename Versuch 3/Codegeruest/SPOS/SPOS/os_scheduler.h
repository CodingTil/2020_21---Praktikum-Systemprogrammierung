/*! \file
 *  \brief Scheduling module for the OS.
 *
 *  Contains the scheduler and process switching functionality for the OS.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef _OS_SCHEDULER_H
#define _OS_SCHEDULER_H

#include <stdbool.h>

#include "defines.h"
#include "os_process.h"

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//! The enum specifying which scheduling strategies exist
typedef enum SchedulingStrategy {
    OS_SS_EVEN,
    OS_SS_RANDOM,
    OS_SS_RUN_TO_COMPLETION,
    OS_SS_ROUND_ROBIN,
    OS_SS_INACTIVE_AGING
} SchedulingStrategy;

// Change this define to reflect the number of available strategies:
#define SCHEDULING_STRATEGY_COUNT 5

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------

//! Get a pointer to the process structure by process ID
Process* os_getProcessSlot(ProcessID pid);

//! Starts the scheduler
void os_startScheduler(void);

//! Registers a program (will not be started)
ProgramID os_registerProgram(Program* program);

//! Checks if a program is to be executed at boot-time
bool os_checkAutostartProgram(ProgramID programID);

//! Looks up the function of a program with the passed ID (index) and returns NULL on failure
Program* os_lookupProgramFunction(ProgramID programID);

//! Looks up the ID (i.e. index) of a program and returns INVALID_PROGRAM on failure
ProgramID os_lookupProgramID(Program* program);

//! Executes a process by instantiating a program
ProcessID os_exec(ProgramID programID, Priority priority);

//! Returns the number of programs
uint8_t os_getNumberOfRegisteredPrograms(void);

//! Initializes scheduler arrays
void os_initScheduler(void);

//! Returns the currently active process
ProcessID os_getCurrentProc(void);

//! Returns the number of currently active processes
uint8_t os_getNumberOfActiveProcs(void);

//! Sets the scheduling strategy
void os_setSchedulingStrategy(SchedulingStrategy strategy);

//! Gets the current scheduling strategy
SchedulingStrategy os_getSchedulingStrategy(void);

//! Calculates the checksum of the stack for the corresponding process of pid.
StackChecksum os_getStackChecksum(ProcessID pid);

//----------------------------------------------------------------------------
// Critical section management
//----------------------------------------------------------------------------

//! Enters a critical code section
void os_enterCriticalSection(void);

//! Leaves a critical code section
void os_leaveCriticalSection(void);

#endif
