#include "os_mem_drivers.h"
#include "defines.h"

void initSRAM_internal(void) {};
	
MemValue readSRAM_internal(MemAddr addr) {
	return *((MemValue*) addr);
}

void writeSRAM_internal(MemAddr addr, MemValue value){
	*((MemValue*) addr) = value;
}

MemDriver intSRAM__ = {
	.init = &initSRAM_internal,
	.read = &readSRAM_internal,
	.write = &writeSRAM_internal,
	.start = AVR_SRAM_START,
	.size = AVR_MEMORY_SRAM
};

