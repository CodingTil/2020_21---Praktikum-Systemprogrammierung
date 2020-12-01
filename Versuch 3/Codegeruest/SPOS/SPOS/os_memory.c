#include "os_memory.h"
#include "os_scheduler.h"
#include "os_core.h"
#include "os_memory_strategies.h"

void os_free(Heap* heap, MemAddr addr) {
	os_freeOwnerRestricted(heap, addr, os_getCurrentProc());
}

size_t os_getMapSize(Heap const* heap) {
	return heap->map_size;
}

size_t os_getUseSize(Heap const* heap) {
	return heap->use_size;
}

MemAddr os_getMapStart(Heap const* heap) {
	return heap->map_start;
}

MemAddr os_getUseStart(Heap const* heap) {
	return heap->use_start;
}

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr) {
	os_enterCriticalSection();
	MemAddr first = os_getFirstByteOfChunk(heap, addr);
	if(os_getMapEntry(heap, first) == 0) return 0;
	uint16_t size = 1;
	while(os_getMapEntry(heap, addr + size) == 0xF) {
		size++;
	}
	os_leaveCriticalSection();
	return size;
}

AllocStrategy os_getAllocationStrategy(Heap const* heap) {
	return heap->alloc_strategy;
}

void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat) {
	heap->alloc_strategy = allocStrat;
}

void setLowNibble (Heap const *heap, MemAddr addr, MemValue value) {
	heap->driver->write(addr, (value & 0xF) | (heap->driver->read(addr) & 0xF0));
}


void setHighNibble (Heap const *heap, MemAddr addr, MemValue value) {
	heap->driver->write(addr, (value << 4) | (heap->driver->read(addr) & 0xF));
}

MemValue getLowNibble (Heap const *heap, MemAddr addr) {
	return heap->driver->read(addr) & 0xF;
}

MemValue getHighNibble (Heap const *heap, MemAddr addr) {
	return heap->driver->read(addr) >> 4;
}

void setMapEntry (Heap const *heap, MemAddr addr, MemValue value) {
	MemAddr map_addr = (addr - heap->use_start) / 2 + heap->map_start;
	if((addr - heap->use_start) / 2 % 2 == 0) setHighNibble(heap, addr, value);
	setLowNibble(heap, map_addr, value);
}

MemValue os_getMapEntry (Heap const *heap, MemAddr addr) {
	os_enterCriticalSection();
	MemAddr map_addr = (addr - heap->use_start) / 2 + heap->map_start;
	MemValue value;
	if((addr - heap->use_start) / 2 % 2 == 0) value = getHighNibble(heap, map_addr);
	else value = getLowNibble(heap, map_addr);
	os_leaveCriticalSection();
	return value;
}

MemAddr os_getFirstByteOfChunk (Heap const *heap, MemAddr addr) {
	os_enterCriticalSection();
	size_t i = 0;
	while(os_getMapEntry(heap, addr + i) == 0xF) {
		i++;
	}
	os_leaveCriticalSection();
	return addr + i;
}

ProcessID getOwnerOfChunk (Heap const *heap, MemAddr addr) {
	return os_getMapEntry(heap, os_getFirstByteOfChunk(heap, addr));
}

void os_freeOwnerRestricted (Heap *heap, MemAddr addr, ProcessID owner) {
	if(getOwnerOfChunk(heap, addr) != owner) { //Missing: OS
		return;
	}
	os_enterCriticalSection();
	MemAddr first = os_getFirstByteOfChunk(heap, addr);
	do {
		setMapEntry(heap, first, 0);
		heap->driver->write(first, 0);
		first++;
	} while (os_getMapEntry(heap, first) == 0xF);
	os_leaveCriticalSection();
}

void os_freeProcessMemory(Heap *heap, ProcessID pid) {
	// Documentation: Call os_getMapEntry() ?
	os_enterCriticalSection();
	MemAddr addr = os_getUseStart(heap);
	size_t size = os_getUseSize(heap);
	for(size_t i = 0; i < size; i++) {
		os_freeOwnerRestricted(heap, addr + i, pid);
	}
	os_leaveCriticalSection();
}

MemAddr os_malloc(Heap* heap, size_t size) {
	os_enterCriticalSection();
	AllocStrategy allocStrat = os_getAllocationStrategy(heap);
	MemAddr addr;
	switch(allocStrat) {
		case OS_MEM_FIRST: addr = os_Memory_FirstFit(heap, size); break;
		case OS_MEM_NEXT: addr = os_Memory_NextFit(heap, size); break;
		case OS_MEM_BEST: addr = os_Memory_BestFit(heap, size); break;
		case OS_MEM_WORST: addr = os_Memory_WorstFit(heap, size); break;
	}
	if(addr == 0) {
		os_leaveCriticalSection();
		return 0;
	}
	setMapEntry(heap, addr, os_getCurrentProc());
	size_t i = 1;
	while(i < size) {
		setMapEntry(heap, addr + i, 0xF);
	}
	os_leaveCriticalSection();
	return addr;
}