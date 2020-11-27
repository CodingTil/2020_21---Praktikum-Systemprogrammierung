#include "os_memheap_drivers.h"
#include "os_mem_drivers.h"

const PROGMEM char name[] = "internal";

Heap intHeap__ = {
	intHeap__.driver = intSRAM,
	intHeap__.map_start = intSRAM.start,
	intHeap__.map_size = intSRAM.size / 3,
	intHeap__.use_start = intHeap__.map_start + intHeap__.map_size,
	intHeap__.use_size = intSRAM.size - intSRAM.size / 3
};

void os_initHeaps(void) {
	for(MemAddr i = intHeap__.map_size; i < intHeap__.use_start; i++) {
		intSRAM.write(i, 0);
	}
}

uint16_t os_getHeapListLength(void){
	return 1;
}

Heap* os_lookupHeap(uint8_t index){
	if(index == 0) return intHeap;
}