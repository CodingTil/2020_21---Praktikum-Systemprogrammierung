#include "os_mem_drivers.h"
#include "defines.h"
#include "os_spi.h"
#include "util.h"
#include "os_scheduler.h"

void select_memory() {
	sbi(PORTB, SPI_CS);
}

void deselect_memory() {
	cbi(PORTB, SPI_CS);
}

void set_operation_mode(uint8_t mode) {
	select_memory();
	os_spi_send(SPI_INS_WRMR);
	os_spi_send(mode<<6);
	deselect_memory();
}

void transfer_adress(MemAddr addr) {
	os_spi_send(0);
	os_spi_send((uint8_t) (addr >> 8));
	os_spi_send((uint8_t) (addr & 0x0F));
}

void initSRAM_internal(void) {
}

MemValue readSRAM_internal(MemAddr addr) {
	return *((MemValue*) addr);
}

void writeSRAM_internal(MemAddr addr, MemValue value) {
	*((MemValue*) addr) = value;
}

MemDriver intSRAM__ = {
	.init = &initSRAM_internal,
	.read = &readSRAM_internal,
	.write = &writeSRAM_internal,
	.start = AVR_SRAM_START + HEAP_OFFSET,
	.size = AVR_MEMORY_SRAM / 2 - HEAP_OFFSET
};

void initSRAM_external(void) {
	os_spi_init();
	// Configure CS
	sbi(DDRB, SPI_CS);
	deselect_memory();
	// Set Operation Mode to Byte Operation
	set_operation_mode(0);
}

MemValue readSRAM_external(MemAddr addr) {
	os_enterCriticalSection();
	select_memory();
	os_spi_send(SPI_INS_READ);
	transfer_adress(addr);
	uint8_t result = os_spi_receive();
	deselect_memory();
	os_leaveCriticalSection();
	return result;
}

void writeSRAM_external(MemAddr addr, MemValue value) {
	os_enterCriticalSection();
	select_memory();
	os_spi_send(SPI_INS_WRITE);
	transfer_adress(addr);
	os_spi_send(value);
	deselect_memory();
	os_leaveCriticalSection();
}

MemDriver extSRAM__ = {
	.init = &initSRAM_external,
	.read = &readSRAM_external,
	.write = &writeSRAM_external,
	.start = EXT_SRAM_START,
	.size = EXT_MEMORY_SRAM
};


void initMemoryDevices(void) {
	intSRAM->init();
	extSRAM->init();	
}