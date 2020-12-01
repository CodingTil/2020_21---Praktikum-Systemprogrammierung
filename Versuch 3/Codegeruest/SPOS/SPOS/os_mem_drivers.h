/*
 * os_mem_drivers.h
 *
 * Created: 26.11.2020 14:29:23
 *  Author: User
 */ 


#ifndef OS_MEM_DRIVERS_H_
#define OS_MEM_DRIVERS_H_

#define intSRAM &(intSRAM__)

typedef uint16_t MemAddr;
typedef uint8_t MemValue;

typedef struct{
	//const MemAddr os_mem_drivers_addr;
	//const MemValue os_mem_drivers_value;
	MemAddr start;
	size_t size;	

	typedef void (*init)(void);
	typedef MemValue (*read)(MemAddr);
	typedef void (*write)(MemAddr, MemValue);
	
}MemDriver;

typedef (struct MemDriver) MemDriver;


#endif /* OS_MEM_DRIVERS_H_ */