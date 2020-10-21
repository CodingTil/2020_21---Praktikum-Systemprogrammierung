//-------------------------------------------------
//          TestSuite: Stability Private
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"

#include <avr/interrupt.h>

#define DELAY 100

//! This program prints the current time in the first line.
PROGRAM(4, AUTOSTART) {
    for (;;) {
        os_enterCriticalSection();
        lcd_line1();
        lcd_writeProgString(PSTR("Time: ")); // +6
        uint16_t const msecs = (os_coarseSystemTime * 33 / 10) % 1000;
        uint16_t const secs = os_coarseSystemTime / 300;
        uint16_t const mins = secs / 100;
        if (mins < 100) {
            lcd_writeDec((secs / 60) % 100); // +2
            lcd_writeChar('m'); // +1
            lcd_writeChar(' '); // +1
            lcd_writeDec(secs % 60); // +2
            lcd_writeChar('.'); // +1
            lcd_writeDec(msecs / 100); // +2
            lcd_writeChar('s'); // +1
            lcd_writeChar(' '); // +1
        } else {
            lcd_writeProgString(PSTR("> 100 min"));
        }
        os_leaveCriticalSection();
        delayMs(100);
    }
}

//! Writes characters consecutively into the second line of the LCD
void writeChar(char c) {
    static unsigned char pos = 16;
    os_enterCriticalSection();
    // Clear line if full
    if (++pos > 16) {
        pos = 1;
        lcd_line2();
        lcd_writeProgString(PSTR("                "));
    }
    lcd_goto(2, pos);
    lcd_writeChar(c);
    os_leaveCriticalSection();
}

//! Prints the status of the process
void printPhase(uint8_t id, char phase) {
    os_enterCriticalSection();
    writeChar(id + '0');
    writeChar(phase);
    writeChar(' ');
    os_leaveCriticalSection();
}

#define PROCESS_NUMBER 3
#define SZ 6

#define DRIVER intHeap

void makeCheck(uint8_t id, uint16_t mod, MemValue pat[SZ]) {
    uint8_t i;
    uint16_t sizes[SZ];
    MemAddr ps[SZ];

    // Alloc chunks and test if they are within the valid range.
    // Then write the corresponding number from the pattern into them.
    printPhase(id, 'a');
    for (i = 0; i < SZ; i++) {
        sizes[i] = (TCNT0 % mod) + 1; // Get sizes from timer counter
        ps[i] = os_malloc(DRIVER, sizes[i]);
        if (ps[i] < os_getUseStart(DRIVER)) {
            os_error("Address too small");
        }
        if (ps[i] + sizes[i] > (os_getUseStart(DRIVER) + os_getUseSize(DRIVER))) { // This is NO off-by-one!
            os_error("Address too large");
        }
        uint16_t j;
        for (j = 0; j < sizes[i]; j++) {
            DRIVER->driver->write(ps[i] + j, pat[i]);
        }
    }

    // Just...wait.
    printPhase(id, 'b');
    delayMs(10 * DELAY);

    // Are the correct numbers still contained in the right chunks?
    printPhase(id, 'c');
    for (i = 0; i < SZ; i++) {
        uint16_t j;
        for (j = 0; j < sizes[i]; j++)
            if (DRIVER->driver->read(ps[i] + j) != pat[i]) {
                os_error("Pattern mismatch");
            }
    }
    delayMs(1 * DELAY);

    // Free all chunks.
    printPhase(id, 'd');
    for (i = 0; i < SZ; i++) {
        os_free(DRIVER, ps[i]);
    }
}

//! Main test program.
PROGRAM(1, AUTOSTART) {

    // First spawn consecutively numbered instances of this program.
    // Having increasing priority.
    static uint8_t count = 0;
    uint8_t me = ++count;
    if (me < PROCESS_NUMBER) {
        os_exec(1, me * 20);
    }

    // Create a characteristic pattern (number sequence) for each instance.
    MemValue pat[SZ];
    uint8_t i;
    for (i = 0; i < SZ; i++) {
        pat[i] = me * SZ + i;
    }


    // Prepare AllocStrats and cycle through them endlessly while
    // doing stability checks.
    // Here it's only one strategy.
    uint8_t checkCount = 0, strategyId = 0;
    struct {
        AllocStrategy strat;
        char name;
    } cycle[] = {
        {
            .strat = OS_MEM_FIRST,
            .name = 'f'
        },
    };
    uint8_t const cycleSize = sizeof(cycle) / sizeof(*cycle);
    for (;;) {
        // One process changes the allocation strategy periodically
        if (me == 1 && !(checkCount++ % 8)) {
            os_enterCriticalSection();
            lcd_clear();
            delayMs(2 * DELAY);
            lcd_writeProgString(PSTR("Change to "));
            lcd_writeChar(cycle[strategyId % cycleSize].name);
            delayMs(5 * DELAY);
            os_setAllocationStrategy(DRIVER, cycle[strategyId++ % cycleSize].strat);
            delayMs(2 * DELAY);
            lcd_clear();
            os_leaveCriticalSection();
        }

        makeCheck(me, me * 5, pat);
    }
}