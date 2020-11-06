#include "os_scheduler.h"
#include "util.h"
#include "os_input.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "os_core.h"
#include "lcd.h"

#include <avr/interrupt.h>

//----------------------------------------------------------------------------
// Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//! Array of states for every possible process
Process os_processes[MAX_NUMBER_OF_PROCESSES];

//! Array of function pointers for every registered program
Program* os_programs[MAX_NUMBER_OF_PROGRAMS];

//! Index of process that is currently executed (default: idle)
ProcessID currentProc;

//----------------------------------------------------------------------------
// Private variables
//----------------------------------------------------------------------------

//! Currently active scheduling strategy
SchedulingStrategy currentSchedStrat;

//! Count of currently nested critical sections
uint8_t criticalSectionCount;

//! Used to auto-execute programs.
uint16_t os_autostart;

//----------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------

//! ISR for timer compare match (scheduler)
ISR(TIMER2_COMPA_vect) __attribute__((naked));

//----------------------------------------------------------------------------
// Function definitions
//----------------------------------------------------------------------------

/*!
 *  Timer interrupt that implements our scheduler. Execution of the running
 *  process is suspended and the context saved to the stack. Then the periphery
 *  is scanned for any input events. If everything is in order, the next process
 *  for execution is derived with an exchangeable strategy. Finally the
 *  scheduler restores the next process for execution and releases control over
 *  the processor to that process.
 */
ISR(TIMER2_COMPA_vect) {
	/*
	os_processes[currentProc]->sp.as_ptr[0] = PC & 0b1111;
	os_processes[currentProc]->sp.as_ptr[-1] = PC>>4;
	os_processes[currentProc]->sp.as_ptr -= 2;
	*/
	saveContext();
	
	os_processes[currentProc]->sp.as_ptr = SP;
	
	SP = BOTTOM_OF_ISR_STACK;
	
	os_processes[currentProc]->checksum = os_getStackChecksum(currentProc);
	
	if (os_getInput() == 0b10000001) {
		os_waitForNoInput();
		os_taskManMain();
	}
	
	os_processes[currentProc]->state = OS_PS_READY;
	
	switch(currentSchedStrat) {
		case OS_SS_EVEN: currentProc = os_Scheduler_Even(os_processes, currentProc); break;
		case OS_SS_RANDOM: currentProc = os_Scheduler_Random(os_processes, currentProc); break;
		case OS_SS_ROUND_ROBIN: currentProc = os_Scheduler_RoundRobin(os_processes, currentProc); break;
		case OS_SS_INACTIVE_AGING: currentProc = os_Scheduler_InactiveAging(os_processes, currentProc); break;
		case OS_SS_RUN_TO_COMPLETION: currentProc = os_Scheduler_RunToCompletion(os_processes, currentProc); break;
	}
	
	if (os_processes[currentProc]->checksum != os_getStackChecksum(currentProc)) {
		os_error("There is an inconsistancy in the stack.");
	}
	os_processes[currentProc]->state = OS_PS_RUNNING;
	
	SP = os_processes[currentProc]->sp.as_ptr;
	
	restoreContext();
}

/*!
 *  Used to register a function as program. On success the program is written to
 *  the first free slot within the os_programs array (if the program is not yet
 *  registered) and the index is returned. On failure, INVALID_PROGRAM is returned.
 *  Note, that this function is not used to register the idle program.
 *
 *  \param program The function you want to register.
 *  \return The index of the newly registered program.
 */
ProgramID os_registerProgram(Program* program) {
    ProgramID slot = 0;

    // Find first free place to put our program
    while (os_programs[slot] &&
           os_programs[slot] != program && slot < MAX_NUMBER_OF_PROGRAMS) {
        slot++;
    }

    if (slot >= MAX_NUMBER_OF_PROGRAMS) {
        return INVALID_PROGRAM;
    }

    os_programs[slot] = program;
    return slot;
}

