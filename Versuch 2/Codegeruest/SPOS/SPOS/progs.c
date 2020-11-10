//-------------------------------------------------
//          TestSuite: Scheduling Strategien
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_scheduler.h"
#include "os_scheduling_strategies.h"
#include <avr/interrupt.h>
#include <util/delay.h>

#define PHASE_1 1
#define PHASE_2 1
#define PHASE_3 1
#define PHASE_4 1


volatile uint8_t capture[32];
volatile uint8_t i = 0;

#define LCD_DELAY 2000
#define NUM_EXECUTIONS_SCHEDULABILITY 30

ISR(TIMER2_COMPA_vect);

// Array containing the correct output values for all four scheduling strategies.
const uint8_t scheduling[4][32] PROGMEM  =  {
    {1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2}, // Even
    {1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 2, 2, 2, 2, 2, 3}, // Round Robin
    {1, 3, 3, 3, 2, 3, 3, 3, 2, 3, 1, 3, 2, 3, 3, 3, 2, 3, 3, 1, 3, 2, 3, 3, 3, 2, 3, 3, 1, 3, 2, 3}, // Inactive Aging
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1} // Run To Completion
};


//! Tests if strategy is implemented (default return is 0)
uint8_t strategyImplemented() {
    ProcessID nextId = 0;
    Process processes[MAX_NUMBER_OF_PROCESSES];

    // Copy current os_processes
    for (uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; ++i) {
        processes[i] = *os_getProcessSlot(i);
    }

    // Request next processId without changing anything
    // Current process is always 1
    switch (os_getSchedulingStrategy()) {
        case OS_SS_EVEN:
            nextId = os_Scheduler_Even(processes, os_getCurrentProc());
            break;
        case OS_SS_RANDOM:
            nextId = os_Scheduler_Random(processes, os_getCurrentProc());
            break;
        case OS_SS_ROUND_ROBIN:
            nextId = os_Scheduler_RoundRobin(processes, os_getCurrentProc());
            break;
        case OS_SS_INACTIVE_AGING:
            nextId = os_Scheduler_InactiveAging(processes, os_getCurrentProc());
            break;
        case OS_SS_RUN_TO_COMPLETION:
            nextId = os_Scheduler_RunToCompletion(processes, os_getCurrentProc());
            break;
        default:
            lcd_clear();
            lcd_writeProgString(PSTR("Invalid strategy"));
            while (1) {}
    }

    os_resetSchedulingInformation(os_getSchedulingStrategy());

    if (nextId == 0) {
        return 0;
    } else {
        return 1;
    }
}

/*!
 *  Function that sets the current strategy to the given one
 *  and outputs the name of the strategy on the LCD.
 */
uint8_t setActiveStrategy(SchedulingStrategy strategy) {
    uint8_t strategyIndex = 0;
    switch (strategy) {
        case OS_SS_EVEN:
            strategyIndex = 0;
            lcd_writeProgString(PSTR("Even"));
            os_setSchedulingStrategy(OS_SS_EVEN);
            break;

        case OS_SS_RANDOM:
            strategyIndex = 4;
            lcd_writeProgString(PSTR("Random"));
            os_setSchedulingStrategy(OS_SS_RANDOM);
            break;

        case OS_SS_ROUND_ROBIN:
            strategyIndex = 1;
            lcd_writeProgString(PSTR("RoundRobin"));
            os_setSchedulingStrategy(OS_SS_ROUND_ROBIN);
            break;

        case OS_SS_INACTIVE_AGING:
            strategyIndex = 2;
            lcd_writeProgString(PSTR("InactiveAging"));
            os_setSchedulingStrategy(OS_SS_INACTIVE_AGING);
            break;

        case OS_SS_RUN_TO_COMPLETION:
            strategyIndex = 3;
            lcd_writeProgString(PSTR("RunToCompletion"));
            os_setSchedulingStrategy(OS_SS_RUN_TO_COMPLETION);
            break;

        default:
            break;
    }
    _delay_ms(LCD_DELAY);

    return strategyIndex;
}

/*!
 *  Function that performs the given strategy for 32 steps
 *  and checks if the processes were scheduled correctly
 */
