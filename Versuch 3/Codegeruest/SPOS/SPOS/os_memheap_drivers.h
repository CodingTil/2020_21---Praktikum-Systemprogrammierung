#ifndef OS_MEMHEAP_DRIVERS_H_
#define OS_MEMHEAP_DRIVERS_H_

#include "os_mem_drivers.h"
#include <stddef.h>
#include "util.h"

extern const PROGMEM char internal_name[];

typedef enum AllocStrategy {
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
} AllocStrategy;

typedef struct Heap {
	MemDriver const *driver;
	MemAddr map_start;
	uint16_t map_size;
	MemAddr use_start;
	uint16_t use_size;
	AllocStrategy alloc_strategy;
	const char* name;
} Heap;

Heap intHeap__;
#define intHeap (&intHeap__)

void os_initHeaps(void);
uint8_t os_getHeapListLength(void);
Heap* os_lookupHeap(uint8_t index);

#endif /* OS_MEMHEAP_DRIVERS_H_ */