/*!
 *  Used to check whether a certain program ID is to be automatically executed at
 *  system start.
 *
 *  \param programID The program to be checked.
 *  \return True if the program with the specified ID is to be auto started.
 */
bool os_checkAutostartProgram(ProgramID programID) {
    return !!(os_autostart & (1 << programID));
}

/*!
 *  This is the idle program. The idle process owns all the memory
 *  and processor time no other process wants to have.
 */
PROGRAM(0, AUTOSTART) {
    while(true) {
		lcd_writeProgString(PSTR("."));
		delayMs(DEFAULT_OUTPUT_DELAY);
	}
}

/*!
 * Lookup the main function of a program with id "programID".
 *
 * \param programID The id of the program to be looked up.
 * \return The pointer to the according function, or NULL if programID is invalid.
 */
Program* os_lookupProgramFunction(ProgramID programID) {
    // Return NULL if the index is out of range
    if (programID >= MAX_NUMBER_OF_PROGRAMS) {
        return NULL;
    }

    return os_programs[programID];
}

/*!
 * Lookup the id of a program.
 *
 * \param program The function of the program you want to look up.
 * \return The id to the according slot, or INVALID_PROGRAM if program is invalid.
 */
ProgramID os_lookupProgramID(Program* program) {
    ProgramID i;

    // Search program array for a match
    for (i = 0; i < MAX_NUMBER_OF_PROGRAMS; i++) {
        if (os_programs[i] == program) {
            return i;
        }
    }

    // If no match was found return INVALID_PROGRAM
    return INVALID_PROGRAM;
}

/*!
 *  This function is used to execute a program that has been introduced with
 *  os_registerProgram.
 *  A stack will be provided if the process limit has not yet been reached.
 *  In case of an error, an according message will be displayed on the LCD.
 *  This function is multitasking safe. That means that programs can repost
 *  themselves, simulating TinyOS 2 scheduling (just kick off interrupts ;) ).
 *
 *  \param programID The program id of the program to start (index of os_programs).
 *  \param priority A priority ranging 0..255 for the new process:
 *                   - 0 means least favorable
 *                   - 255 means most favorable
 *                  Note that the priority may be ignored by certain scheduling
 *                  strategies.
 *  \return The index of the new process (throws error on failure and returns
 *          INVALID_PROCESS as specified in defines.h).
 */
ProcessID os_exec(ProgramID programID, Priority priority) {
    uint8_t index; // 8-bit, because MAX_NUMBER_OF_PROCESSES is 8
	for(index = 0; index < MAX_NUMBER_OF_PROCESSES; index++) {
		if(os_processes[index]->state == OS_PS_UNUSED) break;
	}
	if(index >= MAX_NUMBER_OF_PROGRAMS) return INVALID_PROCESS;
	
	os_resetProcessSchedulingInformation(index);
	
	Program* prog = os_lookupProgramFunction(programID);
	if(prog == NULL) return INVALID_PROCESS;
	
	os_processes[index]->state = OS_PS_READY;
	os_processes[index]->progID = programID;
	os_processes[index]->priority = priority;
	os_processes[index]->age = priority;
	
	StackPointer proccess_stack_bottom = PROCESS_STACK_BOTTOM(programID);
	proccess_stack_bottom.as_ptr = prog & 0b1111;
	proccess_stack_bottom.as_ptr[-1] = prog>>4;
	proccess_stack_bottom.as_ptr -= 2;
	for(int i = 0; i < 33; i++) {
		proccess_stack_bottom.as_ptr[-i] = 0;
	}
	proccess_stack_bottom.as_ptr -= 33;
	os_processes[index]->sp = proccess_stack_bottom;
	os_processes[index]->checksum = os_getStackChecksum(index);
	return index;
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
    currentProc = 0;
	os_processes[0]->state = OS_PS_RUNNING;
	SP = os_processes[0]->sp;
	restoreContext();
}

