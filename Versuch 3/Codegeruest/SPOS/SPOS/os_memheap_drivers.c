#include "os_memheap_drivers.h"

const PROGMEM char internal_name[] = "internal";

void os_initHeap(Heap heap) {
	for (MemAddr i = heap.map_start; i < heap.use_start; i++) {
		heap.driver->write(i, 0);
	}
}

void os_initHeaps(void) {
	intHeap__.driver = intSRAM;
	intHeap__.map_start = intSRAM->start;
	intHeap__.map_size = intSRAM->size / 3;
	intHeap__.use_start = intSRAM->start + intSRAM->size / 3;
	intHeap__.use_start = intSRAM->size * 2 / 3;
	intHeap__.alloc_strategy = OS_MEM_FIRST;
	intHeap__.name = internal_name;
	os_initHeap(intHeap__);
}

uint8_t os_getHeapListLength() {
	return 1;
}

Heap* os_lookupHeap(uint8_t index) {
	if (index == 0) { return intHeap; }
	return NULL;
}