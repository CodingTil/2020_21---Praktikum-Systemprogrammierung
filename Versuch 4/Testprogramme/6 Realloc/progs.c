//-------------------------------------------------
//          TestSuite: Realloc
//-------------------------------------------------

#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"
#include "os_input.h"
#include "defines.h"

#include <avr/interrupt.h>
#include <stdlib.h>

#define DELAY 100

// Calculates the minimum of a and b
#define MIN(a,b) ( (a)<(b) ? (a) : (b) )

// Error printout
#define error_onHeap(HEAP, MSG) \
    if(HEAP == intHeap) \
        os_error(MSG " on intHeap"); \
    else \
        os_error(MSG " on extHeap");\

// Flag indicating if the internal realloc check is done
bool volatile checkI = false;
// Flag indicating if the external realloc check is done
bool volatile checkE = false;
// Flag indicating if the external realloc check discovered failures
bool failI = false;
// Flag indicating if the internal realloc check discovered failures
bool failE = false;

typedef struct {
    MemAddr head; // First address of that chunk
    size_t size;  // Size of chunk
    bool dummy;   // 1 if this chunk is a dummy
} Chunk;

/*!
 * Returns the mapEntry of the use-address ptr
 * \param  *heap Heap on which the map is to be checked
 * \param   ptr  Use-address whose map-entry is requested
 * \return       Entry for that position (between 0x0 and 0xF)
 */
uint8_t progs_getMapEntry(Heap* heap, MemAddr ptr) {
    MemAddr relativeAddress = ptr - os_getUseStart(heap);
    // If nibble is 1, the high nibble is responsible
    uint8_t nibble = relativeAddress % 2 == 0 ? 1 : 0;

    MemAddr mapEntryAddress = relativeAddress / 2;

    MemValue fullEntry = heap->driver->read(os_getMapStart(heap) + mapEntryAddress);

    if (nibble == 1) {
        return fullEntry >> 4;
    } else {
        return fullEntry & 0x0F;
    }
}

/*!
 * Returns the chunk-size of a chunk that starts at ptr. Only works for chunks
 * that were allocated as private memory by one of the processes.
 * \param  *heap Heap where the chunk is
 * \param   ptr  Address of the very first byte of that chunk
 * \return       Length of chunk in bytes
 */
uint16_t progs_getChunkSize(Heap* heap, MemAddr ptr) {
    uint16_t size;
    MemValue owner;
    owner = progs_getMapEntry(heap, ptr);
    if (owner == 0 || owner >= MAX_NUMBER_OF_PROCESSES) {
        return 0;
    }
    for (size = 1; progs_getMapEntry(heap, ptr + size) == 0xF; size++) {
        // Nop
    }
    return size;
}

/*!
 * Writes a pattern to a certain chunk
 * \param  *heap   Heap on which this action is to be performed
 * \param  *c      Chunk to which the pattern is to be written
 * \param   pat    Pattern that is written. This byte gets repeatedly written
 *                 to the chunk until the whole chunk is filled with it.
 */
void writePat(Heap* heap, Chunk* c, MemValue pat) {
    size_t i;
    for (i = 0; i < c->size; i++) {
        heap->driver->write(c->head + i, pat);
    }
}

/*!
 * Checks if a certain chunk contains a given pattern.
 * \param  *heap   On which heap the check is performed.
 * \param   c      Chunk that should contain the pattern
 * \param   pat    Pattern that should be found in that chunk
 * \return         1 if the pattern exists at that position, 0 otherwise
 */
bool checkChunkPat(Heap* heap, Chunk* c, MemValue pat) {
    size_t i;
    for (i = 0; i < c->size; i++) {
        if (heap->driver->read(c->head + i) != pat) {
            return false;
        }
    }
    return true;
}

/*!
 * Checks if a certain area in the memory contains a given pattern.
 * \param  *heap   On which heap the check is performed.
 * \param   start  First address where the pattern starts
 * \param   length Length of the memory area that is to be checked
 * \param   pat    Pattern that should be found in that area
 * \return         1 if the pattern exists at that position, 0 otherwise
 */
bool checkMemPat(Heap* heap, MemAddr start, MemAddr length, MemValue pat) {
    size_t i;
    for (i = 0; i < length; i++) {
        if (heap->driver->read(start + i) != pat) {
            return false;
        }
    }
    return true;
}

