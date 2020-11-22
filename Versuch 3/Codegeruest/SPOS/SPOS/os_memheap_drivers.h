#ifndef OS_MEMHEAP_DRIVERS_H_
#define OS_MEMHEAP_DRIVERS_H_

#include "os_memheap_drivers.h"

#define intHeap &(intHeap__);

typedef enum {
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
}AllocStrategy;

typedef struct {
	MemDriver *driver;
	MemAddr map_start;
	unsigned long map_size;
	MemAddr use_start;
	unsigned long use_size;
	AllocStrategy alloc_strategy;
	extern const PROGMEM char name[];
}Heap;

void os_initHeaps(void);

#endif /* OS_MEMHEAP_DRIVERS_H_ */