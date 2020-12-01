/*
 * os_mem_drivers.c
 *
 * Created: 26.11.2020 14:28:51
 *  Author: User
 */ 
#include "os_mem_drivers.h"

void init(void){
	//TODO: initialize storage medium
	
}

MemValue read(MemAddr addr){
	//get value at addr and return
	return &addr;
}

void write(MemAddr addr, MemValue value){
	//store value at addr
	&addr = value;
}

//struct MemDriver intSRAM__;

MemDriver intSRAM__ = {
	.start = AVR_SRAM_START,
	.size = AVR_MEMORY_SRAM,
	.init = init,
	.read = read,
	.write = write
};
