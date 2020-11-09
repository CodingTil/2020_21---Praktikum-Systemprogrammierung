//-------------------------------------------------
//          UnitTest: os_exec
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

//! This is a dummy program to exec during testing
PROGRAM(9, DONTSTART) {
    while (1);
}

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
void __attribute__((constructor)) test_os_exec() {
    os_init_timer();
    lcd_init();
    os_initInput();
	
	delayMs(100);
	lcd_clear();
	lcd_writeString("Unittest os_exec");
	delayMs(1000);

    // Make relevant variables/functions known and clear values
    extern Process os_processes[MAX_NUMBER_OF_PROCESSES];
    memset(os_processes, 0, sizeof(os_processes));

    extern Program* os_programs[MAX_NUMBER_OF_PROGRAMS];
    memset(os_programs, 0, sizeof(os_programs));

    extern void registerProgram9();
    registerProgram9();

    // Check that os_exec correctly returns INVALID_PROCESS when all process slots are in use
    memset(os_processes, OS_PS_READY, sizeof(os_processes));
    test_assert(os_exec(0, 1) == INVALID_PROCESS, "Expected invalid process");

    // Clear slots again
    memset(os_processes, 0, sizeof(os_processes));

    // Check that os_exec returns INVALID_PROCESS for a non-existing program id
    test_assert(os_exec(1, 1) == INVALID_PROCESS, "Expected invalid process");

    /*
     * Initialize stack of process 0 and the stack below with a mask
     * so we can check after os_exec() if the registers were initialized
     * and other stacks were not overwritten
     */
    uint16_t stackBottomProcess0 = PROCESS_STACK_BOTTOM(0);
    for (uint8_t i = 0; i < 35; i++) {
        // Write to the stack of process 0: 1, 2, 3, ...
        os_processes[0].sp.as_int = stackBottomProcess0 - i;
        *(os_processes[0].sp.as_ptr) = i + 1;

        // Write to the stack below process 0 (scheduler stack): 1, 2, 3, ...
        os_processes[0].sp.as_int = stackBottomProcess0 + i + 1;
        *(os_processes[0].sp.as_ptr) = i + 1;
    }

    // Check that os_exec correctly initializes the process structure and stack and picks the correct slot
    test_assert(os_exec(9, 10) == 0, "PID not 0");
    test_assert(os_processes[0].priority == 10, "Priority not 10");
    test_assert(os_processes[0].progID == 9, "ProgID not 9");
    test_assert(os_processes[0].state == OS_PS_READY, "State not READY");
    test_assert(os_processes[0].sp.as_int == (PROCESS_STACK_BOTTOM(0) - 35), "SP invalid");

    // Check that there are 33 zeros on the stack and the program function is written in the correct order on the stack
    for (uint8_t i = 1; i <= 33; i++) {
        test_assert(os_processes[0].sp.as_ptr[i] == 0, "Non-zero for register");
    }
    uint16_t program = (uint16_t)os_lookupProgramFunction(os_processes[0].progID);
    test_assert(os_processes[0].sp.as_ptr[34] == (program >> 8), "Invalid hi byte");
    test_assert(os_processes[0].sp.as_ptr[35] == (uint8_t)program, "Invalid lo byte");

    for (uint8_t i = 1; i <= 35; i++) {
        test_assert(os_processes[0].sp.as_ptr[35 + i] == i, "Written into wrong stack");
    }

    message("TEST PASSED");
    // Note that the main function is not executed because message contains an infinite loop
}
