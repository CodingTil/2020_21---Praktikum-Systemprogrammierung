#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_process.h"
#include "os_input.h"

PROGRAM(1, AUTOSTART) {
    while (1) {
        lcd_writeChar('A');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}

PROGRAM(2, AUTOSTART) {
    while (1) {
        lcd_writeChar('B');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}

PROGRAM(3, AUTOSTART) {
    while (1) {
        lcd_writeChar('C');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}

PROGRAM(4, DONTSTART) {
    while (1) {
        lcd_writeChar('D');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}
