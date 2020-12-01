/*
 * os_memheap_drivers.h
 *
 * Created: 01.12.2020 14:08:57
 *  Author: User
 */ 


#ifndef OS_MEMHEAP_DRIVERS_H_
#define OS_MEMHEAP_DRIVERS_H_

#define intHeap *(intHeap__);

typedef enum {
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
}AllocStrategy;

typedef struct {
	MemDriver *driver;
	
	MemAddr map_start;
	size_t map_size;
	MemAddr use_start;
	size_t use_size;
	
	AllocStrategy heap_strategy;
	
	extern const PROGMEM char name [];	
}Heap;

typedef (struct Heap) Heap;

//Functions
void os_initHeaps(void);
uint16_t os_getHeapListLength(void);
Heap* os_lookupHeap(uint8_t index);


#endif /* OS_MEMHEAP_DRIVERS_H_ */