#include "os_memheap_drivers.h"
#include "os_spi.h"

char PROGMEM const intStr[] = "internal";
char PROGMEM const extStr[] = "external";

Heap intHeap__ = {
	.driver = intSRAM,
	.map_start = AVR_SRAM_START + HEAP_OFFSET,
	.map_size = (AVR_MEMORY_SRAM / 2 - HEAP_OFFSET) / 3,
	.use_start = AVR_SRAM_START + HEAP_OFFSET + (AVR_MEMORY_SRAM / 2 - HEAP_OFFSET) / 3,
	.use_size = ((AVR_MEMORY_SRAM / 2 - HEAP_OFFSET) / 3) * 2,
	.alloc_strategy = OS_MEM_FIRST,
	.name = intStr,
	.last_addr = AVR_SRAM_START + HEAP_OFFSET + (AVR_MEMORY_SRAM / 2 - HEAP_OFFSET) / 3
};

Heap extHeap__ = {
	.driver = extSRAM,
	.map_start = SPI_SRAM_START,
	.map_size = SPI_SRAM_SIZE / 3,
	.use_start = SPI_SRAM_START + SPI_SRAM_SIZE / 3,
	.use_size = (SPI_SRAM_SIZE / 3) * 2,
	.alloc_strategy = OS_MEM_FIRST,
	.name = extStr,
	.last_addr = SPI_SRAM_START + SPI_SRAM_SIZE
};

void os_initHeap(Heap heap) {
	for (MemAddr i = heap.map_start; i < heap.use_start; i++) {
		heap.driver->write(i, 0);
	}
}

void os_initHeaps(void) {
	os_initHeap(intHeap__);
	os_initHeap(extHeap__);
}

uint8_t os_getHeapListLength() {
	return 2;
}

Heap* os_lookupHeap(uint8_t index) {
	if (index == 0) { return intHeap; }
	if (index == 1) { return extHeap; }
	return NULL;
}