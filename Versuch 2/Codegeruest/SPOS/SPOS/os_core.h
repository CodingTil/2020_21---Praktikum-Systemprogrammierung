/*! \file
 *  \brief Core of the OS.
 *
 *  Contains functionality to for core OS operations.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef _OS_CORE_H
#define _OS_CORE_H

#include <avr/pgmspace.h>

//! Handy define to specify error messages directly
#define os_error(str) os_errorPStr(PSTR(str))

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------

//! Initializes timers
void os_init_timer(void);

//! Initializes OS
void os_init(void);

//! Shows error on display and terminates program
void os_errorPStr(char const* str);

#endif
