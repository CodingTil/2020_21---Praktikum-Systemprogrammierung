#ifndef OS_MEM_DRIVERS_H_
#define OS_MEM_DRIVERS_H_

#define intSRAM &(intSRAM__);

typedef uint16_t MemAddr;
typedef uint8_t MemValue;

typedef struct {
	MemAddr start;
	size_t size;
	void (*init)(void);
	MemValue (*read)(MemAddr);
	void (*write)(MemAddr, MemValue);
} MemDriver;

#endif /* OS_MEM_DRIVERS_H_ */