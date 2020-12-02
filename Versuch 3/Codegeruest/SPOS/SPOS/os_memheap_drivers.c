#include "os_memheap_drivers.h"
#include "os_mem_drivers.h"const PROGMEM char name[] = "internal";

Heap intHeap__;

void os_initHeaps(void) {
	intHeap__.driver = intSRAM;
	intHeap__.map_start = intSRAM->start;
	intHeap__.map_size = intSRAM->size / 3;
	intHeap__.use_start = intSRAM->start + intSRAM->size / 3;
	intHeap__.use_size = intSRAM->size - intSRAM->size / 3;
	intHeap__.name = name;
	for(MemAddr i = intHeap__.map_start; i < intHeap__.use_start; i++) {
		intHeap__.driver->write(i, 0);
	}
}

uint16_t os_getHeapListLength(void){
	return 1;
}

Heap* os_lookupHeap(uint8_t index){
	if(index == 0) return intHeap;
	return NULL;
}