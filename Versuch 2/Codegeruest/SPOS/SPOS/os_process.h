/*! \file
 *  \brief Struct specifying a process.
 *
 *  Contains the struct that bundles all properties of a running process.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef _OS_PROCESS_H
#define _OS_PROCESS_H

#include <stdint.h>
#include <stdbool.h>

//! The type for the ID of a running process.
#warning IMPLEMENT STH. HERE
typedef ? ProcessID;

//! The type for the ID of a program.
#warning IMPLEMENT STH. HERE
typedef ? ProgramID;

//! The type of the priority of a process.
typedef uint8_t Priority;

//! The age of a process (scheduler specific property).
typedef uint16_t Age;

//! The type for the checksum used to check stack consistency.
typedef uint8_t StackChecksum;

//! Type for the state a specific process is currently in.
typedef enum ProcessState {
    OS_PS_UNUSED,
    OS_PS_READY,
    OS_PS_RUNNING,
    OS_PS_BLOCKED
} ProcessState;

//! A union that holds the current stack pointer of a given process.
//! We use a union so we can reduce the number of explicit casts.
typedef union StackPointer {
    uint16_t as_int;
    uint8_t* as_ptr;
} StackPointer;

/*!
 *  The struct that holds all information for a process.
 *  Note that additional scheduling information (such as the current time-slice)
 *  are stored by the module that implements the actual scheduling strategies.
 */
#warning IMPLEMENT STH. HERE
typedef struct {
    ?
} Process;

//! This is the type of a program function (not the pointer to one!).
#warning IMPLEMENT STH. HERE
typedef ? (Program)(?);

//! Specifies if a program should be automatically executed on boot-up.
typedef enum {
    DONTSTART,
    AUTOSTART
} OnStartDo;

/*!
 *  Defines a program function with the name prog0, prog1, prog2, ...
 *  depending on the numerical index you pass as the first macro-parameter.
 *  Note that this will break, if you pass a non-trivial expression as index,
 *  e.g. PROGRAMM(1+1, ...) will not have the desired effect, but will in fact
 *  not even compile. Make sure to always pass natural numbers as first argument.
 *  The second macro parameter specifies whether you want this program to be
 *  auto-magically executed when the system boots.
 *  If you pass 'AUTOSTART', it will create a process for this program while
 *  initializing the scheduler. If you pass 'DONTSTART' instead, only the
 *  program will be registered (which you may execute manually).
 *  Use this macro in this fashion:
 *
 *    PROGRAM(3, AUTOSTART) {
 *      foo();
 *      bar();
 *      ...
 *    }
 */
#define PROGRAM(INDEX, ON_START_DO) \
    void program_with_index_##INDEX##_defined_twice (void) {} \
    Program prog##INDEX; \
    void registerProgram##INDEX(void) __attribute__ ((constructor)); \
    void registerProgram##INDEX(void) { \
        Program** os_getProgramSlot(ProgramID progId); \
        *(os_getProgramSlot(INDEX)) = prog##INDEX; \
        extern uint16_t os_autostart;\
        os_autostart |= (ON_START_DO == AUTOSTART) << (INDEX); \
    } \
    void prog##INDEX(void)

//! Returns whether the passed process can be selected to run.
bool os_isRunnable(Process const* process);

#endif