/*!
 * Allocates all chunks in the given chunk array, writes a pattern to each
 * chunk and deallocates all dummies.
 * \param  *heap   Heap on which this action is to be performed
 * \param  *chunks Array of chunks that are ot be allocated/freed
 * \param   count  Size of chunks-array
 */
void massAlloc(Heap* heap, Chunk* chunks, size_t count) {
    size_t i;

    // Allocation phase
    for (i = 0; i < count; i++) {
        chunks[i].head = os_malloc(heap, chunks[i].size);
        if (!chunks[i].head) {
            error_onHeap(heap, "malloc failure");
        }
        writePat(heap, chunks + i, i);
    }

    // Free phase
    for (i = 0; i < count; i++) {
        if (chunks[i].dummy) {
            os_free(heap, chunks[i].head);
        }
    }
}

/*!
 * Frees all non dummy chunks of the given chunk array.
 * \param  *heap   Heap on which this action is to be performed
 * \param  *chunks Array of chunks that are ot be allocated/freed
 * \param   count  Size of chunks-array
 */
void massFree(Heap* heap, Chunk* chunks, size_t count) {
    size_t i;
    for (i = 0; i < count; i++) {
        if (!chunks[i].dummy) {
            os_free(heap, chunks[i].head);
        }
    }
}

/*!
 * Main test routine
 * In this routine, several chunks are allocated and a pattern is written to
 * them. Then those chunks labeled as dummies are freed again. After that
 * the middle chunk (i.e. with 7 chunks the 3rd chunk) is reallocated to a
 * new size. That size is the sum of the sizes of all chunks labeled as
 * realloc candidates in the reallocBits bitmap.
 * After that reallocation it is checked if the map is correctly representing
 * that reallocation and if the other chunks were not damaged in the process.
 *
 * \param  *heap        Heap to test.
 * \param   dummies     Dummies are those chunks that get freed after all
 *                      chunks were allocated. This bitmap defines, which
 *                      chunk is a dummy. The LSB defines whether the last
 *                      allocated chunk is freed and the MSB defines whether
 *                      the first allocated chunk is freed, etc.
 * \param   reallocBits This bitmap is set up like dummies. Except in this map
 *                      it is defined whose chunks are going to be merged by
 *                      the reallocation.
 * \param   random      If this byte is 0xFF, the new size of the reallocation-
 *                      chunk is calculated deterministically. If it is not
 *                      0xFF, the reallocation-chunk is additionally randomly
 *                      increased for their reallocation. That random value is
 *                      at least 1 and smaller than the size of the chunk with
 *                      index "random".
 * \param   destination This bitmap defines in which chunk the resulting
 *                      reallocated chunk must be located.
 */
