//-----------------------------------------------------
//          TestSuite: Free Private
//-----------------------------------------------------
// Tests if private and shared memory is differentiated
// by the Heap protocol implementation.
//-----------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"
#include "os_input.h"

#include <avr/interrupt.h>

#define DELAY 100

#define WAIT 40

PROGRAM(1, AUTOSTART) {
    lcd_writeProgString(PSTR("Allocating "));
    os_malloc(intHeap, 1);
    lcd_writeChar('.');
    MemAddr sh = os_sh_malloc(intHeap, 1);
    lcd_writeChar('.');
    os_malloc(intHeap, 1);
    lcd_writeChar('.');
    os_sh_malloc(intHeap, 1);
    lcd_writeChar('.');
    MemAddr p = os_malloc(intHeap, 1);
    lcd_writeChar('.');
    os_sh_malloc(intHeap, 1);
    lcd_writeProgString(PSTR(" done!"));
    delayMs(10 * DELAY);

    lcd_clear();
    lcd_writeProgString(PSTR("Free private as shared:"));
    delayMs(10 * DELAY);
    os_sh_free(intHeap, &p);
    delayMs(10 * DELAY);

    lcd_clear();
    lcd_writeProgString(PSTR("Free shared as private:"));
    delayMs(10 * DELAY);
    os_free(intHeap, sh);
    delayMs(10 * DELAY);

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
