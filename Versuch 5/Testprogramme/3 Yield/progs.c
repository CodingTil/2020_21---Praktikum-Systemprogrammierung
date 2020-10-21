#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"
#include <avr/interrupt.h>

#define DELAY (999 + 1ul)
#define BASE 2

//ACHTUNG CUtoffs müssen angepasst werden falls Iterationszahl verändert wird
#define MAX_TEST_ITERATIONS 9

//define strategies to test
#define EVEN  1
#define MLFQ  1
#define RAND  1
#define INAC  1
#define RR    1
#define RCOMP 1

typedef unsigned long volatile Iterations;
Iterations iterations1 = 0, iterations2 = 0;
uint8_t finished;

ProcessID yielder_id,runner_id,printer_id;
volatile uint8_t current_strat = 0;

typedef struct TestResults{
    uint8_t ratio;
    char const* const name;
    SchedulingStrategy strat;
    uint8_t active;
    uint8_t cutoff_bot;
    uint8_t cutoff_top;
} TestResults;

static char PROGMEM const  evenStr[] = "EVEN";
static char PROGMEM const  mlfqStr[] = "MLFQ";
static char PROGMEM const  randStr[] = "RAND";
static char PROGMEM const  inacStr[] = "INACTIVE AGING";
static char PROGMEM const    rrStr[] = "ROUND ROBIN";
static char PROGMEM const  runcomp[] = "Run to Comp";

TestResults results[6] = {
    {0,evenStr,OS_SS_EVEN,EVEN,4,255},
    {0,mlfqStr,OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE,MLFQ,30,255},
    {0,randStr,OS_SS_RANDOM,RAND,3,255},
    {0,inacStr,OS_SS_INACTIVE_AGING,INAC,4,255},
    {0,rrStr,OS_SS_ROUND_ROBIN,RR,8,255},
    {0,runcomp,OS_SS_RUN_TO_COMPLETION,RCOMP,1,1},
};

TestResults *select_next_strat(){
    while(current_strat < 5 ){
        current_strat++;
        if(results[current_strat].active){
            return &results[current_strat];
        }
    }
    return NULL;
}

//! Calculates the log to display the yield/noYield quotient
unsigned logBase(unsigned long base, unsigned long value) {
    if (base <= 1) {
        return 0;
    }
    unsigned result = 0;
    for (; value; value /= base) {
        result++;
    }
    return result;
}

void enterMultipleCriticalSections(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        os_enterCriticalSection();
    }
}

void leaveMultipleCriticalSections(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        os_leaveCriticalSection();
    }
}

/*!
 * For each iteration some operations are executed in a critical section and counting the completed iterations.
 * If yield is true yield will be called after half of the operations.
 * Therefore testing if yield() correctly passes computing time on to the next process.
 */
void loop(bool yield, Iterations* iterations) {
    for (;;) {
        while (1) {
            if (yield) {
                enterMultipleCriticalSections(128);
                os_yield();
                leaveMultipleCriticalSections(128);
            } else {
                cli();
                // This will produce a critical section overflow, if yield doesn't reset the count
                // This is needed to test, because the criticalSectionCount is not available globally
                enterMultipleCriticalSections(128);
                leaveMultipleCriticalSections(128);
                sei();
            }
            cli();
            ++*iterations;
            sei(); // We positively KNOW the I-Flag was off!
        }
    }
}

//! Main test program.
PROGRAM(1, AUTOSTART) {
    lcd_writeProgString(PSTR("1: Checking yield..."));
    delayMs(DELAY);
    // Set ibit of SREG to 0 to check if it is properly restored
    SREG &= ~(1 << 7);
    os_exec(2, DEFAULT_PRIORITY);
    finished = 0;
    while (!finished) {
        os_yield();
    }
    // Check if SREG has been restored
    if ((SREG & (1 << 7)) != 0) {
        os_error("SREG not restored");
        while (1);
    }
    lcd_writeProgString(PSTR("OK"));
    delayMs(DELAY);
    lcd_clear();

    lcd_writeProgString(PSTR("2: Checking quotient..."));
    delayMs(DELAY);
    // Re-enable interrupts
    SREG |= (1 << 7);

    //select first active strat
    while(!results[current_strat].active && current_strat+1 < 6){
        current_strat++;
    }
    if(current_strat == 5 && !results[5].active){
        lcd_clear();
        lcd_writeProgString(PSTR("No Strategys"));
        os_kill(os_getCurrentProc());
    }
    
    os_setSchedulingStrategy(results[current_strat].strat);
    
    yielder_id = os_exec(4, DEFAULT_PRIORITY);
    runner_id = os_exec(3, DEFAULT_PRIORITY);
    printer_id = os_exec(5, DEFAULT_PRIORITY);
    os_exec(6, DEFAULT_PRIORITY);
}

