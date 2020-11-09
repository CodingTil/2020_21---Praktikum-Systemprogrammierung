//-------------------------------------------------
//          TestSuite: Range
//-------------------------------------------------


//  NOTE:
//  The heap cleanup must work for this test to run correctly!
//  The run will hang, if you have a bug in the process termination.

#include "lcd.h"
#include "util.h"
#include "os_input.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"

#include <avr/interrupt.h>

#define DELAY 100

#define DRIVER intHeap
#define DSIZE (os_getUseSize(DRIVER)/4)

uint16_t size = 1;
volatile uint16_t check_i = 0;
//! This variable is set to true when the test program finishes
bool terminated = false;

/*!
 * Display memory size and start test execution.
 */
PROGRAM(3, AUTOSTART) {
    // Init Timeout Timer 1
    // CTC mode
    TCCR1A &= ~(1 << WGM10);
    TCCR1A &= ~(1 << WGM11);
    TCCR1B |= (1 << WGM12);
    TCCR1B &= ~(1 << WGM13);

    // set prescaler to 1024
    TCCR1B |=  (1 << CS12) | (1 << CS10);
    TCCR1B &= ~(1 << CS11);

    // set compare register -> match every 1s
    OCR1A = 19531;

    // enable timer
    TIMSK1 |= (1 << OCIE1A);

    lcd_clear();
    lcd_writeProgString(PSTR("ExtSRAM Size: "));
    lcd_line2();
    lcd_writeDec(extSRAM->size);
    lcd_writeChar('/');
    lcd_writeHex(extSRAM->size);
    delayMs(1000);

    os_exec(1, DEFAULT_PRIORITY);
}


/*!
 * This program tests allocation of memory of arbitrary position and size within the heap.
 * To that end it allocates a quarter of the available memory
 * in chunks of increasing size (from 1 to DSIZE).
 */
PROGRAM(1, DONTSTART) {
    ProgramID cont = 1;
    lcd_clear();
    lcd_writeProgString(PSTR("[Testing range]"));
    if (size <= DSIZE) {
        lcd_line2();
        lcd_writeProgString(PSTR("Mass alloc: "));
        lcd_writeDec(size);
        uint16_t i;
        // Allocate chunks
        for (i = 0; i < DSIZE / size; i++) {
            if (!os_malloc(DRIVER, size)) {
                lcd_clear();
                lcd_writeProgString(PSTR("Could not alloc! (Phase 1)"));
                while (1);
            }

            // Print progress
            lcd_drawBar((uint16_t)((100ul * i) / (DSIZE / size)));
            lcd_line2();
            lcd_writeProgString(PSTR("Mass alloc: "));
            lcd_writeDec(size);
            check_i = i;

        }
        // Increase the chunk size for next program run
        size++;
    } else {
        // All chunk sizes done. Switch to second phase (run prog 2).
        cont = 2;
    }
    lcd_line2();
    lcd_writeProgString(PSTR("OK, next..."));

    // Force termination and freeing of allocated memory by heap cleanup
    // before first execution of new program instance.
    os_enterCriticalSection();
    os_exec(cont, DEFAULT_PRIORITY);
}

/*!
 * This program asserts if a chunk of memory can be freed
 * by using a pointer to a arbitrary position within the chunk.
 */
PROGRAM(2, DONTSTART) {
    uint8_t accuracy = 10;
    lcd_clear();
    lcd_writeProgString(PSTR("[Testing range]"));

    // Iterate chunk sizes
    for (size = 1; size <= DSIZE; size++) {
        lcd_line2();
        lcd_writeProgString(PSTR("Random free: "));
        lcd_writeDec(size);

        uint16_t i;
        // Try different addresses to free memory
        for (i = 0; i < (size / accuracy); i++) {
            uint16_t startAddress = os_malloc(DRIVER, size);
            if (!startAddress) {
                lcd_clear();
                lcd_writeProgString(PSTR("Could not alloc! (Phase 2)"));
                while (1);
            }
            // Use the timer counter value as "random" number
            os_free(DRIVER, startAddress + (TCNT0 % size));
            lcd_drawBar((100 * i) / (size / accuracy));
            lcd_line2();
            lcd_writeProgString(PSTR("Random free: "));
            lcd_writeDec(size);
        }
    }

    // SUCCESS
    terminated = true;
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

/*
 * Timeout ISR
 */
ISR(TIMER1_COMPA_vect) {
    // counts seconds
    static int16_t timeout_counter = 0;
    if(++timeout_counter >= 900 && !terminated){
        os_error("Test timed out after 15 minutes!");
    }
}