/*!
 *  In order for the Scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register.
 */
void os_initScheduler(void) {
    for(int index = 0; index < MAX_NUMBER_OF_PROCESSES; index++) {
		os_processes[index]->state = OS_PS_UNUSED;
	}
	
	for(int index = 0; index < MAX_NUMBER_OF_PROGRAMS; index++) {
		if(os_checkAutostartProgram(os_lookupProgramID(os_programs[index]))) os_exec(os_lookupProgramID(os_programs[index])), DEFAULT_PRIORITY);
	}
}

/*!
 *  A simple getter for the slot of a specific process.
 *
 *  \param pid The processID of the process to be handled
 *  \return A pointer to the memory of the process at position pid in the os_processes array.
 */
Process* os_getProcessSlot(ProcessID pid) {
    return os_processes + pid;
}

/*!
 *  A simple getter for the slot of a specific program.
 *
 *  \param programID The ProgramID of the process to be handled
 *  \return A pointer to the function pointer of the program at position programID in the os_programs array.
 */
Program** os_getProgramSlot(ProgramID programID) {
    return os_programs + programID;
}

/*!
 *  A simple getter to retrieve the currently active process.
 *
 *  \return The process id of the currently active process.
 */
ProcessID os_getCurrentProc(void) {
    return currentProc;
}

/*!
 *  This function return the the number of currently active process-slots.
 *
 *  \returns The number currently active (not unused) process-slots.
 */
uint8_t os_getNumberOfActiveProcs(void) {
    uint8_t num = 0;

    ProcessID i = 0;
    do {
        num += os_getProcessSlot(i)->state != OS_PS_UNUSED;
    } while (++i < MAX_NUMBER_OF_PROCESSES);

    return num;
}

/*!
 *  This function returns the number of currently registered programs.
 *
 *  \returns The amount of currently registered programs.
 */
uint8_t os_getNumberOfRegisteredPrograms(void) {
    uint8_t i;
    for (i = 0; i < MAX_NUMBER_OF_PROGRAMS && *(os_getProgramSlot(i)); i++);
    // Note that this only works because programs cannot be unregistered.
    return i;
}

/*!
 *  Sets the current scheduling strategy.
 *
 *  \param strategy The strategy that will be used after the function finishes.
 */
void os_setSchedulingStrategy(SchedulingStrategy strategy) {
    currentSchedStrat = strategy;
	os_resetSchedulingInformation(strategy);
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
    return currentSchedStrat;
}

/*!
 *  Enters a critical code section by disabling the scheduler if needed.
 *  This function stores the nesting depth of critical sections of the current
 *  process (e.g. if a function with a critical section is called from another
 *  critical section) to ensure correct behavior when leaving the section.
 *  This function supports up to 255 nested critical sections.
 */
void os_enterCriticalSection(void) {
    // get global interrupt
    uint8_t interrupts = gbi(SREG, 7);
    	
    // disable global interrupt
    cbi(SREG, 7);
	
	criticalSectionCount++;
	
	cbi(TIMSK2, OCIE2A);
    	
	// reset global interrupt
	if (interrupts) {
		sbi(SREG, 7);
	}
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void) {
    // get global interrupt
    uint8_t interrupts = gbi(SREG, 7);
    
    // disable global interrupt
    cbi(SREG, 7);
    
    if (--criticalSectionCount == 0) {
		sbi(TIMSK2, OCIE2A);
	} else if (criticalSectionCount < 0) {
		os_error("Tried to leave critical section without entering it");
	}
    
    // reset global interrupt
    if (interrupts) {
	    sbi(SREG, 7);
    }
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
    StackChecksum sum = 0;
	for (StackPointer i = PROCESS_STACK_BOTTOM(pid); i.as_int > os_processes[pid]->sp.as_int; i.as_ptr--) {
		sum ^= *(i.as_ptr);
	}
	return sum;
}