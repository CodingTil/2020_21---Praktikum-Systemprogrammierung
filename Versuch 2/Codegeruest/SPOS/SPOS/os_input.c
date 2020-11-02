#include "os_input.h"

#include <avr/io.h>
#include <stdint.h>

/*! \file

Everything that is necessary to get the input from the Buttons in a clean format.

*/

/*!
 *  A simple "Getter"-Function for the Buttons on the evaluation board.\n
 *
 *  \returns The state of the button(s) in the lower bits of the return value.\n
 *  example: 1 Button:  -pushed:   00000001
 *                      -released: 00000000
 *           4 Buttons: 1,3,4 -pushed: 000001101
 *
 */
uint8_t os_getInput(void) {
	return (~PINC & 0b11000011);
}

/*!
 *  Initializes DDR and PORT for input
 */
void os_initInput() {
    DDRC &= 0b00111100;
	PORTC |= 0b11000011;
}

void os_waitForNoInput() {
    while ((PINC & 0b11000011) != 0b11000011);
}

/*!
 *  Endless loop until at least one button is pressed.
 */
void os_waitForInput() {
    while ((PINC & 0b11000011) == 0b11000011);
}
