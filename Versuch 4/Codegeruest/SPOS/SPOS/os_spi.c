#include "os_spi.h"

#define LSB 0 // for MSB set 0
#define POL 0 // idle Low, active High
#define PHA 0 // Leading Edge

#define DUMMY_BYTE 0xFF

void os_spi_init(void) {
	// Activate SPI-Module
	sbi(SPCR, SPE);
	// Configure as Bus-Master
	sbi(SPCR, MSTR);
	// Set Dataorder
	if(LSB) sbi(SPCR, DORD);
	else	cbi(SPCR, DORD);
	// Set Polarity
	if(POL) sbi(SPCR, CPOL);
	else	sbi(SPCR, CPOL);
	// Set Phase
	if(PHA)	sbi(SPCR, CPHA);
	else	cbi(SPCR, CPHA);
	// Set Clock Frequency to (f_CPU / 2) [f_CPU = 20MHz;  23A1024 Max. Frequency is 20Mhz]
	sbi(SPSR, SPI2X);
	cbi(SPCR, SPR1);
	cbi(SPCR, SPR0);
}

uint8_t os_spi_send(uint8_t data) {
	os_enterCriticalSection();
	SPDR = data;
	uint8_t response;
	while(!gbi(SPSR, SPIF));
	response = SPDR;
	os_leaveCriticalSection();
	return response;
}

uint8_t os_spi_receive() {
	return os_spi_receive(DUMMY_BYTE);
}