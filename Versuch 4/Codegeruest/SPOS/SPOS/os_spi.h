#ifndef OS_SPI_H_
#define OS_SPI_H_

#include "util.h"

#define SPI_SRAM_START 0
#define SPI_SRAM_SIZE 65535

#define OS_SPI_NULL 0xFF

void os_spi_init(void);
uint8_t os_spi_send(uint8_t data);
uint8_t os_spi_receive();

#endif /* OS_SPI_H_ */