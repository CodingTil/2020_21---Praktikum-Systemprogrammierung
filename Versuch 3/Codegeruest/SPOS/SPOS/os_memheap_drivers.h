#ifndef OS_MEMHEAP_DRIVERS_H_
#define OS_MEMHEAP_DRIVERS_H_

#include "os_mem_drivers.h"

typedef enum AllocStrategy {
	OS_MEM_FIRST;
	OS_MEM_NEXT;
	OS_MEM_BEST;
	OS_MEM_WORST;
} AllocStrategy;

typedef struct Heap {
	MemDriver *driver;
	MemAddr map_start;
	uint16_t map_size;
	MemAddr use_start;
	uint16_t use_size;
	AllocStrategy alloc_strategy;
	PROGMEN char *name;
} Heap;

#endif /* OS_MEMHEAP_DRIVERS_H_ */