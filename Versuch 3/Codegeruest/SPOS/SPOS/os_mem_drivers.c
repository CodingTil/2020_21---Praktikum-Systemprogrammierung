#include "os_mem_drivers.h"

const MemDriver intSRAM__ = {
	.init = initSRAM_internal,
	.read = readSRAM_internal,
	.write = writeSRAM_internal,
	.start = AVR_SRAM_START,
	.size = AVR_MEMORY_SRAM
};

// How to make this functions private?

void initSRAM_internal(void) {
	//nothing to do here since the SRAM does not need to be initialized
}

MemValue readSRAM_internal(MemAddr addr) {
	/*
	uint8_t *ptr;
	&(ptr) = addr;
	return *ptr;
	*/
	return *((uint8_t*) addr)
}

void writeSRAM_internal(MemAddr addr, MemValue value) {
	/*
	uint8_t *ptr;
	&(ptr) = addr;
	*ptr = value;
	*/
	*((uint8_t*) addr) = value;
}