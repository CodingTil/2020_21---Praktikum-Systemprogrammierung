#ifndef OS_SPI_H_
#define OS_SPI_H_

#include <avr/io.h>
#include <stdint.h>

#define EXT_SRAM_START 0
#define EXT_MEMORY_SRAM 65535

#define SPI_CS 4 // Pin 4 Port B

//[Instruction Set]
#define SPI_INS_READ 0x03
#define SPI_INS_WRITE 0x02
#define SPI_INS_EDIO 0x3B
#define SPI_INS_EQIO 0x38
#define SPI_INS_RSTIO 0xFF
#define SPI_INS_RDMR 0x05
#define SPI_INS_WRMR 0x01

void os_spi_init(void);
uint8_t os_spi_send(uint8_t data);
uint8_t os_spi_receive();

#endif