#ifndef OS_MEM_DRIVERS_H_
#define OS_MEM_DRIVERS_H_

typedef uint16_t MemAddr;

typedef uint8_t MemValue;

typedef struct MemDriver {
	MemAddr start;
	uint16_t size;
	void (*init)(void);
	MemValue (*read)(MemAddr addr);
	void (*write)(MemAddr addr, MemValue value);
} MemDriver;

#define intSRAM &(intSRAM__);

#endif /* OS_MEM_DRIVERS_H_ */