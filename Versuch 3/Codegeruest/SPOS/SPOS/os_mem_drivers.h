#ifndef OS_MEM_DRIVERS_H_
#define OS_MEM_DRIVERS_H_

#include <stdint.h>
#include "util.h"

typedef uint16_t MemAddr;
typedef uint8_t MemValue;

typedef struct {
	MemAddr start;
	size_t size;
	void (*init)(void);
	MemValue (*read)(MemAddr);
	void (*write)(MemAddr, MemValue);
}MemDriver;

MemDriver intSRAM__;
#define intSRAM (&intSRAM__)

#endif /* OS_MEM_DRIVERS_H_ */