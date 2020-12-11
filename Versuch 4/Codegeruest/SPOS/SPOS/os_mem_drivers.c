#include "os_mem_drivers.h"
#include "defines.h"
#include "os_spi.h"
#include "util.h"

void select_memory() {
	PORTB |= (1 << SPI_CS);
}

void deselect_memory() {
	PORTB &= ~(1 << SPI_CS);
}

void set_operation_mode(uint8_t mode) {
	select_memory();
	os_spi_send(SPI_INS_WRMR);
	os_spi_send(mode);
	deselect_memory();
}

void transfer_adress(MemAddr addr) { // 16-bit in 2 bytes
	os_spi_send((uint8_t) (addr >> 8));
	os_spi_send((uint8_t) (addr & 0x0F));
}

/*
uint8_t applyDataOrder(uint8_t data) {
	if(SPI_DATORD) { // Swap on LSB
		data = (data & 0xF0) >> 4 | (data & 0x0F) << 4;
		data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
		data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
	}
	return data;
}
*/

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
	// Set Dataorder
	cbi(SPCR, DORD);
	// Set Polarity
	sbi(SPCR, CPOL);
	// Set Phase
	cbi(SPCR, CPHA);
	// Configure CS
	DDRB |= (1 << SPI_CS);
	deselect_memory();
	// Set Operation Mode to Byte Operation
	set_operation_mode(0);
}

MemValue readSRAM_external(MemAddr addr) {
	select_memory();
	os_spi_send(SPI_INS_WRITE);
	transfer_adress(addr);
	uint8_t result = os_spi_receive();
	deselect_memory();
	return result;
}

void writeSRAM_external(MemAddr addr, MemValue value) {
	select_memory();
	os_spi_send(SPI_INS_WRITE);
	transfer_adress(addr);
	os_spi_send(value);
	deselect_memory();
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