//-------------------------------------------------
//          TestSuite: HeapStart
//-------------------------------------------------
// Tests if the HeapStart check is effective
// by allocating a large block of memory as a global
// variable.
// An error must be shown by the operating system.
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_input.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"

// Large global memory
uint8_t dummy[512];


PROGRAM(1, AUTOSTART) {
    // Write something into the memory block to prevent omission at compile time.
    dummy[0] = 'A';
    dummy[sizeof(dummy) - 1] = 'B';

    lcd_clear();
    lcd_writeProgString(PSTR("OK if error"));

    while (1);
}