bool makeTest(Heap* heap, uint8_t dummies, uint8_t reallocBits,
              uint8_t random, uint8_t destination) {


    os_setAllocationStrategy(heap, OS_MEM_FIRST);

    size_t i;
    size_t useSize = os_getUseSize(heap);
    if(useSize > 4096) useSize = 4096;

    // This macro returns 1 for all chunks that are labeled as dummy chunks
#define IS_DUMMY(i)               ((dummies >> (6-i)) & 1)
    // This macro returns 1 for all chunks that are labeled as reallocation candidates
#define IS_REALLOC_CANDIDATE(i)   ((reallocBits >> (6-i)) & 1)
    // This macro returns 1 for all chunks that are still untouched after their initial allocation
#define IS_PERSISTENT(i)          (!(IS_DUMMY(i) || IS_REALLOC_CANDIDATE(i)))


    // 1. Preparation of chunks

    /*
     * In this chunk array, 7 chunks will be prepared. Chunks 0,2,3,4 have semi-
     * random sizes, the other sizes are fixed. Chunk 6 has a size as big as the
     * whole use-size. This is only temporary, as its size will be corrected
     * below
     */
    Chunk chunks[] = {
        {.size = rand() % (useSize / 6) + useSize / 6/* useSize/6 <= size < 2*useSize/6 */, .dummy = IS_DUMMY(0) },
        {.size = 1, .dummy = IS_DUMMY(1) },
        {.size = rand() % (useSize / 6) + 1   /* 1 <= size <= useSize/6 */, .dummy = IS_DUMMY(2) },
        {.size = rand() % (useSize / 6) + 1   /* 1 <= size <= useSize/6 */, .dummy = IS_DUMMY(3) },
        {.size = rand() % (useSize / 6) + 1   /* 1 <= size <= useSize/6 */, .dummy = IS_DUMMY(4) },
        {.size = 1, .dummy = IS_DUMMY(5) },
        {.size = useSize, .dummy = IS_DUMMY(6) }
    };

    // Now resize last chunk so that the sum of the chunk sizes equals the total use size.
    size_t const cSize = sizeof(chunks) / sizeof(*chunks); // Size of chunks[]
    size_t const reallocIndex = cSize / 2; // Index of block we want to reallocate

    for (i = 0; i < cSize - 1; i++) {
        chunks[cSize - 1].size -= chunks[i].size;
    }


    // 2. Allocation of chunks

    // Allocate all prepared chunks. Then a pattern is written to the chunks and
    // those chunks flagged as dummies are freed
    massAlloc(heap, chunks, cSize);


    // 3. Reallocation of middle chunk

    size_t newSize = 0;
    // The new size is calculated as the sizes of all those chunks that are
    // labeled as realloc candidates
    for (i = 0; i < cSize; i++) {
        // Add chunk size to new size if reallocate bit is 1
        if (IS_REALLOC_CANDIDATE(i)) {
            newSize += chunks[i].size;
        }
    }
    // Add some random size to new size
    if (random != 0xFF) {
        size_t add = rand() % (chunks[random].size);
        if (add < 1) {
            add = 1;
        }
        // 1 <= add < chunks[random].size
        newSize += add;
    }

    // Reallocate the chunk with index reallocIndex to newSize
    Chunk const old = chunks[reallocIndex];
    chunks[reallocIndex].size = newSize;
    chunks[reallocIndex].head = os_realloc(heap, chunks[reallocIndex].head, newSize);


    // 4. Check if os_realloc returns a valid address

    // Check if os_realloc found a valid position for the new chunk
    if (chunks[reallocIndex].head == 0) {
        error_onHeap(heap, "realloc allocation fail");
    }


    // 5. Check if memcpy was successful

    // Confirm the written pattern is present in the reallocated block (memcpy)
    if (!checkMemPat(heap, chunks[reallocIndex].head, MIN(old.size, newSize), reallocIndex)) {
        error_onHeap(heap, "realloc memcpy fail");
    }


    // 6. Check if the map was adapted correctly

    // Compute the actual size of the reallocated chunk by iterating the map
    if (progs_getChunkSize(heap, chunks[reallocIndex].head) != newSize) {
        error_onHeap(heap, "realloc map adapt. fail");
    }

    // Compute the size of the untouched chunks
    for (i = 0; i < cSize; i++) {
        if (IS_PERSISTENT(i)) {
            if (progs_getChunkSize(heap, chunks[i].head) != chunks[i].size) {
                error_onHeap(heap, "realloc map damage");
            }
        }
    }


    // 7. Check if the new chunk overlaps with other chunks

    // Rewrite the pattern in the reallocated chunk.
    writePat(heap, chunks + reallocIndex, reallocIndex);

    // Check all patterns of non dummy chunks.
    // Therefore checking if reallocation interfered with any other chunk.
    for (i = 0; i < cSize; i++) {
        if (!chunks[i].dummy && !checkChunkPat(heap, chunks + i, i)) {
            error_onHeap(heap, "realloc pattern broken");
        }
    }


    // 8. Check if the chunk is at the correct position

    bool found = false;
    for (i = 0; !found && i < cSize; i++) {
        if ((1 << (6 - i)) & destination) {
            if (i == reallocIndex) {
                found = (chunks[reallocIndex].head == old.head);
            } else {
                found = (chunks[reallocIndex].head == chunks[i].head);
            }
        }
    }


    // 9. Clean up

    // Free remaining chunks
    massFree(heap, chunks, cSize);

    return found;


#undef  IS_PERSISTENT
#undef  IS_REALLOC_CANDIDATE
#undef  IS_DUMMY
}

/*!
 * Print test message.
 * \param   i    Index of test
 * \param  *msg  Name of test
 * \param  *heap Heap on which the test is performed
 */
void test(char i, char const* msg, Heap* heap) {

    static char PROGMEM const spaces16[] = "                ";
    os_enterCriticalSection();
    if (heap == intHeap) {
        lcd_line1();
        lcd_writeProgString(spaces16);
        lcd_line1();
        lcd_writeChar(i);
        lcd_writeProgString(PSTR("i:"));
    }
    if (heap == extHeap) {
        lcd_line2();
        lcd_writeProgString(spaces16);
        lcd_line2();
        lcd_writeChar(i);
        lcd_writeProgString(PSTR("e:"));
    }
    lcd_writeProgString(msg);
    os_leaveCriticalSection();
    delayMs(5 * DELAY);
}

