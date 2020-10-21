//-------------------------------------------------
//          TestSuite: Shared Access
//-------------------------------------------------

#include <avr/interrupt.h>

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"
#include "os_input.h"

#define NUM_MAL 5
#define SIZE 10

#if (NUM_MAL * SIZE > 255)
    #error Reduce SIZE or NUM_MAL
#endif
#if (SIZE < 4)
    #error Chunks must at least be 4 Bytes
#endif

#define DELAY 100
#define DRIVER intHeap

uint8_t errors = 0;
MemAddr shBlock;
MemAddr shBlockYield;
volatile uint8_t state = 0;
volatile uint8_t mode = 0;

PROGRAM(1, AUTOSTART) {
    MemAddr allocs[NUM_MAL];
    volatile int i, j;
    MemAddr opBlock1, opBlock2;

    // Allocate some shared memory
    lcd_writeProgString(PSTR("1: Allocating..."));
    delayMs(10 * DELAY);
    for (i = 0; i < NUM_MAL; i++) {
        lcd_clear();
        lcd_writeDec(i);
        allocs[i] = os_sh_malloc(DRIVER, 10);

        // Assure that the allocated memory chunks are pairwise distinct
        bool different = true;
        for (j = 0; different && j < i; j++) {
            different = allocs[j] != allocs[i];
        }
        lcd_writeChar(' ');
        if (different) {
            lcd_writeProgString(PSTR("OK"));
        } else {
            errors++;
            os_error("FAILURE");
        }
        delayMs(1 * DELAY);
    }
    lcd_clear();

    // Test if locks can be opened and closed.
    lcd_writeProgString(PSTR("2a: Accessing (open/close)..."));
    delayMs(10 * DELAY);
    for (i = 0; i < NUM_MAL; i++) {
        lcd_clear();
        lcd_writeDec(i);
        lcd_writeChar(' ');
        for (j = 0; j < SIZE; j++) {
            MemAddr const p = allocs[i] + j;
            os_sh_writeOpen(DRIVER, &p);
            os_sh_close(DRIVER, p);
            os_sh_readOpen(DRIVER, &p);
            os_sh_close(DRIVER, p);
        }
        lcd_writeProgString(PSTR("OK"));
        delayMs(1 * DELAY);
    }
    lcd_clear();

    // Test if locks can read on private memory
    lcd_writeProgString(PSTR("2b: Read on private..."));
    delayMs(10 * DELAY);
    {
        MemAddr privBlock = os_malloc(DRIVER, 10);
        os_sh_readOpen(DRIVER, &privBlock);
        os_free(DRIVER, privBlock);
        delayMs(5 * DELAY);
        lcd_writeProgString(PSTR("OK (if error)"));
        delayMs(5 * DELAY);
    }
    lcd_clear();

    // Test if locks can write on private memory
    lcd_writeProgString(PSTR("2c: Write on private..."));
    delayMs(10 * DELAY);
    {
        MemAddr privBlock = os_malloc(DRIVER, 10);
        os_sh_writeOpen(DRIVER, &privBlock);
        os_free(DRIVER, privBlock);
        delayMs(5 * DELAY);
        lcd_writeProgString(PSTR("OK (if error)"));
        delayMs(5 * DELAY);
    }
    lcd_clear();

    // Do some simple write operations
    lcd_writeProgString(PSTR("3: Accessing (write)..."));
    delayMs(10 * DELAY);
    for (i = 0; i < NUM_MAL; i++) {
        lcd_clear();
        lcd_writeDec(i);
        lcd_writeChar(' ');
        for (j = 0; j < SIZE; j++) {
            MemValue const a = i * SIZE + j;
            MemAddr const p = allocs[i];
            os_sh_write(DRIVER, &p, j, &a, 1);
        }
        lcd_writeProgString(PSTR("OK"));
        delayMs(1 * DELAY);
    }
    lcd_clear();

    // Read the written pattern and test if it's still the same
    lcd_writeProgString(PSTR("4: Accessing (read)..."));
    delayMs(10 * DELAY);
    for (i = 0; i < NUM_MAL; i++) {
        lcd_clear();
        lcd_writeDec(i);
        lcd_writeChar(' ');
        MemValue a;
        for (j = 0; j < SIZE; j++) {
            MemAddr const p = allocs[i];
            a = (MemValue) - 1;
            os_sh_read(DRIVER, &p, j, &a, 1);
            if (a != i * SIZE + j) {
                cli();
                lcd_clear();
                lcd_writeDec(i);
                lcd_writeChar(' ');
                lcd_writeProgString(PSTR("FAILURE @ "));
                lcd_writeDec(j);
                lcd_writeChar('/');
                lcd_writeDec(SIZE);
                errors++;
                // Can't use os_error here since this is no static error message
                while (os_getInput() != 0b1001);
                os_waitForNoInput();
                lcd_clear();
                sei();
            }
        }
        lcd_writeProgString(PSTR("OK"));
        delayMs(1 * DELAY);
    }
    lcd_clear();

    // Now read past the end of the allocated memory to provoke an error
    lcd_writeProgString(PSTR("5: Provoking viol. (read)..."));
    delayMs(10 * DELAY);
    {
        lcd_clear();
        lcd_writeChar('1');
        lcd_writeChar(' ');
        MemAddr const p = allocs[1] + 1;
        uint32_t a;
        os_sh_read(DRIVER, &p, SIZE - 3, (MemValue*)&a, 4);
        delayMs(5 * DELAY);
        lcd_writeProgString(PSTR("OK (if error)"));
        delayMs(5 * DELAY);
    }
    lcd_clear();

    // As above just with writing
    lcd_writeProgString(PSTR("6: Provoking viol. (write)..."));
    delayMs(10 * DELAY);
    {
        lcd_clear();
        lcd_writeChar('1');
        lcd_writeChar(' ');
        MemAddr const p = allocs[1] + 2;
        MemValue const a = 0xFF;
        os_sh_write(DRIVER, &p, SIZE, &a, 1);
        delayMs(5 * DELAY);
        lcd_writeProgString(PSTR("OK (if error)"));
        delayMs(5 * DELAY);
    }
    lcd_clear();

    // Now check if the write operation was executed despite the access violation
    lcd_writeProgString(PSTR("7: Checking..."));
    delayMs(10 * DELAY);
    {
        MemAddr const p = allocs[2];
        MemValue a = 0xBB;
        os_sh_read(DRIVER, &p, 0, &a, 1);
        delayMs(2 * DELAY);
        if (a == 2 * SIZE + 0) {
            lcd_clear();
            lcd_writeProgString(PSTR("OK"));
        } else {
            errors++;
            os_error("FAILURE @ Checking");
        }
        delayMs(10 * DELAY);
    }
    lcd_clear();

    // Check parallel reading
    lcd_writeProgString(PSTR("8: Multiple access..."));
    delayMs(10 * DELAY);
    {
        shBlock = os_sh_malloc(DRIVER, 10);
        if (shBlock == 0) {
            errors++;
            os_error("Not enough memory");
        }
        shBlockYield = os_sh_malloc(DRIVER, 10);
        if (shBlockYield == 0) {
            errors++;
            os_error("Not enough memory");
        }

        lcd_clear();
        lcd_writeProgString(PSTR("readOpen Yield"));
        delayMs(10 * DELAY);
        ProcessID readOpenProc = os_exec(3, DEFAULT_PRIORITY);
        delayMs(10 * DELAY);
        os_kill(readOpenProc);

        lcd_clear();
        lcd_writeProgString(PSTR("write/readOpen  Yield"));
        mode = 0;
        delayMs(10 * DELAY);
        ProcessID writeReadOpenProc = os_exec(4, DEFAULT_PRIORITY);
        delayMs(10 * DELAY);
        os_kill(writeReadOpenProc);

        lcd_clear();
        lcd_writeProgString(PSTR("read/writeOpen  Yield"));
        mode = 1;
        delayMs(10 * DELAY);
        ProcessID readWriteOpenProc = os_exec(4, DEFAULT_PRIORITY);
        delayMs(10 * DELAY);
        os_kill(readWriteOpenProc);

        // Open the block twice (-> test parallel reading)
        opBlock1 = os_sh_readOpen(DRIVER, &shBlock);
        if (opBlock1 != shBlock) {
            errors++;
            os_error("Address shouldn't have changed (1)");
        }

        lcd_clear();
        lcd_writeProgString(PSTR("1x readOpen"));
        delayMs(10 * DELAY);

        opBlock2 = os_sh_readOpen(DRIVER, &shBlock);
        if (opBlock2 != shBlock) {
            errors++;
            os_error("Address shouldn't have changed (2)");
        }

        lcd_clear();
        lcd_writeProgString(PSTR("2x readOpen"));
        delayMs(10 * DELAY);


        os_sh_close(DRIVER, opBlock2);

        lcd_clear();
        lcd_writeProgString(PSTR("OK"));
        delayMs(10 * DELAY);
    }
    lcd_clear();

    // Check read/write barriers
    lcd_writeProgString(PSTR("9: Checking RW barriers..."));
    delayMs(10 * DELAY);
    {
        // Read before write (opBlock1 is still read-open here!):
        lcd_clear();
        lcd_writeProgString(PSTR("Read before\nwrite"));
        delayMs(10 * DELAY);

        // Set behavior of program 2: readOpen
        mode = 1;
        os_exec(2, DEFAULT_PRIORITY);
        // Grant process at least one time slot
        while (state == 0);
        delayMs(5 * DELAY);
        os_kill(2);
        state = 0;
        os_sh_close(DRIVER, opBlock1);

        lcd_clear();
        lcd_writeProgString(PSTR("OK"));
        delayMs(5 * DELAY);

        // Write before read:
        lcd_clear();
        lcd_writeProgString(PSTR("Write before\nread"));
        delayMs(10 * DELAY);

        opBlock1 = os_sh_writeOpen(DRIVER, &shBlock);
        if (opBlock1 != shBlock) {
            errors++;
            os_error("Address shouldn't have changed (3)");
        }

        // Set behavior of program 2: writeOpen
        mode = 2;
        os_exec(2, DEFAULT_PRIORITY);
        // Grant process at least one time slot
        while (state == 0);
        delayMs(5 * DELAY);
        os_kill(2);
        state = 0;

        lcd_clear();
        lcd_writeProgString(PSTR("OK"));
        delayMs(5 * DELAY);

        // Write before write:
        lcd_clear();
        lcd_writeProgString(PSTR("Write before\nwrite"));
        delayMs(10 * DELAY);

        // Set behavior of program 2: readOpen
        mode = 3;
        os_exec(2, DEFAULT_PRIORITY);
        // Grant process at least one time slot
        while (state == 0);
        delayMs(5 * DELAY);
        os_kill(2);
        state = 0;
        os_sh_close(DRIVER, opBlock1);

        lcd_clear();
        lcd_writeProgString(PSTR("OK"));
        delayMs(10 * DELAY);
    }
    lcd_clear();


    // Check offset
    lcd_writeProgString(PSTR("10: Checking offset..."));
    delayMs(10 * DELAY);
    {
        MemValue value = 0;
        MemValue pat[10];

        // Write pattern (opBlock1 is still write-open here!):
        for (i = 0; i < 10; i++) {
            MemValue const a = i + 1;
            os_sh_write(DRIVER, &shBlock, i, &a, 1);
        }

        // Read pattern without offset
        // The read pattern should always be the first one
        for (i = 0; i < 10; i++) {
            opBlock1 = shBlock + i;
            os_sh_read(DRIVER, &opBlock1, 0, &value, 1);
            if (value != 1) {
                errors++;
                os_error("Pattern mismatch (1)");
            }
        }

        // Read pattern one by one with offset
        for (i = 0; i < 10; i++) {
            os_sh_read(DRIVER, &shBlock, i, &value, 1);
            if (value != i + 1) {
                errors++;
                os_error("Pattern mismatch (2)");
            }
        }

        // Read pattern all at once without offset
        os_sh_read(DRIVER, &shBlock, 0, (MemValue*)&pat, 10);
        for (i = 0; i < 10; i++) {
            if (pat[i] != i + 1) {
                errors++;
                os_error("Pattern mismatch (3)");
            }
        }

        // Read pattern-blocks with false and correct offset
        for (i = 0; i < 10; i++) {
            opBlock1 = shBlock + i;
            os_sh_read(DRIVER, &opBlock1, i, (MemValue*)&pat, 10 - i);

            for (j = 0; j < 10 - i; j++)
                if (pat[j] != i + j + 1) {
                    errors++;
                    os_error("Pattern mismatch (4)");
                }
        }

        lcd_clear();
        lcd_writeProgString(PSTR("OK"));
        delayMs(10 * DELAY);
    }
    lcd_clear();

    if (errors) {
        lcd_writeDec(errors);
        lcd_writeProgString(PSTR(" errors!"));
    } else {
        // SUCCESS
        lcd_clear();
        lcd_writeProgString(PSTR("ALL TESTS PASSED"));
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
    delayMs(10 * DELAY);
}

// Used for checking R/W barriers
PROGRAM(2, DONTSTART) {
    state = 1;

    if (mode == 1) {
        os_sh_writeOpen(DRIVER, &shBlock);
        os_error("FAILURE @ Read before write");
    } else if (mode == 2) {
        os_sh_readOpen(DRIVER, &shBlock);
        os_error("FAILURE @ Write before read");
    } else if (mode == 3) {
        os_sh_writeOpen(DRIVER, &shBlock);
        os_error("FAILURE @ Write before write");
    }
    errors++;
}

PROGRAM(3, DONTSTART) {
    os_sh_readOpen(DRIVER, &shBlockYield);
    os_sh_readOpen(DRIVER, &shBlockYield);
    os_sh_readOpen(DRIVER, &shBlockYield);
    os_sh_readOpen(DRIVER, &shBlockYield);
    os_sh_readOpen(DRIVER, &shBlockYield);
    os_sh_readOpen(DRIVER, &shBlockYield);
    os_error("No yield when read opened");
    while (1);
}

PROGRAM(4, DONTSTART) {
    if (mode == 0) {
        os_sh_writeOpen(DRIVER, &shBlockYield);
        os_sh_readOpen(DRIVER, &shBlockYield);
        os_sh_readOpen(DRIVER, &shBlockYield);
        os_error("No yield when w/r opened");
        while (1);
    } else if (mode == 1) {
        os_sh_readOpen(DRIVER, &shBlockYield);
        os_sh_readOpen(DRIVER, &shBlockYield);
        os_sh_writeOpen(DRIVER, &shBlockYield);
        os_error("No yield when r/w opened");
        while (1);
    }
}