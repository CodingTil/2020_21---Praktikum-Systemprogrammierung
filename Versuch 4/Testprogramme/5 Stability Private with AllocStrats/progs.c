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

//! Function for managing ongoing lcd output
void writeChar(char c) {
    static unsigned char pos = 16;
    os_enterCriticalSection();
    if (++pos > 16) {
        pos = 1;
        lcd_line2();
        lcd_writeProgString(PSTR("                "));
    }
    lcd_goto(2, pos);
    lcd_writeChar(c);
    os_leaveCriticalSection();
}

//! Prints process id and phase using thread-safe write-char
void printPhase(uint8_t id, char phase) {
    os_enterCriticalSection();
    writeChar(id + '0');
    writeChar(phase);
    writeChar(' ');
    os_leaveCriticalSection();
}

#define PROCESS_NUMBER 5
#define SZ 6

void makeCheck(uint8_t id, uint16_t mod, MemValue pat[SZ]) {
    uint8_t i, j;
    uint16_t sizes[SZ * 2];
    MemAddr sizedChunks[SZ * 2];

    // Alloc chunks and test if they are within the valid range.
    // Then write the corresponding number from the pattern into them.
    printPhase(id, 'a');
    for (i = 0; i < SZ; i++) { // Chunk
        for (j = 0; j < 2; j++) { // Heap
            Heap* heap = os_lookupHeap(j);
            sizes[i + SZ * j] = (TCNT0 % (mod * (1 + j))) + 1; // Get sizes from timer counter
            sizedChunks[i + SZ * j] = os_malloc(heap, sizes[i + SZ * j]);
            if (sizedChunks[i + SZ * j] < os_getUseStart(heap)) {
                os_error("Address too small");
            }
            if (sizedChunks[i + SZ * j] + sizes[i + SZ * j] > (os_getUseStart(heap) + os_getUseSize(heap))) { // this is NO off-by-one!
                os_error("Address too large");
            }
            uint16_t k;
            for (k = 0; k < sizes[i + SZ * j]; k++) {
                heap->driver->write(sizedChunks[i + SZ * j] + k, pat[i]);
            }
        }
    }

    // Just...wait.
    printPhase(id, 'b');
    delayMs(10 * DELAY);

    // Are the correct numbers still contained in the right chunks?
    printPhase(id, 'c');
    for (i = 0; i < SZ; i++) {
        for (j = 0; j < 2; j++) {
            uint16_t k;
            for (k = 0; k < sizes[i + SZ * j]; k++)
                if (os_lookupHeap(j)->driver->read(sizedChunks[i + SZ * j] + k) != pat[i]) {
                    j == 0 ? os_error("Pattern mismatch   internal !") : os_error("Pattern mismatch   external !");
                }
        }
    }
    delayMs(1 * DELAY);

    // Free all chunks.
    printPhase(id, 'd');
    for (i = 0; i < SZ; i++) {
        for (j = 0; j < 2; j++) {
            os_free(os_lookupHeap(j), sizedChunks[i + SZ * j]);
        }
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
    uint8_t checkCount = 0, strategyId = 0;
    struct {
        AllocStrategy strat;
        char name;
    } cycle[] = {
        {
            .strat = OS_MEM_FIRST,
            .name = 'f'
        },
        {
            .strat = OS_MEM_NEXT,
            .name = 'n'
        },
        {
            .strat = OS_MEM_BEST,
            .name = 'b'
        },
        {
            .strat = OS_MEM_WORST,
            .name = 'w'
        }
    };
    uint8_t const cycleSize = sizeof(cycle) / sizeof(*cycle);
    for (;;) {
        // One process changes the strategies continually
        if (me == 1 && !(checkCount++ % 8)) {
            os_enterCriticalSection();
            lcd_clear();
            delayMs(2 * DELAY);
            lcd_writeProgString(PSTR("Change to "));
            lcd_writeChar(cycle[strategyId].name);
            delayMs(5 * DELAY);
            os_setAllocationStrategy(os_lookupHeap(0), cycle[strategyId].strat);
            os_setAllocationStrategy(os_lookupHeap(1), cycle[strategyId].strat);
            delayMs(2 * DELAY);
            lcd_clear();
            strategyId = (strategyId + 1) % cycleSize;
            os_leaveCriticalSection();
        }

        makeCheck(me, me * 5, pat);
    }
}