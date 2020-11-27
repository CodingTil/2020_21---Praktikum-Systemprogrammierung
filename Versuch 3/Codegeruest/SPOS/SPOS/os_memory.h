#ifndef OS_MEMORY_H_
#define OS_MEMORY_H_

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"

MemAddr os_malloc(Heap* heap, size_t size);
void os_free(Heap* heap, MemAddr addr);

size_t os_getMapSize(Heap const* heap);
size_t os_getUseSize(Heap const* heap);
MemAddr os_getMapStart(Heap const* heap);
MemAddr os_getUseStart(Heap const* heap);

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr);
AllocStrategy os_getAllocationStrategy(Heap const* heap);
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);

MemValue os_getMapEntry (Heap const *heap, MemAddr addr);

MemAddr os_getFirstByteOfChunk (Heap const *heap, MemAddr addr);
void os_freeOwnerRestricted (Heap *heap, MemAddr addr, ProcessID owner);
void os_freeProcessMemory(Heap *heap, ProcessID pid);

#endif /* OS_MEMORY_H_ */