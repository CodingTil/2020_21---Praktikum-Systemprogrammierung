#include "os_core.h"
#include "defines.h"
#include "util.h"
#include "lcd.h"
#include "os_input.h"
#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"

#include <avr/interrupt.h>

void os_initScheduler(void);

extern uint8_t const __heap_start;


//variable specially forcing the c/c++ linker to not initialize the variable so we can detect its initialized state
uint8_t softResetDetector __attribute__ ((section (".noinit")));

/*! \file
 *
 * The main system core with initialization functions and error handling.
 *
 */

/* 
 * Certain key-functionalities of SPOS do not work properly if optimization is
 * disabled (O0). It will still compile with O0, but you may encounter
 * run-time errors. This check is supposed to remind you on that.
 */
#ifndef __OPTIMIZE__
    #warning "Compiler optimizations disabled; SPOS and Testtasks may not work properly."
#endif

/*!
 *  Initializes the used timers.
 */
void os_init_timer(void) {
    // Init timer 2 (Scheduler)
    sbi(TCCR2A, WGM21); // Clear on timer compare match

    sbi(TCCR2B, CS22); // Prescaler 1024  1
    sbi(TCCR2B, CS21); // Prescaler 1024  1
    sbi(TCCR2B, CS20); // Prescaler 1024  1
    sbi(TIMSK2, OCIE2A); // Enable interrupt
    OCR2A = 60;

    // Init timer 0 with prescaler 256
    cbi(TCCR0B, CS00);
    cbi(TCCR0B, CS01);
    sbi(TCCR0B, CS02);

    sbi(TIMSK0, TOIE0);
}

/*!
*
*	Reads out the MCUs reset register and prints the last hardware reset reasons on the lcd
*/

void os_checkResetRegister(uint8_t blocking){
    lcd_line2();
    //evaluating hardware reset register
    if(MCUSR){
        //checking Bit 4 = JTag reset Register only set when AVR_Reset is recieved
        if(gbi(MCUSR,4)){
            lcd_writeProgString(PSTR("JT "));
        }
        //checking Bit 3 = Watchdog reset register
        if(gbi(MCUSR,3)){
            lcd_writeProgString(PSTR("WD "));
        }
        //checking Bit 2 = Brown out detection register
        if(gbi(MCUSR,2)){
            lcd_writeProgString(PSTR("BO "));
        }
        //checking Bit 1 = External reset register (external reset button pressed)
        if(gbi(MCUSR,1)){
            lcd_writeProgString(PSTR("EXT "));
        }
        //checking Bit 0 = Power reset register mains power failed
        if(gbi(MCUSR,0)){
            lcd_writeProgString(PSTR("POW"));
        }
        //if user is required to confirm error by button presses
        if(blocking){
            os_waitForInput();
            os_waitForNoInput();
        }
        //reset hardware and software reset detectors
        MCUSR = 0;
        softResetDetector = 0;
    }
}

/*!
*
*	Reads out the specially defined soft reset variable to detect softresets by the MCU
*/

void os_checkSoftReset(uint8_t blocking){
    //if variable is already initialized the main method already had ran to a certain point and softly reset
    if(softResetDetector){
        lcd_clear();
        lcd_writeProgString(PSTR("--SYSTEM ERROR--"));
        lcd_writeProgString(PSTR("---SOFT RESET---"));
        if(blocking){
            os_waitForInput();
            os_waitForNoInput();
        }
    }
    //initialize global variable to detect soft resets later
    softResetDetector = 0xFF;
}

/*!
 *  Readies stack, scheduler and heap for first use. Additionally, the LCD is initialized. In order to do those tasks,
 *  it calls the sub function os_initScheduler().
 */
void os_init(void) {
    // Init timer 0 and 2
    os_init_timer();

    // Init buttons
    os_initInput();

    // Init LCD display
    lcd_init();

    lcd_writeProgString(PSTR("Booting SPOS ..."));
    os_checkResetRegister(0);
    os_checkSoftReset(1);
    delayMs(2000);

    os_initScheduler();

	if(&(__heap_start) > (AVR_SRAM_START + HEAPOFFSET)) {
		os_errorPStr("Heap Offset too small!");
	}
	intSRAM.init();
	
	os_initHeaps()

    os_coarseSystemTime = 0;
}

/*!
 *  Terminates the OS and displays a corresponding error on the LCD.
 *
 *  \param str  The error to be displayed
 */
void os_errorPStr(char const* str) {
	uint8_t interrupts = gbi(SREG, 7);
	
	cbi(SREG,7);
	
	lcd_clear();
	
	// print error message
	lcd_writeProgString(str);
	
	// wait for Enter + ESC
	while (true) {
		os_waitForInput();
		if (os_getInput() == 0b00001001) {
			break;
		}
	}
	
	// wait for release
	os_waitForNoInput();
	
	lcd_clear();

	if (interrupts) {
		sbi(SREG, 7);
	}
}
