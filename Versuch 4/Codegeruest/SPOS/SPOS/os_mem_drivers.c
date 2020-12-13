#include "os_mem_drivers.h"
#include "defines.h"
#include "os_spi.h"

// How to make this functions private?

void initSRAM_internal(void) {
	//nothing to do here since the SRAM does not need to be initialized
}

MemValue readSRAM_internal(MemAddr addr) {
	// CHECK IF PERMISSIONS???
	return *((MemValue*) addr);
}

void writeSRAM_internal(MemAddr addr, MemValue value) {
	// CHECK IF PERMISSIONS???
	*((MemValue*) addr) = value;
}

MemDriver intSRAM__ = {
	.init = &initSRAM_internal,
	.read = &readSRAM_internal,
	.write = &writeSRAM_internal,
	.start = AVR_SRAM_START + HEAP_OFFSET,
	.size = AVR_MEMORY_SRAM / 2 - HEAP_OFFSET
};

void select_memory() {
	PORTB = 0;
	sbi(PORTB, SPI_CS);
}

void deselect_memory() {
	PORTB = 0;
}

void set_operation_mode(uint8_t mode) {
	select_memory();
	os_spi_send(SPI_WRMR);
	os_spi_send(mode<<6);
	deselect_memory();
}

void transfer_address(MemAddr addr) {
	os_spi_send(0);
	os_spi_send((uint8_t) (addr >> 8));
	os_spi_send((uint8_t) (addr & 0x0F));
}

uint8_t apply_endian(uint8_t value) {
	if (gbi(SPCR, DORD)) { //swap if not byte set?
		value = (value << 7) | ((value & 0x02) << 6) | ((value & 0x04) << 5) | ((value & 0x08) << 4)
				| ((value & 0x10) >> 4) | ((value & 0x20) >> 5) | ((value & 0x40) >> 6) | (value >> 7);
	}
	return value;
}

void initSRAM_external(void) {
	os_spi_init();
	deselect_memory();
	set_operation_mode(0);
}

MemValue readSRAM_external(MemAddr addr) {
	// CHECK IF PERMISSIONS???
	select_memory();
	os_spi_send(SPI_READ);
	transfer_address(addr);
	uint8_t ret = os_spi_receive();
	deselect_memory();
	ret = apply_endian(ret);
	return ret;
}

void writeSRAM_external(MemAddr addr, MemValue value) {
	// CHECK IF PERMISSIONS???
	select_memory();
	os_spi_send(SPI_WRITE);
	transfer_address(addr);
	os_spi_send(apply_endian(value));
	deselect_memory();
}

MemDriver extSRAM__ = {
	.init = &initSRAM_external,
	.read = &readSRAM_external,
	.write = &writeSRAM_external,
	.start = SPI_SRAM_START,
	.size = SPI_SRAM_SIZE
};

void initMemoryDevices() {
	intSRAM->init();
	extSRAM->init();
}