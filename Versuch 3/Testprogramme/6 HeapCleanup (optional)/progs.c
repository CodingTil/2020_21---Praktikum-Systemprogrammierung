//-------------------------------------------------
//          TestSuite: HeapCleanup
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"
#include "os_input.h"

#include <avr/interrupt.h>

//! Delay to wait after starting a swarm process
#define DELAY          (100)
//! Delay after printing a phase-notification in ms
#define PRINT_DELAY    (1000)

//! Number of runs of swarm-allocations that are to be performed. Usually 400
#define RUNS           (400)

//! Number of heap-drivers that are to be tested
#define DRIVERS            (os_getHeapListLength())
//! Test if all heaps are used. Should be 1 for V4 and 0 for V3
#define TEST_FOR_ALL_HEAPS (0)
//! This flag defines if phase 1 of this test is executed
#define PHASE_1            (1)
//! This flag defines if phase 2 of this test is executed
#define PHASE_2            (1)
//! This flag defines if phase 3 of this test is executed
#define PHASE_3            (1)
//! This flag defines if phase 4 of this test is executed
#define PHASE_4            (1)
//! The default memory allocation strategy for phase 3
#define DEFAULT_ALLOC_STRATEGY (OS_MEM_FIRST)

//! This flag decides if the first fit allocation strategy is tested
#define CHECK_FIRST    (1)
//! This flag decides if the next fit allocation strategy is tested
#define CHECK_NEXT     (1)
//! This flag decides if the best fit allocation strategy is tested
#define CHECK_BEST     (1)
//! This flag decides if the worst fit allocation strategy is tested
#define CHECK_WORST    (1)

//! Program ID of the swarm allocation program
#define SWARM_ALLOC_PROG (2)
//! Program ID of the small allocation program
#define SMALL_ALLOC_PROG (3)


//! ProcessID of small allocation process
volatile ProcessID allocProc = 0;
//! This variable is set to 1 when the small allocation process is finished
volatile uint8_t   allocProcReadyToDie = 0;
//! This variable is set to true when the test program finishes
bool terminated = false;

/*!
 * Prints an error message together with a corresponding heap on which the
 * error occurred. After that the program halts.
 * \param  *error ProgString error-message
 * \param  *heap  Heap on which error occurred
 */
void hc_errorHeap(char const* error, Heap* heap) {
    lcd_clear();
    lcd_writeProgString(error);
    lcd_writeProgString(heap->name);
    lcd_writeChar('!');
    while (1) {}
}

/*!
 * Prints an error message and halts execution.
 * \param  *error ProgString error-message
 */
void hc_error(char const* error) {
    lcd_clear();
    lcd_writeProgString(error);
    while (1) {}
}

/*!
 * Starts the small allocation process and waits until it finished its
 * allocations.
 */
void hc_startSmallAllocator(void) {
    allocProcReadyToDie = 0;
    os_exec(SMALL_ALLOC_PROG, DEFAULT_PRIORITY);
    while (!allocProcReadyToDie) {
        //wait until the allocation process allocated its memory
    }
}

/*!
 * Kills the small allocation process.
 */
void hc_killSmallAllocator(void) {
    os_kill(allocProc);
}

/*!
 * Starts as many processes of the swarm-allocation program as possible.
 */
void hc_startAllocatorSwarm(void) {
    while (os_getNumberOfActiveProcs() < MAX_NUMBER_OF_PROCESSES) {
        os_exec(SWARM_ALLOC_PROG, DEFAULT_PRIORITY);
    }
}

/*!
 * Waits until all started swarm-processes are dead.
 */
void hc_waitForSwarmKill(void) {
    /* 2 because: 1 idle-process, 1 main test-process */
    while (os_getNumberOfActiveProcs() != 2) {}
}

/*!
 * Waits until a process-slot is free.
 */
void hc_waitForFreeProcSlot(void) {
    while (os_getNumberOfActiveProcs() == MAX_NUMBER_OF_PROCESSES);
}

