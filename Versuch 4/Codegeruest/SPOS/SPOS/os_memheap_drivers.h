#ifndef OS_MEMHEAP_DRIVERS_H_
#define OS_MEMHEAP_DRIVERS_H_

#include "os_mem_drivers.h"
#include <stddef.h>
#include "util.h"
#include "defines.h"

extern char PROGMEM const intStr[];
extern char PROGMEM const extStr[];

typedef enum AllocStrategy {
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
} AllocStrategy;

typedef struct Heap {
	MemDriver *driver;
	MemAddr map_start;
	uint16_t map_size;
	MemAddr use_start;
	uint16_t use_size;
	AllocStrategy alloc_strategy;
	char const* const name;
	MemAddr last_addr;
	MemAddr[MAX_NUMBER_OF_PROCESSES] first_used;
	MemAddr[MAX_NUMBER_OF_PROCESSES] last_used;
} Heap;

extern Heap intHeap__;
#define intHeap (&intHeap__)

extern Heap extHeap__;
#define extHeap (&extHeap__)

void os_initHeaps(void);
uint8_t os_getHeapListLength(void);
Heap* os_lookupHeap(uint8_t index);

#endif /* OS_MEMHEAP_DRIVERS_H_ */