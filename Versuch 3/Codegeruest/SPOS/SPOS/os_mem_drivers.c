#include "os_mem_drivers.h"
#include "defines.h"

// How to make this functions private?

void initSRAM_internal(void) {
	//nothing to do here since the SRAM does not need to be initialized
}

MemValue readSRAM_internal(MemAddr addr) {
	// CHECK IF PERMISSIONS???
	/*
	uint8_t *ptr;
	&(ptr) = addr;
	return *ptr;
	*/
	return *((MemValue*) addr);
}

void writeSRAM_internal(MemAddr addr, MemValue value) {
	// CHECK IF PERMISSIONS???
	/*
	uint8_t *ptr;
	&(ptr) = addr;
	*ptr = value;
	*/
	*((MemValue*) addr) = value;
}

const MemDriver intSRAM__ = {
	.init = &initSRAM_internal,
	.read = &readSRAM_internal,
	.write = &writeSRAM_internal,
	.start = AVR_SRAM_START + HEAP_OFFSET,
	.size = AVR_MEMORY_SRAM / 2 - HEAP_OFFSET
};