void performStrategyTest(SchedulingStrategy strategy) {
    lcd_clear();

    // Change scheduling strategy
    uint8_t strategyIndex = setActiveStrategy(strategy);

    // Test if first step is 0
    if (!strategyImplemented()) {
        lcd_clear();
        lcd_writeProgString(PSTR("Not impl. or idle returned"));
        _delay_ms(LCD_DELAY);
        return;
    }

    // Perform scheduling test.
    // Save the id of the running process and call the scheduler.
    i = 0;
    while (i < 32) {
        capture[i++] = 1;
        TIMER2_COMPA_vect();
    }

    // Print captured schedule
    lcd_clear();
    for (i = 0; i < 32; i++) {
        lcd_writeDec(capture[i]);
    }

    // Check captured schedule
    if (strategyIndex < 4) {
        for (i = 0; i < 32; i++) {
            if (capture[i] != pgm_read_byte(&scheduling[strategyIndex][i])) {
                // Move cursor
                lcd_goto((i >= 16) + 1, (i % 16) + 1);
                // Show cursor without underlining the position
                lcd_command((LCD_SHOW_CURSOR & ~(1 << 1)) | LCD_DISPLAY_ON);
                while (1) {}
            }
            if (i == 31) {
                _delay_ms(LCD_DELAY);
                lcd_clear();
                lcd_writeProgString(PSTR("OK"));
            }
        }
    } else {
		// Output message after Random-Strategy in order not to confuse students
		_delay_ms(LCD_DELAY);
		lcd_clear();
		lcd_writeProgString(PSTR("Can not be checked automatically"));
	}

    _delay_ms(LCD_DELAY);
}

/*!
 *  Function that performs the given strategy and checks
 *  if all processes could be scheduled
 */
void performSchedulabilityTest(SchedulingStrategy strategy, uint8_t expectation) {
    if (strategy == OS_SS_RUN_TO_COMPLETION) {
        // Ignore RunToCompletion as the programs never terminate
        return;
    }

    lcd_clear();

    // Change scheduling strategy
    setActiveStrategy(strategy);

    // Test if first step is 0
    if (!strategyImplemented()) {
        lcd_clear();
        lcd_writeProgString(PSTR("Not impl. or idle returned"));
        _delay_ms(LCD_DELAY);
        return;
    }

    uint8_t captured = 0;

    // Doing this multiple times will make sure we have a high probability of
    // scheduling every process (relevant for random strategy)
    for (uint8_t j = 0; j < NUM_EXECUTIONS_SCHEDULABILITY; j++) {
        // Perform strategy 32 times
        i = 0;
        while (i < 32) {
            capture[i++] = 1;
            TIMER2_COMPA_vect();
        }

        // Calculate which processes were scheduled
        for (uint8_t k = 0; k < 32; k++) {
            captured |= (1 << capture[k]);
        }

        // Check if all processes other than the idle process
        // were scheduled
        if (captured == expectation) {
            lcd_clear();
            lcd_writeProgString(PSTR("OK"));
            _delay_ms(LCD_DELAY);

            // Everything fine, so we can stop the test here
            return;
        }
    }

    // Find processes that were not scheduled (but should be)
    uint8_t notScheduled = (expectation & captured) ^ expectation;
    
    // Find processes that were scheduled (but should not be)
    uint8_t wrongScheduled = (captured & ~expectation);
	
	// Output the error to the user
	lcd_clear();
	switch (strategy) {
		case OS_SS_EVEN:
		lcd_writeProgString(PSTR("Even: "));
		break;

		case OS_SS_RANDOM:
		lcd_writeProgString(PSTR("Random: "));
		break;

		case OS_SS_ROUND_ROBIN:
		lcd_writeProgString(PSTR("RoundRobin: "));
		break;

		case OS_SS_INACTIVE_AGING:
		lcd_writeProgString(PSTR("InactiveAg.: "));
		break;

		case OS_SS_RUN_TO_COMPLETION:
		lcd_writeProgString(PSTR("RunToCompl.: "));
		break;
	}
	lcd_goto(1, 14);

    // Find the first incorrect process id
    for (uint8_t k = 1; k < MAX_NUMBER_OF_PROCESSES; k++) {
        if (notScheduled & (1 << k)) {
            lcd_writeDec(k);
            lcd_line2();
            lcd_writeProgString(PSTR("not schedulable"));
            break;
        }
		if (wrongScheduled & (1 << k)) {
			lcd_writeDec(k);
			lcd_line2();
			lcd_writeProgString(PSTR("falsely sched."));
			break;
		}
    }

    // Wait forever
    while (1) {}
}

/*!
 *  Function that tests the given strategy on an artificial empty process array
 *  This ensures the strategy does schedule the idle process if necessary.
 */
