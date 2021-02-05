#ifndef OS_MEMORY_H_
#define OS_MEMORY_H_

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_scheduler.h"

MemAddr os_malloc(Heap* heap, size_t size);
void os_free(Heap* heap, MemAddr addr);
MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size);

MemValue os_getMapEntry(Heap const *heap, MemAddr addr);

void os_freeProcessMemory(Heap* heap, ProcessID pid);

size_t os_getMapSize(Heap const* heap);
size_t os_getUseSize(Heap const* heap);
MemAddr os_getMapStart(Heap const* heap);
MemAddr os_getUseStart(Heap const* heap);

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr);

MemAddr os_getFirstByteOfChunk(Heap const *heap, MemAddr addr);

AllocStrategy os_getAllocationStrategy(Heap const* heap);
void os_setAllocationStrategy(Heap* heap, AllocStrategy allocStrat);

MemAddr os_mallocOwner(Heap *heap, size_t size, ProcessID owner);
MemAddr os_sh_malloc(Heap *heap, size_t size);
void os_sh_free(Heap *heap, MemAddr *addr);
MemAddr os_sh_readOpen(Heap const *heap, MemAddr const *ptr);
MemAddr os_sh_writeOpen(Heap const *heap, MemAddr const *ptr);
void os_sh_close(Heap const *heap, MemAddr addr);
void os_sh_write(Heap const *heap, MemAddr const *ptr, uint16_t offset, MemValue const *dataSrc, uint16_t length);
void os_sh_read(Heap const *heap, MemAddr const *ptr, uint16_t offset, MemValue *dataDest, uint16_t length);
void os_sh_close(Heap const* heap, MemAddr addr);

#endif /* OS_MEMORY_H_ */