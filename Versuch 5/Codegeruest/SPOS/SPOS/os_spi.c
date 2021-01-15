#include "os_spi.h"
#include "os_scheduler.h"
#include "util.h"

#define DUMMY_BYTE 0xFF

void os_spi_init(void) {
	sbi(DDRB, 5);
	cbi(DDRB, 6);
	sbi(DDRB, 7);
	SPSR = (1<<SPI2X);
	SPCR = (0<<SPIE) | (1<<MSTR) | (1<<SPE) | (0<<SPR1) | (0<<SPR0) | (0<<DORD) | (0<<CPOL) | (0<<CPHA);
	/*
	// Configure as Bus-Master
	sbi(SPCR, MSTR);
	// Activate SPI-Module
	sbi(SPCR, SPE);
	// Set Clock Frequency to (f_CPU / 2) [f_CPU = 20MHz;  23A1024 Max. Frequency is 20Mhz]
	sbi(SPSR, SPI2X);
	cbi(SPCR, SPR1);
	cbi(SPCR, SPR0);
	// Set Dataorder
	cbi(SPCR, DORD);
	// Set Polarity
	cbi(SPCR, CPOL);
	// Set Phase
	cbi(SPCR, CPHA);
	*/
}

uint8_t os_spi_send(uint8_t data) {
	// get global interrupt
	uint8_t interrupts = gbi(SREG, 7);
	// disable global interrupt
	cbi(SREG, 7);
	
	SPDR = data;
	uint8_t response;
	while(!gbi(SPSR, SPIF));
	response = SPDR;
	
	// reset global interrupt
	if (interrupts) {
		sbi(SREG, 7);
	}
	
	return response;
}

uint8_t os_spi_receive() {
	return os_spi_send(DUMMY_BYTE);
}