void performScheduleIdleTest(SchedulingStrategy strategy) {

    lcd_clear();

    // Change scheduling strategy
    setActiveStrategy(strategy);

    // Test if first step is 0
    if (!strategyImplemented()) {
        lcd_clear();
        lcd_writeProgString(PSTR("Not impl. or idle returned"));
        _delay_ms(LCD_DELAY);
        return;
    }

    // Setup artificial processes array
    Process processes[MAX_NUMBER_OF_PROCESSES];

    // Ensure clean states
    for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++)
        processes[i].state = OS_PS_UNUSED;

    uint8_t result;
    for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){

        switch(strategy) {
            case OS_SS_EVEN:
                result = os_Scheduler_Even(processes, i);
                break;
            case OS_SS_RANDOM:
                result = os_Scheduler_Random(processes, i);
                break;
            case OS_SS_ROUND_ROBIN:
                result = os_Scheduler_RoundRobin(processes, i);
                break;
            case OS_SS_INACTIVE_AGING:
                result = os_Scheduler_InactiveAging(processes, i);
                break;
            case OS_SS_RUN_TO_COMPLETION:
                result = os_Scheduler_RunToCompletion(processes, i);
                break;
        }

        if(result != 0)
            break;

    }

    if(result == 0){
		lcd_goto(2,0);
		lcd_writeProgString(PSTR("OK"));
		_delay_ms(LCD_DELAY*0.5);
		
        return;
	}
	
    // Output the error to the user
    lcd_clear();
	switch (strategy) {
        case OS_SS_EVEN:
            lcd_writeProgString(PSTR("Even: "));
            break;

        case OS_SS_RANDOM:
            lcd_writeProgString(PSTR("Random: "));
            break;

        case OS_SS_ROUND_ROBIN:
            lcd_writeProgString(PSTR("RoundRobin: "));
            break;

        case OS_SS_INACTIVE_AGING:
            lcd_writeProgString(PSTR("Inac.Ag.: "));
            break;

        case OS_SS_RUN_TO_COMPLETION:
            lcd_writeProgString(PSTR("RunToCompl.: "));
            break;
    }
    lcd_writeProgString(PSTR("Idle not scheduled"));
    
    // Wait forever
    while (1) {}
}


/*!
 * Program that deactivates the scheduler, spawns two programs
 * and performs the test
 */
PROGRAM(1, AUTOSTART) {
    // Disable scheduler-timer
    cbi(TCCR2B, CS22);
    cbi(TCCR2B, CS21);
    cbi(TCCR2B, CS20);

    os_getProcessSlot(os_getCurrentProc())->priority = 2;

    os_exec(2, 5);
    os_exec(3, 17);

    SchedulingStrategy strategies[] = {
        OS_SS_EVEN,
        OS_SS_RANDOM,
        OS_SS_ROUND_ROBIN,
        OS_SS_INACTIVE_AGING,
        OS_SS_RUN_TO_COMPLETION
    };

    uint8_t k = 0;
    uint8_t numStrategies = sizeof(strategies) / sizeof(SchedulingStrategy);

#if PHASE_1 == 1

    lcd_clear();
    lcd_writeProgString(PSTR("Phase 1:\nStrategies"));
    _delay_ms(LCD_DELAY);

    // Start strategies test
    for (k = 0; k < numStrategies; k++) {
        performStrategyTest(strategies[k]);
    }

#endif
#if PHASE_2 == 1

    lcd_clear();
    lcd_writeProgString(PSTR("Phase 2:\nIdle"));
    _delay_ms(LCD_DELAY);

    // Start strategies test
    for (k = 0; k < numStrategies; k++) {
        performScheduleIdleTest(strategies[k]);
    }

#endif
    
	// Execute programs so all process slots are in use
    os_exec(4, DEFAULT_PRIORITY);
    os_exec(5, DEFAULT_PRIORITY);
    os_exec(6, DEFAULT_PRIORITY);
    os_exec(7, DEFAULT_PRIORITY);
	
#if PHASE_3 == 1

    lcd_clear();
    lcd_writeProgString(PSTR("Phase 3: Schedulability All"));
    _delay_ms(LCD_DELAY);

    // Start schedulability test
    for (k = 0; k < numStrategies; k++) {
        performSchedulabilityTest(strategies[k], 0b11111110);
    }

#endif
#if PHASE_4 == 1

    lcd_clear();
    lcd_writeProgString(PSTR("Phase 4: Schedulability Partial"));
    _delay_ms(LCD_DELAY);

    // Reset some processes
    os_getProcessSlot(3)->state = OS_PS_UNUSED;
    os_getProcessSlot(4)->state = OS_PS_UNUSED;
    os_getProcessSlot(7)->state = OS_PS_UNUSED;

    // Start schedulability test
    for (k = 0; k < numStrategies; k++) {
        performSchedulabilityTest(strategies[k], 0b01100110);
    }

#endif

    // All tests passed
    while (1) {
        lcd_clear();
        lcd_writeProgString(PSTR("  TEST PASSED   "));
        _delay_ms(0.5 * LCD_DELAY);
        lcd_clear();
        _delay_ms(0.5 * LCD_DELAY);
    }
}

/*!
 *  Writes a given program ID to the capture array
 *  and calls the ISR manually afterwards
 *
 *  \param programID The ID which will be written to the capture array
 */
void testProgram(uint8_t programID) {
    while (1) {
        if (i < 32) {
            capture[i++] = programID;
        }
        TIMER2_COMPA_vect();
    }
}

PROGRAM(2, DONTSTART) {
    testProgram(2);
}

PROGRAM(3, DONTSTART) {
    testProgram(3);
}

PROGRAM(4, DONTSTART) {
    testProgram(4);
}

PROGRAM(5, DONTSTART) {
    testProgram(5);
}

PROGRAM(6, DONTSTART) {
    testProgram(6);
}

PROGRAM(7, DONTSTART) {
    testProgram(7);
}