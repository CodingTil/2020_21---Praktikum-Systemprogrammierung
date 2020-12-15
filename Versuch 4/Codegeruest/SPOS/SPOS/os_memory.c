#include "os_memory.h"
#include "os_memory_strategies.h"
#include "os_core.h"

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
	if (addr >= heap->use_start + heap->use_size || addr < heap->use_start) {
		os_error("Heap: Out of bounds!");
	}
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
	if(addr >= heap->use_start + heap->use_size || addr < heap->use_start) return 0;
	while (addr >= heap->use_start && os_getMapEntry(heap, addr) == 0xF) addr--;
	return addr;
}

ProcessID getOwnerOfChunk(Heap const* heap, MemAddr addr) {
	return (ProcessID) os_getMapEntry(heap, os_getFirstByteOfChunk(heap, addr));
}

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr) {
	MemAddr start = os_getFirstByteOfChunk(heap, addr);
	if (os_getMapEntry(heap, start) == 0) return 0;
	uint16_t l = 1;
	while (start + l < heap->use_start + heap->use_size && os_getMapEntry(heap, start + l) == 0xF) {
		l++;
	}
	return l;
}

void os_freeOwnerRestricted(Heap* heap, MemAddr addr, ProcessID owner) {
	os_enterCriticalSection();
	MemAddr start = os_getFirstByteOfChunk(heap, addr);
	if (start >= heap->use_start + heap->use_size || start < heap->use_start) {
		os_leaveCriticalSection();
		return;
	}
	if ((ProcessID) os_getMapEntry(heap, start) == owner) {
		do {
			setMapEntry(heap, start, 0);
			start++;
		} while (start < heap->use_start + heap->use_size && os_getMapEntry(heap, start) == 0xF);
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
		if(address >= heap->use_start + heap->use_size || address + size < heap->use_start) {
			os_error("Alloc Strat wrong");
		}
		ProcessID cp = os_getCurrentProc();
		setMapEntry(heap, address, cp);
		for (uint16_t i = 1; i < size; i++) {
			setMapEntry(heap, address + i, 0xF);
		}
		
		if (heap->first_used[cp] == 0 || address < heap->first_used[cp]) {
			heap->first_used[cp] = address;
		}
		if (heap->last_used[cp] == 0 || address + size > heap->last_used[cp]) {
			heap->last_used[cp] = address + size;
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
	MemAddr start = heap->first_used[pid];
	MemAddr end = heap->last_used[pid];
	if (start == 0) {
		start = heap->use_start;
	}
	if (end == 0) {
		end = heap->use_start + heap->use_size - 1;
	}
	for (size_t i = start; i <= end; i++) {
		os_freeOwnerRestricted(heap, i, pid);
	}
	os_leaveCriticalSection();
}

void moveChunk(Heap* heap, MemAddr oldChunk, size_t oldSize, MemAddr newChunk, size_t newSize) {
	if(newSize > oldSize) {
		for(size_t i = 0; i < newSize; i++) {
			if(os_getMapEntry(heap, oldChunk) == 0xF || os_getMapEntry(heap, oldChunk) == os_getCurrentProc()) {
				setMapEntry(heap, newChunk, os_getMapEntry(heap, oldChunk));
			}else {
				setMapEntry(heap, newChunk, 0xF);
			}
			heap->driver->write(oldChunk, heap->driver->read(newChunk));
			oldChunk++;
			newChunk++;
		}
		if(newChunk < heap->first_used[os_getCurrentProc()]) {
			heap->first_used[os_getCurrentProc()] = newChunk;
		}
		if(newChunk + newSize > heap->last_used[os_getCurrentProc()]) {
			heap->last_used[os_getCurrentProc()] = newChunk + newSize;
		}
	}
}

MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size) {
	os_enterCriticalSection();
	if ((ProcessID) os_getMapEntry(heap, addr) != os_getCurrentProc()) {
		os_leaveCriticalSection();
		return 0;
	}
	
	size_t oldSize = os_getChunkSize(heap, addr);
	
	if(size <= oldSize) {
		for(MemAddr i = addr + size; i < addr + oldSize; i++) {
			setMapEntry(heap, i, 0);
		}
		return addr;
	}
	
	// Find after
	for(size_t i = 0; i < heap->use_start + heap->use_size; i++) {
		if(os_getMapEntry(heap, addr + oldSize + i) != 0) {
			break;
		}else {
			if(oldSize + i >= size) {
				moveChunk(heap, addr, oldSize, addr, size);
				os_leaveCriticalSection();
				return addr;
			}
		}
	}
	
	//Find before
	MemAddr before = addr;
	while (before > heap->use_start && os_getMapEntry(heap, before) == 0) {
		before--;
	}
	for(size_t i = 0; i < heap->use_start + heap->use_size; i++) {
		if(os_getMapEntry(heap, before + oldSize + i) != 0) {
			break;
		}else {
			if(oldSize + i >= size) {
				moveChunk(heap, addr, oldSize, addr, size);
				os_leaveCriticalSection();
				return addr;
			}
		}
	}
	
	// Finding any free chunk
	MemAddr newChunk = os_malloc(heap, size);
	if(newChunk != 0) {
		moveChunk(heap, addr, oldSize, newChunk, size);
	}
	
	os_leaveCriticalSection();
	return 0;
}