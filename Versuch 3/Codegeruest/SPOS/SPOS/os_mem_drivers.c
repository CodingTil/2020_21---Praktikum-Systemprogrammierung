#include "os_mem_drivers.h"

MemDriver intSRAM__ = {
	.init = initSRAM_internal,
	.read = readSRAM_internal,
	.write = writeSRAM_internal,
	.start = AVR_SRAM_START,
	.size = AVR_MEMORY_SRAM
};

void *initSRAM_internal(void) {};
	
MemValue *readSRAM_internal(MemAddr addr) {
	return *((uint8_t*) addr);
}

void *writeSRAM_internal(MemAddr addr, MemValue){
	*((uint8_t*) addr) = value;
}

