/*
 * os_memheap_drivers.h
 *
 * Created: 01.12.2020 14:08:57
 *  Author: User
 */ 


#ifndef OS_MEMHEAP_DRIVERS_H_
#define OS_MEMHEAP_DRIVERS_H_

typedef enum {
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
}AllocStrategy;

typedef struct {
	MemDriver *driver;
	
	MemAddr start_map;
	size_t size_map;
	MemAddr start_use;
	size_t size_use;
	
	AllocStrategy heap_strategy;
	
	extern const PROGMEM char name [];
	
}Heap;

typedef (struct Heap) Heap;


#endif /* OS_MEMHEAP_DRIVERS_H_ */