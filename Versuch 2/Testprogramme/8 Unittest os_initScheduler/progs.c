//-------------------------------------------------
//          UnitTest: os_initScheduler
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_process.h"
#include "os_input.h"

#include <avr/interrupt.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

//! This is a dummy program
PROGRAM(1, AUTOSTART) {}

/*!
 * Atomar funtion that prints a message on
 * the LCD and starts an infinite loop
 */
void message(const char* text) {
    cli();
    lcd_clear();
    lcd_writeString(text);
    while (1);
}

/*!
 * Prints an error if the given condition is false
 */
void test_assert(bool condition, const char* error) {
    if (!condition) {
        message(error);
    }
}

/*!
 * Unit test function which is declared with
 * "__attribute__ ((constructor))" such that it
 * is executed before the main function.
 */
void __attribute__((constructor)) test_os_initScheduler() {
    os_init_timer();
    lcd_init();
    os_initInput();
    
    delayMs(100);
	lcd_clear();
    lcd_writeString("Unittest os_initScheduler");
    delayMs(1000);

    // Make relevant variables/functions known and clear values
    extern Process os_processes[MAX_NUMBER_OF_PROCESSES];
    memset(os_processes, 0, sizeof(os_processes));

    extern Program* os_programs[MAX_NUMBER_OF_PROGRAMS];
    memset(os_programs, 0, sizeof(os_programs));

    extern void registerProgram0();
    extern void registerProgram1();
    registerProgram0();
    registerProgram1();

    os_initScheduler();

    // Check that idle and program 1 were started with the correct priority
    test_assert(os_processes[0].state == OS_PS_READY, "Idle not ready");
    test_assert(os_processes[0].priority == DEFAULT_PRIORITY, "Idle not default priority");
    test_assert(os_processes[1].state == OS_PS_READY, "Program 1 not started");
    test_assert(os_processes[1].priority == DEFAULT_PRIORITY, "Prog 1 not default priority");
    for (uint8_t i = 2; i < MAX_NUMBER_OF_PROCESSES; i++) {
        test_assert(os_processes[i].state == OS_PS_UNUSED, "Other slots not unused");
    }

    message("TEST PASSED");
    // Note that the main function is not executed because message contains an infinite loop
}