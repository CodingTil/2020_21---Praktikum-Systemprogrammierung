//-------------------------------------------------
//          TestSuite: Multiple
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"

#include <avr/interrupt.h>

#define DELAY 1000

/*!
 * Program that spawns several instances of the same
 * program
 */
PROGRAM(1, AUTOSTART) {
    // Start 4 instances of program 2
    uint8_t x;
    for (x = 0; x < 12; x++) {
        cli();
        // Output identity
        lcd_writeChar(' ');
        lcd_writeChar('1');

        // Spawn a new process every third time
        if (!(x % 3)) {
            os_exec(2, DEFAULT_PRIORITY);
            lcd_writeChar('!');
            lcd_writeChar(';');
        }
        sei();
        delayMs(DELAY);
    }
    for (;;) {
        lcd_writeChar(' ');
        lcd_writeChar('1');
        delayMs(DELAY);
    }
}

PROGRAM(2, AUTOSTART) {
    cli();
    // Mark each instance with unique character
    static char siblings = 'a';
    char whoami = siblings;
    siblings++;
    sei();

    for (;;) {
        cli();
        // Output identity
        lcd_writeChar(' ');
        lcd_writeChar('2');
        lcd_writeChar(whoami);
        lcd_writeChar(';');
        sei();
        delayMs(DELAY);
    }
}
