/*! \file
 *  \brief Input library for the OS.
 *
 *  Contains functionalities to read user input.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------


//! Refreshes the button states
uint8_t getInput(void);

//! Initializes PIN and PORT for input
void initInput(void);

//! Waits for all buttons to be released
void waitForNoInput(void);

//! Waits for at least one button to be pressed
void waitForInput(void);

//! Tests the button implementation
void buttonTest(void);

#endif
