//-------------------------------------------------
//          TestSuite: Init
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"

#include <avr/interrupt.h>

//! Preprocessor macro to make a string out of a value
#define STRINGIFY(x) #x
//! Preprocessor macro to make a string out of a define
#define TOSTRING(x) STRINGIFY(x)
//! Number of programs that are tested
#define TEST_PROGRAMS (4)
//! Base address for test-results. In Main-stack, because Main-stack is unused
#define BASE_ADDRESS (0x1100-TEST_PROGRAMS)
//! Address for register-buffer
#define BUFFER_ADDRESS (BASE_ADDRESS-1)

#if VERSUCH != 2
    // You cannot run this with exercises after V2 due to the dynamic dispatcher.
    #error Define 'VERSUCH' in defines.h needs to be '2'
#endif

/*!
 * Assembler macro testing all registers for correct initialization e.g. all
 * registers are equal to zero. The result of this checked will be stored at
 * the very end of the SRAM: The last bytes of the main-stack. If the test is
 * successful, 0x01 is stored, if it fails, 0xFF is stored.
 * \param IDX ID of program whose setup is to be checked
 */
#define checkRegisterInit(IDX) \
    __asm__ volatile(\
                 "tst r0         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r1         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r2         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r3         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r4         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r5         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r6         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r7         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r8         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r9         \n\t"\
                 "brne 1f        \n\t"\
                 "tst r10        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r11        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r12        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r13        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r14        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r15        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r16        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r17        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r18        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r19        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r20        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r21        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r22        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r23        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r24        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r25        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r26        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r27        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r28        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r29        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r30        \n\t"\
                 "brne 1f        \n\t"\
                 "tst r31        \n\t"\
                 "brne 1f        \n\t"\
                 "breq 2f        \n\t"\
                 "1: sts  "TOSTRING(BUFFER_ADDRESS)", r16 \n\t"\
                 "   ldi  r16, 0xFF \n\t"\
                 "   sts  "TOSTRING(BASE_ADDRESS+IDX-1)",  r16  \n\t"\
                 "   jmp  3f        \n\t"\
                 "2: sts  "TOSTRING(BUFFER_ADDRESS)", r16 \n\t"\
                 "   ldi  r16, 0x01 \n\t"\
                 "   sts  "TOSTRING(BASE_ADDRESS+IDX-1)", r16 \n\t"\
                 "3: lds  r16, "TOSTRING(BUFFER_ADDRESS)" \n\t");


/*!
 * Program that checks its registers, spawns another process
 * and which is responsible for any outputs on the lcd
 */
PROGRAM(1, AUTOSTART) {
    // Here is where the real action takes place
    checkRegisterInit(1);

    uint8_t success;
    ProcessID execResult;

    execResult = os_exec(3, DEFAULT_PRIORITY);

    // Test if os_exec returned the correct value
    if (execResult == INVALID_PROCESS) {
        lcd_clear();
        lcd_writeProgString(PSTR("ERROR:"));
        lcd_line2();
        lcd_writeProgString(PSTR("os_exec failed"));
        while (1);
    }

    // This loop prints the table header
    for (uint8_t i = 0; i < TEST_PROGRAMS; i++) {
        lcd_writeDec(i + 1);
        lcd_writeChar(' ');
        if (i < TEST_PROGRAMS - 1) {
            lcd_writeChar('|');
        }
    }

    // In this loop we wait for all the processes that were automatically
    // started or manually started to evaluate their registers.
    for (;;) {
        lcd_line2();
        //Assume everything is good
        success = 1;

        for (uint8_t i = 0; i < TEST_PROGRAMS; i++) {
            uint8_t value = *((uint8_t*)(BASE_ADDRESS + i));
            switch (value) {
                case 0xFF:
                    lcd_writeProgString(PSTR("XX")); //XX is bad
                    success = 0; //We failed at least once
                    break;
                case 0x01:
                    lcd_writeProgString(PSTR("OK")); //OK is good
                    break;
                default:
                    lcd_writeProgString(PSTR("  ")); //Not evaluated yet
                    success = 0;
            }
            if (i < TEST_PROGRAMS - 1) {
                lcd_writeChar('|');
            }
        }

        // Check if any failures occured
        if (success) {
            delayMs(1000); // Give a chance to see the result
            lcd_clear();
            for (;;) {
                lcd_writeProgString(PSTR("ALL TESTS PASSED"));
                delayMs(1000);
                lcd_clear();
                delayMs(1000);
            }
        }
    }
}

/*!
 * Program that checks its registers and
 * spawns another process
 */
PROGRAM(2, AUTOSTART) {
    checkRegisterInit(2);
    // Test if os_exec returned the correct value
    if (os_exec(4, DEFAULT_PRIORITY) == INVALID_PROCESS) {
        lcd_clear();
        lcd_writeProgString(PSTR("ERROR:"));
        lcd_line2();
        lcd_writeProgString(PSTR("os_exec failed"));
    }
    for (;;);
}

/*!
 * Program that only checks its registers
 */
PROGRAM(3, DONTSTART) {
    checkRegisterInit(3);
    for (;;);
}

/*!
 * Program that only checks its registers
 */
PROGRAM(4, DONTSTART) {
    checkRegisterInit(4);
    for (;;);
}
