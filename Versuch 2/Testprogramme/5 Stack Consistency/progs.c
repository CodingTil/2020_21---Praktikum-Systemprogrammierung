//-------------------------------------------------
//          TestSuite: Stack Consistency
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"

#include <avr/interrupt.h>
#include <stdint.h>

#define DELAY 100

/*!
 * Function that is called by both processes. Only one process proceeds and writes
 * into the other's stack. The other one stays traped in an infinite loop.
 *
 * \param mine points to the address of a variable on the stack of this process
 * \param his points to the address of a variable on the stack of the other process
 * \param dummy points to the variable of the process that has called this function
 */
void dumper(uint16_t* mine, uint16_t volatile const* his, uint8_t const* dummy) {
    // Initialize process specific pointer
    os_enterCriticalSection();
    *mine = (uint16_t)dummy;
    os_leaveCriticalSection();

    // Wait for other process
    while (!*his);

    // Only one process may continue
    if (*mine < *his) {
        for (;;);
    }
    // Print which one did continue
    lcd_writeChar(*dummy);
    lcd_writeChar(':');
    lcd_writeChar(' ');

    // Compute the number of bytes in between and reduce it by some.
    unsigned i = *mine - *his - 24;

    // Now iterate up and flip the bits of the chosen byte till the other stack gets hit.
    // That should cause a stack inconsistency.
    while (++i) {
        lcd_writeDec(i);
        {
            // Choose byte using an array of the right size.
            uint8_t arr[i];

            // Flip bits
            *arr ^= 0xFF;
        }
        lcd_writeChar(';');
        lcd_writeChar(' ');
        delayMs(3 * DELAY);
    }

    /* 
	 * If no inconsistency is reported it is considered broken. This will most likely
     * never happen because "i" is of type uint16_t with a max. value of 65535.
     * The code will just crash and start rebooting SPOS, which is acceptable at this
     * point because the inconsistency should've been detected long ago.
     */
    os_error("Consistency check broken");
}

// Variables to store addresses to the processe's stack variable
uint16_t p1 = 0;
uint16_t p2 = 0;

PROGRAM(1, AUTOSTART) {
    uint8_t i = '1';
    dumper(&p1, &p2, &i);
}

PROGRAM(2, AUTOSTART) {
    uint8_t i = '2';
    dumper(&p2, &p1, &i);
}
