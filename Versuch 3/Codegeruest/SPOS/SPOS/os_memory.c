#include "os_memory.h"
#include "os_memory_strategies.h"

MemValue os_getMap(Heap const* heap, MemAddr address) {
	MemAddr map_address = (address - heap->use_start) / 2 + heap->map_start;
	// Error when out of bound?
	if ((address - heap->use_start) % 2 == 0) {
		return heap->driver->read(map_address) >> 4;
		} else {
		return heap->driver->read(map_address) & 0xF;
	}
}

void os_setMap(Heap* heap, MemAddr address, MemValue owner) {
	MemAddr map_address = (address - heap->use_start) / 2 + heap->map_start;
	// Error when out of bound?
	if ((address - heap->use_start) % 2 == 0) {
		heap->driver->write(map_address, (heap->driver->read(map_address) & 0xF) | (owner << 4));
		} else {
		heap->driver->write(map_address, (heap->driver->read(map_address) & 0xF0) | (owner & 0xF));
	}
}

MemAddr os_malloc(Heap* heap, uint16_t size) {
	os_enterCriticalSection();
	MemAddr address;
	switch(os_getAllocationStrategy(heap)) {
		case OS_MEM_BEST: address = os_Memory_BestFit(heap, size); break;
		case OS_MEM_FIRST: address = os_Memory_FirstFit(heap, size); break;
		case OS_MEM_NEXT: address = os_Memory_NextFit(heap, size); break;
		case OS_MEM_WORST: address = os_Memory_WorstFit(heap, size); break;
	}
	
	if (address != 0) {
		os_setMap(heap, address, os_getCurrentProc());
		for (uint16_t i = 1; i < size; i++) {
			os_setMap(heap, address + i, 0xF);
		}
	}
	os_leaveCriticalSection();
	return address;
}

MemAddr os_getFirstAddress(Heap const* heap, MemAddr address) {
	while (os_getMap(heap, address) == 0xF) address--;
	return address;
}

ProcessID os_getOwner(Heap const* heap, MemAddr address) {
	return os_getMap(heap, address);
}

void os_free_pid(Heap* heap, MemAddr address, ProcessID pid) {
	MemAddr start = os_getFirstAddress(heap, address);
	if (os_getOwner(heap, start) == pid) {
		do {
			os_setMap(heap, start, 0);
			start++;
		} while (os_getMap(heap, start) == 0xF);
	}
	return;
}

void os_free(Heap* heap, MemAddr address) {
	os_enterCriticalSection();
	os_free_pid(heap, address, os_getCurrentProc());
	os_leaveCriticalSection();
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

uint16_t os_getChunkSize(Heap const* heap, MemAddr address) {
	os_enterCriticalSection();
	MemAddr start = os_getFirstAddress(heap, address);
	if (os_getOwner(heap, start) == 0) return 0;
	uint16_t l = 1;
	while (os_getOwner(heap, start + l) == 0xF) {
		l++;
	}
	os_leaveCriticalSection();
	return l;
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
		os_free_pid(heap, i, pid);
	}
}