//-------------------------------------------------
//          TestSuite: Critical
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"

#include <stdbool.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

// Flag is used to signal to phase 1 that the button was pressed
static volatile bool flag = false;

// Handy error message define
#define ERROR(TEXT) {                   \
    cli();                              \
    lcd_clear();                        \
    lcd_writeProgString(PSTR(TEXT));    \
	while(1);                           \
}


// Check for a pin change on Enter button
ISR(PCINT2_vect) {
    flag = true;
}

#define PROG_PHASE_1            1
#define PROG_PHASE_1_HELPER     2
#define PROG_PHASE_2            3
#define PROG_PHASE_3            4
#define PROG_PHASE_3_TEXT_1     5
#define PROG_PHASE_3_TEXT_2     6

/*
 * Print input string character by character.
 * Using a critical section the output should be uninterrupted.
 */
void crPrint(char const* str) {
    char print[17];
    size_t i = 0;
    char c;

    strlcpy_P(print, str, 16);
    os_enterCriticalSection();

    while ((c = print[i++])) {
        lcd_writeChar(c);
        delayMs(DEFAULT_OUTPUT_DELAY);
    }

    while (i++ <= 8) {
        lcd_writeChar(' ');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }

    os_leaveCriticalSection();
    delayMs(4 * DEFAULT_OUTPUT_DELAY);
}

//-------------------------------------------------------------
// Phase 1: Check if critical sections are wrongly implemented
//          using the global interrupt enable bit
//-------------------------------------------------------------
PROGRAM(PROG_PHASE_1, AUTOSTART) {
	lcd_clear();
	lcd_writeProgString(PSTR("Phase 1"));
	lcd_line2();
	lcd_writeProgString(PSTR("Interrupt Bits"));
	delayMs(10 * DEFAULT_OUTPUT_DELAY);

    uint8_t i = 0, sec = 0;

    // Setup Pin Change interrupt for Enter button
    PCICR  = (1 << PCIE2);
    PCMSK2 = (1 << PCINT16);

    os_enterCriticalSection();
	
	// Preserve original process states
	extern Process os_processes[MAX_NUMBER_OF_PROCESSES];
	Process processes_backup[MAX_NUMBER_OF_PROCESSES];
	memcpy(processes_backup, os_processes, sizeof(os_processes));

	os_exec(PROG_PHASE_1_HELPER, DEFAULT_PRIORITY);

    lcd_clear();
    lcd_writeProgString(PSTR("Please wait"));
    
    // if the scheduler is still active, Program PROG_PHASE_1_HELPER will indicate the error when its scheduled during this delay
    delayMs(1000);

    lcd_clear();
    lcd_writeProgString(PSTR("Press Enter"));

    // Wait 15 seconds or until button is pressed
    for (; (i < 150) && (!flag); i++) {
        lcd_goto(1, 14);
        sec = 15 - i / 10;
        if (i % 4 == 0 || sec > 5) {
            if (sec < 10) {
                lcd_writeChar(' ');
            }
            lcd_writeDec(sec);
            lcd_writeChar('s');
        } else {
            lcd_writeProgString(PSTR("   "));
        }

        delayMs(100);
    }

    // Check if button was pressed or a timeout occurred
    lcd_clear();
    if (flag) {
        lcd_writeProgString(PSTR("Phase 1 complete"));
        delayMs(20 * DEFAULT_OUTPUT_DELAY);
    } else {
        // Button was either not pressed or pin change interrupt
        // was blocked by critical section
        ERROR("ERR: No button  press detected")
    }
    
	// restore process array, this removes PROG_PHASE_1_HELPER from the process array allowing the testtask to continue
	memcpy(os_processes, processes_backup, sizeof(os_processes));

    os_leaveCriticalSection();

    // proceed to phase 2
	os_exec(PROG_PHASE_2, DEFAULT_PRIORITY);

    // Wait for the scheduler to switch to another process
    while (1);
}

PROGRAM(PROG_PHASE_1_HELPER, DONTSTART) {
    ERROR("ERR: Other Proc. scheduled");
}


//-------------------------------------------------------------
// Phase 2: Check correct usage of critical sections in os_exec
//-------------------------------------------------------------
extern uint8_t criticalSectionCount;
PROGRAM(PROG_PHASE_2, DONTSTART){
	lcd_clear();
	lcd_writeProgString(PSTR("Phase 2"));
	lcd_line2();
	lcd_writeProgString(PSTR("Save os_exec"));
	delayMs(10 * DEFAULT_OUTPUT_DELAY);

	os_enterCriticalSection();
		
	// Acquire external process array
	extern Process os_processes[MAX_NUMBER_OF_PROCESSES];

    // and occupy all slots
    for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
        os_processes[i].state = OS_PS_READY;
    }

	// Check that os_exec leaves critical section when all process slots are in use
	os_exec(0, DEFAULT_PRIORITY); // program ID is irrelevant as it cannot be executed anyway
	if(criticalSectionCount != 1){
        ERROR("Critical Section not closed");
    }

	// Clear slots again
    for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
        os_processes[i].state = OS_PS_UNUSED;
    }

	// Check that os_exec leaves critical section after non existing Program ID
	os_exec(MAX_NUMBER_OF_PROGRAMS, DEFAULT_PRIORITY);
	if(criticalSectionCount != 1){
        ERROR("Critical Section not closed");
    }
		
	lcd_clear();
	lcd_writeProgString(PSTR("Phase 2 complete"));
	delayMs(20 * DEFAULT_OUTPUT_DELAY);

	// Show PASSED message
	while (1) {
		lcd_clear();
		lcd_writeProgString(PSTR("  TEST PASSED   "));
		delayMs(1000);
		lcd_clear();
		delayMs(500);
	}
}