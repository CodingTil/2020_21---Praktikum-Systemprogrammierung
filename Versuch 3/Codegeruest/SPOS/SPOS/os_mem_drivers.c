#include "os_mem_drivers.h"

MemDriver intSRAM__;

void *initSRAM_internal(void) {
	intSRAM__ = {
		.init = initSRAM_internal,
		.read = readSRAM_internal,
		.write = writeSRAM_internal,
		.start = AVR_SRAM_START,
		.size = AVR_MEMORY_SRAM
	};
;
MemValue *readSRAM_internal(MemAddr addr);
void *writeSRAM_internal(MemAddr addr, MemValue);

