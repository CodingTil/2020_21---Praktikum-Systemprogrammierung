//-------------------------------------------------
//          TestSuite: Button/Error Test
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_input.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>

/************************************************************************/
/* Defines                                                              */
/************************************************************************/

//! Output delay after writing strings to the LCD
#define DELAY 2000
//! Halts execution of program
#define HALT  do{}while(1)

//! Set this to 1 to check the button functionality
#define PHASE_BUTTONS        (1)
//! Set this to 1 to check the os_error functionality
#define PHASE_ERROR          (1)

/************************************************************************/
/* Forward declarations                                                 */
/************************************************************************/

//! Prints an error message on the LCD. Then halts.
void be_throwError(char const* str);
//! Tests one button.
void be_testBtn(uint8_t btnNr);
//! Prints phase information to the LCD.
void be_showPhase(uint8_t i, char const* name);
//! Prints OK for the phase message.
void be_phaseSuccess(void);

/************************************************************************/
/* Public variables                                                     */
/************************************************************************/

// Variable to count the number of timer interrupts that occurred
static volatile uint16_t count = 0;

// Counts the number of interrupts that occured
ISR(TIMER1_COMPA_vect) {
    count++;
    if (count > 5) {
        be_throwError(PSTR("Interrupted"));
    }
}


PROGRAM(1, AUTOSTART) {

#if PHASE_BUTTONS == 1
    // 1. Checking registers for buttons
    be_showPhase(1, PSTR("Registers"));
    delayMs(DELAY);

    if ((DDRC & 0xC3) != 0) {
        be_throwError(PSTR("DDR wrong"));
    }

    if ((PORTC & 0xC3) != 0xC3) {
        be_throwError(PSTR("No pull ups"));
    }

    be_phaseSuccess();
    delayMs(DELAY);

    // 2. Checking input-functions
    be_showPhase(2, PSTR("Button test"));
    delayMs(DELAY);

    uint8_t i;
    for (i = 1; i <= 4; i++) {
        be_testBtn(i);
    }

    be_showPhase(2, PSTR("Button test"));
    be_phaseSuccess();
    delayMs(DELAY);
#endif

#if PHASE_ERROR == 1
    // 3. Checking if the global interrupt flag is disabled during os_error
    // Disable scheduler timer
    TCCR2B   = 0;
    // Configure our check timer for CTC, 1024 prescaler
    TCCR1A   = 0;
    TCCR1C   = 0;
    OCR1A    = 500;
    TIMSK1  |= (1 << OCIE1A);

    be_showPhase(3, PSTR("Interrupts"));
    delayMs(DELAY);

    TCCR1B = (1 << WGM12) | (1 << CS10);

    // Throw an error
    os_error("Interrupts Confirm error!");

    // At this point the error was confirmed and a correct os_error will reactivate
    // global interrupts, which leads to exactly one timer interrupt

    // Disable our check timer
    TCCR1B = 0;
    lcd_clear();

    // The test passes only when less or equal 5 interrupts were counted
    if (count <= 5) {
        be_showPhase(3, PSTR("Interrupts"));
        be_phaseSuccess();
        delayMs(DELAY);
    } else {
        be_throwError(PSTR("Interrupted"));
    }

    // Phase 4: Does os_error restore GIEB if it was activated?
    be_showPhase(4, PSTR("GIEB on"));
    delayMs(DELAY);

    // Enable global interrupts
    sei();

    // Throw an error
    os_error("GIEB on Confirm error!");

    // Error was confirmed, check if global interrupts are still on
    if (SREG & (1 << 7)) {
        be_showPhase(4, PSTR("GIEB on"));
        be_phaseSuccess();
        delayMs(DELAY);
    } else {
        cli();
        be_throwError(PSTR("GIEB falsely off"));
    }


    // Phase 5: Does os_error restore GIEB if it was deactivated?
    be_showPhase(5, PSTR("GIEB off"));
    delayMs(DELAY);

    // Disable global interrupts
    cli();

    // Throw an error
    os_error("GIEB off Confirm error!");

    // Error was confirmed, check if global interrupts are still off
    if (SREG & (1 << 7)) {
        be_throwError(PSTR("GIEB falsely on"));
    } else {
        be_showPhase(5, PSTR("GIEB off"));
        be_phaseSuccess();
        delayMs(DELAY);
    }
#endif

    lcd_clear();
    while (1) {
        lcd_writeProgString(PSTR("ALL TESTS PASSED"));
        delayMs(DELAY);
        lcd_clear();
        delayMs(DELAY);
    }
}

/*!
 * Prints an error message on the LCD. Then halts.
 * \param  *str Message to print
 */
void be_throwError(char const* str) {
    lcd_clear();
    lcd_line1();
    lcd_writeProgString(PSTR("Error:"));
    lcd_line2();
    lcd_writeProgString(str);
    HALT;
}

/*!
 * Tests one button. Tests for the correct function of os_getInputs,
 * os_waitForInput and os_waitForNoInput. If an error occurs, an error message
 * is shown.
 * \param btnNr Number of button to test (between 1 and 4)
 */
void be_testBtn(uint8_t btnNr) {
    lcd_clear();
    lcd_writeProgString(PSTR("Press button "));
    lcd_writeDec(btnNr);

    os_waitForInput();
    delayMs(50);

    if (os_getInput() == 0) {
        be_throwError(PSTR("waitForInput"));
    }
    if ((os_getInput() != 1 << (btnNr - 1)) || ((PINC & 0b11000011) != (btnNr < 3 ? (0b11000011 ^ 1 << (btnNr - 1)) : (0b11000011 ^ 1 << (btnNr + 3))))) {
        be_throwError(PSTR("getInput"));
    }

    lcd_clear();
    lcd_writeProgString(PSTR("Release button "));
    lcd_writeDec(btnNr);

    os_waitForNoInput();
    delayMs(50);

    if ((os_getInput() != 0) || ((PINC & 0xC3) != 0xC3)) {
        be_throwError(PSTR("waitForNoInput"));
    }
}

/*!
 * Prints phase information to the LCD.
 * \param  i    Index of phase
 * \param *name Name of phase
 */
void be_showPhase(uint8_t i, char const* name) {
    lcd_clear();
    lcd_writeProgString(PSTR("Phase "));
    lcd_writeDec(i);
    lcd_writeChar(':');
    lcd_line2();
    lcd_writeProgString(name);
}

/*!
 * Prints OK for the phase message.
 */
void be_phaseSuccess(void) {
    lcd_goto(2, 15);
    lcd_writeProgString(PSTR("OK"));
}
