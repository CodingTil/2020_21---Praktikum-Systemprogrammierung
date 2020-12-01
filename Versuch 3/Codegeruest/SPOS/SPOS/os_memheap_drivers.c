/*
 * os_memheap_drivers.c
 *
 * Created: 01.12.2020 14:09:46
 *  Author: User
 */ 
#include "os_memheap_drivers.h"

void os_initHeaps() {
	for(MemAddr i = intHeap__.start_map; i < intHeap__.start_use; i++) {
		intSRAM.write(i, 0);
	}
}

uint16_t os_getHeapListLength() {
	//TODO: Anzahl der existierenden Heaps zurückgeben
}

Heap* os_lookupHeap(uint8_t index) {
	//TODO: Zeiger auf den Heap mit Index index zurückgeben (intHeap hat Index 0)
}

Heap intHeap__ = {
	.driver = ,
	.start_map = ,
	.size_map = ,
	.start_use = ,
	.size_use = ,
	.heap_strategy = ,
	.name = 
};


