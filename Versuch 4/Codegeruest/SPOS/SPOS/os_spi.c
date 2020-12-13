#include "os_spi.h"

#include <avr/io.h>

void os_spi_init(void) {
	cbi(SPCR, SPIE);
	sbi(SPCR, SPE);
	sbi(SPCR, MSTR);
	sbi(SPSR, SPI2X);
	cbi(SPCR, SPR0);
	cbi(SPCR, SPR1);
	cbi(SPCR, DORD);
	cbi(SPCR, CPOL);
	cbi(SPCR, CPHA);
}

uint8_t os_spi_send(uint8_t data) {
	os_enterCriticalSection();
	SPDR = data;
	while(!gbi(SPSR, SPIF));
	cbi(SPSR, SPIF) // ???
	uint8_t ret = SPDR;
	os_leaveCriticalSection();
	return ret;
}

uint8_t os_spi_receive() {
	return os_spi_receive(OS_SPI_NULL);
}