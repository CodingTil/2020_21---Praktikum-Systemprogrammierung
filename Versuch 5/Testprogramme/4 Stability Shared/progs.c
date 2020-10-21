//-------------------------------------------------
//          TestSuite: Stability Shared
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"

#include <avr/interrupt.h>
#include <string.h>

#define DELAY 100
#define DRIVER intHeap

// Forward declarations
void writeChar(char c);
void makeCheck(uint16_t size, MemValue pat);
void myRelocate(uint16_t newsz);

unsigned char pos = 16;

//! This program prints the current time in the first line
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
        lcd_goto(2, pos + 1);
        os_leaveCriticalSection();
        delayMs(100);
    }
}

static char PROGMEM const spaces16[] = "                ";

//! Write a character to the correct position in the second line after the pipe
void writeChar(char c) {
    os_enterCriticalSection();
    if (++pos > 16) {
        pos = 5;
        lcd_line2();
        lcd_goto(2, pos);
        lcd_writeChar('|');
        pos++;
        lcd_writeProgString(spaces16 + 5);
    }
    lcd_goto(2, pos);
    lcd_writeChar(c);
    os_leaveCriticalSection();
}

MemAddr sh;
uint16_t sz;

//! Main test function
void makeCheck(uint16_t size, MemValue pat) {
    // Allocate private memory and read in the data of the shared memory
    MemAddr const priv = os_malloc(intHeap, size);
    os_sh_read(DRIVER, &sh, 0, (uint8_t*)priv, size);

    // Test if only one pattern is present throughout the memory chunk
    MemValue const check = intHeap->driver->read(priv);
    MemAddr p;
    unsigned char tmp;
    for (p = priv; p < priv + size; p++) {
        tmp = intHeap->driver->read(p);
        if (tmp != check) {
            os_error("Write was interleaved");
        }
        intHeap->driver->write(p, pat);
    }

    // Write private memory (now with new pattern) back to shared and free the private one
    os_sh_write(DRIVER, &sh, 0, (uint8_t*)priv, size);
    os_free(intHeap, priv);
}

//! Function for reallocation of shared memory
void myRelocate(uint16_t newsz) {

    // Allocate new shared memory and write zeros to it
    MemAddr newsh = os_sh_malloc(DRIVER, newsz);
    // important because our programs expect all cells to be the same
    MemAddr i;
    for (i = 0; i < sz; i++) {
        DRIVER->driver->write(newsh + i, 0);
    }

    // Swap newly allocated shared memory with public shared memory
    os_enterCriticalSection();
    sh ^= newsh;
    newsh ^= sh;
    sh ^= newsh;
    sz = newsz;
    os_leaveCriticalSection();

    // Now free the old shared memory
    os_sh_free(DRIVER, &newsh);
}

//! Entry program that creates several instances of the test program
PROGRAM(1, AUTOSTART) {
    uint8_t counter = 4;

    // Allocate shared memory and write to it
    sz = os_getUseSize(intHeap) / counter;
    sh = os_sh_malloc(DRIVER, sz);
    MemAddr i;
    for (i = 0; i < sz; i++) {
        DRIVER->driver->write(sh + i, 0xFF);
    }

    // Start checking processes
    while (--counter) {
        os_exec(2, 10);
    }

    // Do iteration printout.
    MemValue start;
    MemValue end;
    uint16_t writeItrs = 0;
    for (;;) {
        // Read from shared memory to test if a writing operation is in progress by another process
        // If writing to shared memory uses critical section as locks this will never be the case
        cli();
        start = DRIVER->driver->read(sh);
        end = DRIVER->driver->read(sh + sz - 1);
        sei();
        if (start != end) {
            writeItrs++;
            cli();
            lcd_goto(2, 1);
            lcd_writeProgString(spaces16 + (16 - 4));
            lcd_goto(2, 1);
            lcd_writeDec(writeItrs);
            sei();
            Time const now = getSystemTime(); // Granularity: 0.1/8 ms
            while (now == getSystemTime()) {
                os_yield();
            }
        }
        os_yield();
    }
}


/*!
 * Program that writes a pattern to the shared memory region and reallocates it
 * using a newly introduced function. Furthermore, it checks whether the pattern
 * in shared memory is consistent.
 */
PROGRAM(2, DONTSTART) {
    for (;;) {
        uint8_t k = (TCNT0 % 40);
        while (k--) {
            makeCheck(sz, os_getCurrentProc());
        }
        myRelocate(sz);
        writeChar(os_getCurrentProc() + '0');
    }
}