/*!
 * If a reallocation test was successful, this prints "OK" in the correct line
 * \param  *heap Heap on which the test was successful
 */
void ok(Heap* heap) {
    os_enterCriticalSection();
    if (heap == intHeap) {
        lcd_goto(1, 15);
    }
    if (heap == extHeap) {
        lcd_goto(2, 15);
    }
    lcd_writeProgString(PSTR("OK"));
    os_leaveCriticalSection();
    delayMs(5 * DELAY);
}

/*!
 * If a reallocation test fails, this function prints "FAIL" and labels the
 * whole test on that heap as failed.
 * \param  *heap Heap where the test failed
 */
void fail(Heap* heap) {
    os_enterCriticalSection();
    if (heap == intHeap) {
        lcd_goto(1, 13);
        failI = true;
    }
    if (heap == extHeap) {
        lcd_goto(2, 13);
        failE = true;
    }
    lcd_writeProgString(PSTR("FAIL"));
    os_leaveCriticalSection();
    delayMs(5 * DELAY);
}

/*!
 * Runs the tests on one of the heaps.
 * \param  *heap Heap on which the tests are run.
 */
void runTest(Heap* heap) {
    // Expand by the chunk to the right.
    // Use free memory to the right.
    test('1', PSTR("R.res."), heap);
    if (makeTest(heap, 0b1110101, 0b0001100, 0xFF, 0b0001000)) {
        ok(heap);
    } else {
        error_onHeap(heap, "R.res. fail");
        fail(heap);
    }

    // Expand by the chunk to the left
    // Use free memory to the left or free and malloc.
    test('2', PSTR("L.res."), heap);
    if (makeTest(heap, 0b1010011, 0b0011000, 0xFF, 0b0010000)) {
        ok(heap);
    } else {
        error_onHeap(heap, "L.res. fail");
        fail(heap);
    }

    // Force expansion by one chunk to the left.
    test('3', PSTR("L.res.frc."), heap);
    if (makeTest(heap, 0b0010000, 0b0011000, 0xFF, 0b0010000)) {
        ok(heap);
    } else {
        error_onHeap(heap, "L.res.frc. fail");
        fail(heap);
    }

    // Expand by a bit more than the chunk to the right.
    // Use free memory to the left and right or free and malloc.
    test('4', PSTR("LR res."), heap);
    if (makeTest(heap, 0b1010101, 0b0001100, 2, 0b0010000)) {
        ok(heap);
    } else {
        error_onHeap(heap, "LR res. fail");
        fail(heap);
    }

    // Expand by a bit more than the free memory to the right.
    // Force expansion to the left and right.
    test('5', PSTR("LR res.frc."), heap);
    if (makeTest(heap, 0b0010100, 0b0001100, 2, 0b0010000)) {
        ok(heap);
    } else {
        error_onHeap(heap, "LR res.frc. fail");
        fail(heap);
    }

    // Expand chunk to roughly double size without having free memory left or right.
    // Thereby forcing to move (free and malloc) the chunk.
    test('6', PSTR("Move"), heap);
    if (makeTest(heap, 0b1100001, 0b1000000, 0xFF, 0b1000001)) {
        ok(heap);
    } else {
        error_onHeap(heap, "Move fail");
        fail(heap);
    }

    // Shrink chunk to randomly chosen smaller size.
    test('7', PSTR("Shrink"), heap);
    if (makeTest(heap, 0b1110111, 0b0000000, 3, 0b0001000)) {
        ok(heap);
    } else {
        error_onHeap(heap, "Shrink fail");
        fail(heap);
    }

    // Keep the chunk as is.
    test('8', PSTR("Keep"), heap);
    if (makeTest(heap, 0b1110111, 0b0001000, 0xFF, 0b0001000)) {
        ok(heap);
    } else {
        error_onHeap(heap, "Keep fail");
        fail(heap);
    }
}


PROGRAM(1, AUTOSTART) {
    runTest(intHeap);
    checkI = true;
}

PROGRAM(2, AUTOSTART) {
    runTest(extHeap);
    checkE = true;
}

PROGRAM(3, AUTOSTART) {
    while (!checkI || !checkE);

    if (!failE && !failI) {
		// SUCCESS
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
    } else {
        lcd_clear();
        lcd_writeProgString(PSTR("  TEST FAILED!"));
        while (1);
    }
}
