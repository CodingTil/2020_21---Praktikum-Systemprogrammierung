//-------------------------------------------------
//          TestSuite: Resume
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"

#include <avr/interrupt.h>

#define DELAY 500

uint8_t volatile i = 0;
uint8_t volatile iFlag = 0;

/*!
 * Prog 1 and 3 take turns printing.
 * Doing some simple communication
 */
PROGRAM(1, AUTOSTART) {
    for (;;) {
        while (iFlag);
        lcd_writeChar('0' + i);
        iFlag = 1;
        delayMs(DELAY);
    }
}

/*!
 * Program that prints Characters in ascending order
 */
PROGRAM(2, AUTOSTART) {
    // Declare register bound variable to test recovery of register values after scheduling
    register uint8_t j = 0;
    for (;;) {
        lcd_writeChar('a' + j);
        j = (j + 1) % ('z' - 'a' + 1);
        delayMs(DELAY);
    }
}

/*!
 * Prog 1 and 3 take turns printing.
 * Doing some simple communication.
 */
PROGRAM(3, AUTOSTART) {
    uint8_t k;
    for (;;) {
        if (iFlag) {
            k = i + 1;
            i = k % 10;
            iFlag = 0;
            lcd_writeChar(' ');
        }
        delayMs(DELAY);
    }
}