//! Main test routine. Executes the single phases.
PROGRAM(1, AUTOSTART) {    
    // Init Timeout Timer 1
    // CTC mode
    TCCR1A &= ~(1 << WGM10);
    TCCR1A &= ~(1 << WGM11);
    TCCR1B |= (1 << WGM12);
    TCCR1B &= ~(1 << WGM13);

	// set prescaler to 1024
	TCCR1B |=  (1 << CS12) | (1 << CS10);
	TCCR1B &= ~(1 << CS11);

	// set compare register -> match every 1s
	OCR1A = 19531;

	// enable timer
	TIMSK1 |= (1 << OCIE1A);
    uint8_t i;

    // Check if heaps are defined correctly
#if TEST_FOR_ALL_HEAPS==1
    if (os_getHeapListLength() != 2 || os_lookupHeap(0) == os_lookupHeap(1)) {
        hc_error(PSTR("Missing heap!"));
    }
#endif

// 1. Phase: Test if you can over-allocate memory
#if PHASE_1 == 1
    lcd_clear();
    lcd_writeProgString(PSTR("Phase 1:"));
    lcd_line2();
    lcd_writeProgString(PSTR("Overalloc     "));
    delayMs(PRINT_DELAY);

    // First allocate a small amount of memory on each heap. This should make
    // it impossible to allocate the whole use size in the next step
    hc_startSmallAllocator();
    // Then we try to alloc all the memory on one heap. If it works, the
    // student's solution is faulty.
    for (i = 0; i < DRIVERS; i++) {
        Heap*     heap  = os_lookupHeap(i);
        uint16_t  total = os_getUseSize(heap);
        if (os_malloc(heap, total)) {
            hc_errorHeap(PSTR("Overalloc failure in "), heap);
        }
    }
    hc_killSmallAllocator();

    lcd_writeProgString(PSTR("OK"));
    delayMs(PRINT_DELAY);
#endif

// 2. Phase: Settings test: Check if heap is set up correctly
#if PHASE_2 == 1
    lcd_clear();
    lcd_writeProgString(PSTR("Phase 2:"));
    lcd_line2();
    lcd_writeProgString(PSTR("Heap settings "));
    delayMs(PRINT_DELAY);

    // This address is certainly in the stacks.
    MemValue* startOfStacks = (uint8_t*)((AVR_MEMORY_SRAM / 2) + AVR_SRAM_START);

    /*
     * If we allocate all the memory and write in it, we should not be able to
     * write into that stack memory. If we can, the internal heap is initialized
     * with wrong settings
     */
    os_setAllocationStrategy(intHeap, OS_MEM_FIRST);
    MemAddr  hugeChunk = os_malloc(intHeap, os_getUseSize(intHeap));
    if (!hugeChunk) {
        hc_errorHeap(PSTR("f:Huge alloc fail in "), intHeap);
    }

    /*
     * We will now write a pattern into RAM and check if they change the
     * contents of the stack. To check that, we check the top of the stack of
     * process 8. This can be done because there is no process 8
     */
    MemAddr currentAddress;
    *startOfStacks = 0x00;
    for (currentAddress = hugeChunk; currentAddress < hugeChunk + os_getUseSize(intHeap); currentAddress++) {
        intHeap->driver->write(currentAddress, 0xFF);
    }

    if (*startOfStacks != 0x00) {
        hc_error(PSTR("Internal heap   too large"));
    }

    // Clean up
    os_free(intHeap, hugeChunk);
    lcd_goto(2, 15);
    lcd_writeProgString(PSTR("OK"));
    delayMs(PRINT_DELAY);

#endif

// 3. Phase: Check if we can allocate the whole use-area after killing a RAM-hogger
#if PHASE_3 == 1
    lcd_clear();
    lcd_writeProgString(PSTR("Phase 3:"));
    lcd_line2();
    lcd_writeProgString(PSTR("Huge alloc    "));
    delayMs(PRINT_DELAY);

    for (i = 0; i < DRIVERS; i++) {
        Heap*   heap = os_lookupHeap(i);
        MemAddr hugeChunk;

        /*
         * For each allocation strategy the following two steps are executed
         * 1. Make a RAM-hogger and kill it, such that the heap-cleaner should
         *    have freed its memory
         * 2. Test if this is correct with a certain strategy
         */

        lcd_goto(1, 11);
        lcd_writeDec(i);
        lcd_writeProgString(PSTR(":    "));
        lcd_goto(1, 13);
#if CHECK_BEST == 1
        lcd_writeChar('b');
        hc_startSmallAllocator();
        hc_killSmallAllocator();
        os_setAllocationStrategy(heap, OS_MEM_BEST);
        hugeChunk = os_malloc(heap, os_getUseSize(heap));
        if (!hugeChunk) {
            hc_errorHeap(PSTR("b:Huge alloc fail in "), heap);
        } else {
            os_free(heap, hugeChunk);
        }
#endif

#if CHECK_WORST == 1
        lcd_writeChar('w');
        hc_startSmallAllocator();
        hc_killSmallAllocator();
        os_setAllocationStrategy(heap, OS_MEM_WORST);
        hugeChunk = os_malloc(heap, os_getUseSize(heap));
        if (!hugeChunk) {
            hc_errorHeap(PSTR("w:Huge alloc fail in "), heap);
        } else {
            os_free(heap, hugeChunk);
        }
#endif

#if CHECK_FIRST == 1
        lcd_writeChar('f');
        hc_startSmallAllocator();
        hc_killSmallAllocator();
        os_setAllocationStrategy(heap, OS_MEM_FIRST);
        hugeChunk = os_malloc(heap, os_getUseSize(heap));
        if (!hugeChunk) {
            hc_errorHeap(PSTR("f:Huge alloc fail in "), heap);
        } else {
            os_free(heap, hugeChunk);
        }
#endif

#if CHECK_NEXT == 1
        lcd_writeChar('n');
        hc_startSmallAllocator();
        hc_killSmallAllocator();
        os_setAllocationStrategy(heap, OS_MEM_NEXT);
        hugeChunk = os_malloc(heap, os_getUseSize(heap));
        if (!hugeChunk) {
            hc_errorHeap(PSTR("n:Huge alloc fail in "), heap);
        } else {
            os_free(heap, hugeChunk);
        }
#endif
    }

    lcd_goto(2, 15);
    lcd_writeProgString(PSTR("OK"));
    delayMs(PRINT_DELAY);
#endif

// 4. Phase: Main test: Check if heap is cleaned up
#if PHASE_4 == 1
    lcd_clear();
    lcd_writeProgString(PSTR("Phase 4:"));
    lcd_line2();
    lcd_writeProgString(PSTR("Heap Cleanup  "));
    delayMs(PRINT_DELAY);

    // We want to use the default strategy on every heap. After phase 2 we
    // cannot know which strategy was last used.
    for (i = 0; i < DRIVERS; i++) {
        os_setAllocationStrategy(os_lookupHeap(i), DEFAULT_ALLOC_STRATEGY);
    }

    /*
     * In this phase a swarm of allocation-process is started. These processes
     * terminate. Thus heap-cleanup should be performed. Whenever a process
     * terminates, a new one is started in his stead. Every 50 steps we will
     * check, if the cleanup was successful.
     */
    hc_startAllocatorSwarm();

    uint16_t       runs;
    uint16_t const totalRuns = RUNS;

    for (runs = 0; runs < totalRuns; runs++) {
        // Every run we start a new allocator and give it some time to allocate
        // memory
        hc_waitForFreeProcSlot();

        os_exec(SWARM_ALLOC_PROG, DEFAULT_PRIORITY);
        delayMs(DELAY);


        lcd_drawBar((100 * runs) / totalRuns);
        lcd_goto(2, 6);
        lcd_writeDec(runs + 1);
        lcd_writeChar('/');
        lcd_writeDec(totalRuns);

        /*
         * Every 50 steps we test if the memory get freed correctly after a
         * kill. For that, we wait for every big allocator to get killed and
         * then check if the whole memory is free again
         */
        if ((runs + 1) % 50 == 0) {
            hc_waitForSwarmKill();
            for (i = 0; i < DRIVERS; i++) {
                Heap*    heap      = os_lookupHeap(i);
                MemAddr  hugeChunk = os_malloc(heap, os_getUseSize(heap));
                if (!hugeChunk) {
                    // We could not allocate all the memory on that heap, so there was
                    // a leak
                    hc_errorHeap(PSTR("Heap not clean: "), heap);
                } else {
                    os_free(heap, hugeChunk);
                }
            }
            // Since we killed the whole swarm, we have to revive it again before
            // we jump back to our loop
            hc_startAllocatorSwarm();
        }
    }
    hc_waitForSwarmKill();

    lcd_clear();
    lcd_writeProgString(PSTR("Phase 4:"));
    lcd_line2();
    lcd_writeProgString(PSTR("Heap Cleanup  "));
    lcd_writeProgString(PSTR("OK"));
    delayMs(PRINT_DELAY);

#endif


    // SUCCESS
    terminated = true;
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

/*!
 * Swarm allocation program. This program tries to allocate all bytes in
 * every heap. This is done by allocating less and less memory in each step.
 * Due to this other processes will be able to allocate memory, too. This
 * yields a very fragmented memory.
 */
PROGRAM(SWARM_ALLOC_PROG, DONTSTART) {
    uint8_t i;
    for (i = 0; i < DRIVERS; i++) {
        Heap* heap = os_lookupHeap(i);
        // The number 4 is a heuristic. We expect the ram not to be that
        // fragmented.
        uint16_t max = os_getUseSize(heap) / (MAX_NUMBER_OF_PROCESSES - 3) / 4;
        while (max) {
            // Allocate less and less memory with every step
            uint16_t const sz = (max + 1) / 2;
            max -= sz;
            os_malloc(heap, sz);
        }
    }
}

/*!
 * Small allocation program that tries to allocate 2 times 4 bytes on every
 * heap. After that it signals that it is finished and it waits in an infinite
 * loop until it is externally killed.
 */
PROGRAM(SMALL_ALLOC_PROG, DONTSTART) {
    allocProc = os_getCurrentProc();
    for (uint8_t i = 0; i < DRIVERS; i++) {
        os_malloc(os_lookupHeap(i), 4);
        os_malloc(os_lookupHeap(i), 4);
    }
    allocProcReadyToDie = 1;
    while (1) {}
}

/*
 *	Timeout ISR
 */
ISR(TIMER1_COMPA_vect)
{
    // counts seconds
    static int16_t timeout_counter = 0;
    if(++timeout_counter >= 900 && !terminated){
        hc_error(PSTR("Test timed out after 15 minutes!"));
    }
}
