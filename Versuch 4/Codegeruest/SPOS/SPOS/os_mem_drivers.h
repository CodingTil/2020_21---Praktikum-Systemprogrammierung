#ifndef OS_MEM_DRIVERS_H_
#define OS_MEM_DRIVERS_H_

#include <inttypes.h>

#define SPI_CS 4

#define SPI_READ 0x03
#define SPI_WRITE 0x02
#define SPI_WRMR 0x01

typedef uint16_t MemAddr;

typedef uint8_t MemValue;

typedef struct MemDriver {
	MemAddr start;
	uint16_t size;
	void (*init)(void);
	MemValue (*read)(MemAddr addr);
	void (*write)(MemAddr addr, MemValue value);
} MemDriver;

extern MemDriver intSRAM__;
#define intSRAM (&intSRAM__)

extern MemDriver extSRAM__;
#define extSRAM (&extSRAM__)

void initMemoryDevices(void);

#endif /* OS_MEM_DRIVERS_H_ */