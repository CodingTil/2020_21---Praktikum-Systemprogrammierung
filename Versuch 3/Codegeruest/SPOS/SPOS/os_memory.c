#include "os_memory.h"
#include "os_memory_strategies.h"

void setLowNibble(Heap const *heap, MemAddr addr, MemValue value) {
	heap->driver->write(addr, (value & 0xF) | (heap->driver->read(addr) & 0xF0));
}

void setHighNibble(Heap const *heap, MemAddr addr, MemValue value) {
	heap->driver->write(addr, (value << 4) | (heap->driver->read(addr) & 0xF));
}

MemValue getLowNibble(Heap const *heap, MemAddr addr) {
	return heap->driver->read(addr) & 0xF;
}

MemValue getHighNibble(Heap const *heap, MemAddr addr) {
	return heap->driver->read(addr) >> 4;
}

MemAddr getMapAddress(Heap const *heap, MemAddr addr) {
	return (addr - heap->use_start) / 2 + heap->map_start;
}

void setMapEntry(Heap* heap, MemAddr addr, MemValue value) {
	MemAddr map_addr = getMapAddress(heap, addr);
	if ((addr - heap->use_start) % 2 == 0) {
		setHighNibble(heap, map_addr, value);
	} else {
		setLowNibble(heap, map_addr, value);
	}
}

MemValue os_getMapEntry(Heap const* heap, MemAddr addr) {
	MemAddr map_addr = getMapAddress(heap, addr);
	if ((addr - heap->use_start) % 2 == 0) {
		return getHighNibble(heap, map_addr);
	} else {
		return getLowNibble(heap, map_addr);
	}
}

MemAddr os_getFirstByteOfChunk(Heap const* heap, MemAddr addr) {
	while (os_getMapEntry(heap, addr) == 0xF) addr--;
	return addr;
}

ProcessID getOwnerOfChunk(Heap const* heap, MemAddr addr) {
	return (ProcessID) os_getMapEntry(heap, os_getFirstByteOfChunk(heap, addr));
}

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr) {
	MemAddr start = os_getFirstByteOfChunk(heap, addr);
	if (os_getMapEntry(heap, start) == 0) return 0;
	uint16_t l = 1;
	while (os_getMapEntry(heap, start + l) == 0xF) {
		l++;
	}
	return l;
}

void os_freeOwnerRestricted(Heap* heap, MemAddr addr, ProcessID owner) {
	os_enterCriticalSection();
	MemAddr start = os_getFirstByteOfChunk(heap, addr);
	if ((ProcessID) os_getMapEntry(heap, start) == owner) {
		do {
			setMapEntry(heap, start, 0);
			start++;
		} while (os_getMapEntry(heap, start) == 0xF);
	}
	os_leaveCriticalSection();
}

MemAddr os_malloc(Heap* heap, uint16_t size) {
	os_enterCriticalSection();
	MemAddr address = 0;
	switch(os_getAllocationStrategy(heap)) {
		case OS_MEM_BEST: address = os_Memory_BestFit(heap, size); break;
		case OS_MEM_FIRST: address = os_Memory_FirstFit(heap, size); break;
		case OS_MEM_NEXT: address = os_Memory_NextFit(heap, size); break;
		case OS_MEM_WORST: address = os_Memory_WorstFit(heap, size); break;
	}
	
	if (address != 0) {
		setMapEntry(heap, address, os_getCurrentProc());
		for (uint16_t i = 1; i < size; i++) {
			setMapEntry(heap, address + i, 0xF);
		}
	}
	os_leaveCriticalSection();
	return address;
}

void os_free(Heap* heap, MemAddr address) {
	os_freeOwnerRestricted(heap, address, os_getCurrentProc());
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

AllocStrategy os_getAllocationStrategy(Heap const* heap) {
	return heap->alloc_strategy;
}

void os_setAllocationStrategy(Heap* heap, AllocStrategy allocStrat) {
	heap->alloc_strategy = allocStrat;
}

void os_freeProcessMemory(Heap* heap, ProcessID pid) {
	os_enterCriticalSection();
	for (size_t i = heap->use_start; i < heap->use_start + heap->use_size; i++) {
		os_freeOwnerRestricted(heap, i, pid);
	}
	os_leaveCriticalSection();
}