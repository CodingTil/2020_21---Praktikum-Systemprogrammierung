/*
 * os_memory.h
 *
 * Created: 01.12.2020 13:44:06
 *  Author: User
 */ 


#ifndef OS_MEMORY_H_
#define OS_MEMORY_H_

//Funktionen
MemAddr os_malloc(Heap* heap, uint16_t size);
void os_free(Heap* heap, MemAddr addr);

size_t os_getMapSize(Heap const* heap);
size_t os_getUseSize(Heap const* heap);
MemAddr os_getMapStart(Heap const* heap);
MemAddr os_getUseStart(Heap const* heap);

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr);

AllocStrategy os_getAllocationStrategy(Heap const* heap);
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);



#endif /* OS_MEMORY_H_ */