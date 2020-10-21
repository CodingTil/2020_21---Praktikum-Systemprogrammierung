//------------------------------------------------------------
//          TestSuite: Alloc Strategies
//------------------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_memory.h"
#include "os_scheduler.h"
#include "os_input.h"

#include <stdlib.h>

#define DELAY 100

#define DRIVER intHeap
/*
 * SELECT HERE!
 * Choose Alloc.Strats, which you want to be tested (1 = will be tested, 0 = wont be tested)
 */
#define FIRST   1
#define NEXT    1
#define BEST    1
#define WORST   1

PROGRAM(1, AUTOSTART) {

    // Struct-Array with Alloc.Strats
    struct {
        AllocStrategy strat;
        char name;
        uint8_t check;
    } cycle[] = {
        {
            .strat = OS_MEM_FIRST,
            .name = 'f',
            .check = FIRST
        }, {
            .strat = OS_MEM_NEXT,
            .name = 'n',
            .check = NEXT
        }, {
            .strat = OS_MEM_BEST,
            .name = 'b',
            .check = BEST
        }, {
            .strat = OS_MEM_WORST,
            .name = 'w',
            .check = WORST
        }
    };

    /*
     * Create following pattern in memory: first big, second small, rest huge
     * X = allocated
     * __________________________________
     * |   |   |   | X |   |   | X |   | ...
     * 0   5   10  15  20  25  30  35
     *
     * malloc and free must be working correctly
     */
    MemAddr p[7];
    uint8_t s[] = {15, 5, 10, 5, 1};
    uint8_t lastStrategy;
    uint8_t error = 0;

    // Precheck heap size
    if (os_getUseSize(DRIVER) < 50) {
        os_error("Heap too small");
    }

    os_enterCriticalSection();
    lcd_clear();
    lcd_writeProgString(PSTR("Check strategy.."));
    delayMs(10 * DELAY);
    lcd_clear();
    os_leaveCriticalSection();

    uint16_t i;
    uint16_t start = os_getMapStart(DRIVER);

    // Check if map is clean
    for (i = 0; i < os_getMapSize(DRIVER); i++) {
        if (DRIVER->driver->read(start + i)) {
            os_enterCriticalSection();
            lcd_clear();
            lcd_writeProgString(PSTR("Map not free"));
            while (1);
        }
    }

    // Check overalloc for all strategies
    for (uint8_t strategy = 0; strategy < 4; strategy++) {
        if (!cycle[strategy].check) {
            continue;
        }
        os_setAllocationStrategy(DRIVER, cycle[strategy].strat);
        if (os_malloc(DRIVER, os_getUseSize(DRIVER) + 1) != 0) {
            lcd_clear();
            lcd_writeProgString(PSTR("Overalloc"));
            lcd_line2();
            lcd_writeChar(cycle[strategy].name);
            while (1);
        }
    }

    // The test for next fit depends on not creating the memory pattern with OS_MEM_NEXT
    os_setAllocationStrategy(DRIVER, OS_MEM_FIRST);
    // Create pattern in memory
    for (i = 0; i < 5; i++) {
        p[i] = os_malloc(DRIVER, s[i]);
    }
    for (i = 0; i <= 4; i += 2) {
        os_free(DRIVER, p[i]);
    }

    // Check strategies
    for (lastStrategy = 0; lastStrategy < 4; lastStrategy++) {

        if (!cycle[lastStrategy].check) {
            continue;
        }

        lcd_clear();
        lcd_writeProgString(PSTR("Checking strat.\n"));
        lcd_writeChar(cycle[lastStrategy].name);
        os_setAllocationStrategy(DRIVER, cycle[lastStrategy].strat);
        delayMs(10 * DELAY);
        lcd_clear();

        /*
         * We allocate, and directly free two times (except for BestFit)
         * for NextFit we should get different addresses
         * for FirstFit, both addresses are equal to first segment
         * for BestFit, first address equals second segment and second address equals first segment
         * for WorstFit we get the first byte of the rest memory
         * otherwise we found an error
         */
        for (i = 5; i < 7; i++) {
            p[i] = os_malloc(DRIVER, s[2]);
            if (cycle[lastStrategy].strat != OS_MEM_BEST) {
                os_free(DRIVER, p[i]);
            }
        }


        // NextFit
        if (cycle[lastStrategy].strat == OS_MEM_NEXT) {
            if (p[5] != p[0] || p[6] != p[2]) {
                lcd_writeProgString(PSTR("Error NextFit"));
                delayMs(10 * DELAY);
                error = 1;
            } else {
                lcd_writeProgString(PSTR("NextFit Ok"));
            }
            delayMs(10 * DELAY);
        }

        // FirstFit
        else if (cycle[lastStrategy].strat == OS_MEM_FIRST) {
            if (p[5] != p[6] || p[5] != p[0]) {
                lcd_writeProgString(PSTR("Error FirstFit"));
                delayMs(10 * DELAY);
                error = 1;
            } else {
                lcd_writeProgString(PSTR("FirstFit Ok"));
            }
            delayMs(10 * DELAY);
        }
        // BestFit
        else if (cycle[lastStrategy].strat == OS_MEM_BEST) {
            if (p[5] != p[2] || p[6] != p[0]) {
                lcd_writeProgString(PSTR("Error BestFit"));
                delayMs(10 * DELAY);
                error = 1;
            } else {
                lcd_writeProgString(PSTR("BestFit Ok"));
            }
            delayMs(10 * DELAY);

            // Free manually as it wasn't done before
            if (p[5]) {
                os_free(DRIVER, p[5]);
            }
            if (p[6]) {
                os_free(DRIVER, p[6]);
            }
        }
        // WorstFit
        else if (cycle[lastStrategy].strat == OS_MEM_WORST) {
            if (p[4] != p[5] || p[5] != p[6]) {
                lcd_writeProgString(PSTR("Error WorstFit"));
                delayMs(10 * DELAY);
                error = 1;
            } else {
                lcd_writeProgString(PSTR("WorstFit Ok"));
            }
            delayMs(10 * DELAY);
        } else {
            lcd_writeChar('e');
        }
    }

    // Remove pattern
    for (i = 1; i <= 3; i += 2) {
        os_free(DRIVER, p[i]);
    }

    // Check if map is clean
    for (i = 0; i < os_getMapSize(DRIVER); i++) {
        if (DRIVER->driver->read(start + i)) {
            os_enterCriticalSection();
            lcd_clear();
            lcd_writeProgString(PSTR("Map not free afterwards"));
            while (1);
        }
    }

    // Special NextFit test
    if (NEXT) {
        lcd_clear();
        lcd_writeProgString(PSTR("Special NextFit test"));
        delayMs(10 * DELAY);
        lcd_clear();
        os_setAllocationStrategy(DRIVER, OS_MEM_NEXT);
        size_t rema = os_getUseSize(DRIVER);
        for (i = 0; i < 5; i++) {
            rema -= s[i];
        }
        p[0] = os_malloc(DRIVER, rema);
        os_free(DRIVER, p[0]);
        for (i = 0; i < 3; i++) {
            p[i] = os_malloc(DRIVER, s[i]);
        }
        rema = os_getUseSize(DRIVER) - (s[0] + s[1] + s[2]);
        os_malloc(DRIVER, rema);
        for (i = 1; i < 3; i++) {
            os_free(DRIVER, p[i]);
        }
        p[1] = os_malloc(DRIVER, s[1]);
        os_free(DRIVER, p[1]);
        if (!os_malloc(DRIVER, s[1] + s[2])) {
            os_enterCriticalSection();
            lcd_clear();
            lcd_writeProgString(PSTR("Error Next Fit (special)"));
            delayMs(10 * DELAY);
            error = 1;
        } else {
            lcd_writeProgString(PSTR("Ok"));
            delayMs(10 * DELAY);
        }
    }

    lcd_clear();
    if (!error) {
        lcd_writeProgString(PSTR("   All passed"));
    } else {
        lcd_writeProgString(PSTR("Check failed"));
        while (1);
    }
	lcd_line2();
	lcd_writeProgString(PSTR(" PLEASE CONFIRM!"));
	os_waitForInput();
	os_waitForNoInput();
	lcd_clear();
	lcd_writeProgString(PSTR("WAITING FOR"));
	lcd_line2();
	lcd_writeProgString(PSTR("TERMINATION"));
	delayMs(1000);
}