//! Sets GIEB to check if it's correctly cleared again after yield of program 1
PROGRAM(2, DONTSTART) {
    SREG |= (1 << 7);
    finished = 1;
}

//! Not yielding process
PROGRAM(3, DONTSTART) {
    loop(false, &iterations1);
}

//! Yielding program
PROGRAM(4, DONTSTART) {
    loop(true, &iterations2);
}

//!RCOMP Yielding program
PROGRAM(7, DONTSTART) {
    iterations1 = 1;
    iterations2 = 1;
}

//! Average Watchdog
PROGRAM(6, DONTSTART) {
    while(1){
        if(logBase(BASE, iterations2) >= MAX_TEST_ITERATIONS || os_getSchedulingStrategy() == OS_SS_RUN_TO_COMPLETION){
            //killing all test programs to reset runtime variables without 0 divisions etc.
            os_enterCriticalSection();
            os_kill(runner_id);
            os_kill(yielder_id);
            os_kill(printer_id);
            
            results[current_strat].ratio = (iterations1 / iterations2);
            
            //resetting iteration counts for next strat
            iterations1 = 0;
            iterations2 = 0;
            
            lcd_clear();
            if(select_next_strat() == NULL){
                uint8_t failed = 0;
                for(uint8_t strat_runner = 1;strat_runner < 7;strat_runner++){
                    if((results[strat_runner-1].ratio < results[strat_runner-1].cutoff_bot ||
                       results[strat_runner-1].ratio > results[strat_runner-1].cutoff_top) &&
                       results[strat_runner-1].active)
                    {
                        failed = strat_runner-1;
                        break;
                    }
                }
                if(!failed){
                    lcd_writeProgString(PSTR("----SUCCESS-----"));
                    lcd_line2();
                    for (uint8_t iter = 0 ; iter < 6 ;iter++){
                        if(results[iter].active){
                        lcd_writeDec(results[iter].ratio);}
                        else {
                            lcd_writeProgString(PSTR("X"));
                        }
                        if(iter != 5) lcd_writeProgString(PSTR("|"));
                    }
                } else {
                    lcd_writeProgString(PSTR("-----FAILED-----"));
                    lcd_line2();
                    for (uint8_t iter = 0 ; iter < failed+1 ;iter++){
                        if(results[iter].active){
                        lcd_writeDec(results[iter].ratio);}
                        else {
                            lcd_writeProgString(PSTR("D"));
                        }
                        if(iter != failed) lcd_writeProgString(PSTR("|"));
                    }
                    lcd_writeProgString(PSTR("<"));
                }
                
                while(1){};
                break;
            }
            os_setSchedulingStrategy(results[current_strat].strat);
            
            if(os_getSchedulingStrategy() == OS_SS_RUN_TO_COMPLETION){
                //starting yield flag process
                os_exec(7, DEFAULT_PRIORITY);
                //force yield to flag process
                os_yield();
            } else {
                yielder_id = os_exec(4, DEFAULT_PRIORITY);
                runner_id = os_exec(3, DEFAULT_PRIORITY);
                printer_id = os_exec(5, DEFAULT_PRIORITY);
            }
            os_leaveCriticalSection();
        }
    }
}

// Prints the quotient between the iterations with and without yield().
PROGRAM(5, DONTSTART) {
    for (;;) {
        lcd_clear();
        lcd_writeProgString(results[current_strat].name);
        lcd_line2();
        lcd_writeDec(BASE);
        lcd_writeChar('^');
        lcd_writeDec(logBase(BASE, iterations1));
        lcd_writeChar('/');
        lcd_writeDec(BASE);
        lcd_writeChar('^');
        lcd_writeDec(logBase(BASE, iterations2));
        Iterations i1, i2;
        do {
            i1 = iterations1;
            i2 = iterations2;
            lcd_goto(2, 11);
            lcd_writeChar('=');
            if (iterations1 >= iterations2) {
                lcd_writeDec(iterations1 / iterations2);
            } else {
                lcd_writeChar('1');
                lcd_writeChar('/');
                lcd_writeDec(iterations2 / iterations1);
            }
            while (i1 == iterations1 && i2 == iterations2);
        } while (logBase(BASE, i1) == logBase(BASE, iterations1) && logBase(BASE, i2) == logBase(BASE, iterations2));
    }
}