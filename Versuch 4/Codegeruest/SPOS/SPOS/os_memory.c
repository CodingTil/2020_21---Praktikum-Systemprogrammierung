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
		if (heap->last_used[cp] == 0 || address > heap->last_used[cp]) {
			heap->last_used[cp] = address;
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

void mem_copy(Heap* heap, MemAddr old_addr, MemAddr new_addr, uint16_t size_old, uint16_t size_new) {
	if (size_new < size_old) {
		os_error("Tried to mem_copy to a smaller size.");
	}
	if (old_addr > new_addr) {
		MemAddr start = new_addr;
		
		setMapEntry(heap, new_addr, os_getCurrentProc());
		heap->driver->write(new_addr, heap->driver->read(old_addr));
		setMapEntry(heap, old_addr, 0);
		old_addr++;
		new_addr++;
		
		while (old_addr < heap->use_start + heap->use_size && os_getMapEntry(heap, old_addr) == 0) {
			setMapEntry(heap, new_addr, 0xFF);
			heap->driver->write(new_addr, heap->driver->read(old_addr));
			setMapEntry(heap, old_addr, 0);
			old_addr++;
			new_addr++;
		}
		
		for (; new_addr < start + size_new; new_addr++) {
			// if we do not init values with 0 can't a process get all the heap and read it out?
			setMapEntry(heap, new_addr, 0xFF);
		}
	} else {
		MemAddr end_old = old_addr + size_old - 1;
		MemAddr end_new = new_addr + size_new;
		for (uint16_t i = 0; i < new_addr - old_addr; i++) {
			setMapEntry(heap, end_new, 0xFF);
			end_new--;
		}
		
		while (os_getMapEntry(heap, end_old) == 0) {
			setMapEntry(heap, end_new, 0xFF);
			heap->driver->write(end_new, heap->driver->read(end_old));
			setMapEntry(heap, end_old, 0);
			end_old++;
			end_new++;
		}
		
		setMapEntry(heap, end_new, os_getCurrentProc());
		heap->driver->write(end_new, heap->driver->read(end_old));
		setMapEntry(heap, end_old, 0);
	}
}

MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size) {
	os_enterCriticalSection();
	if ((ProcessID) os_getMapEntry(heap, addr) != os_getCurrentProc()) {
		os_leaveCriticalSection();
		return 0;
	}
	
	uint16_t current_size = os_getChunkSize(heap, addr);

	// First try to append the space:
	MemAddr after = addr + current_size;
	for (; after < heap->use_start + heap->use_size; after++) {
		if (os_getMapEntry(heap, after) != 0) {
			break;
		} else {
			if (after >= addr + size) {
				for (MemAddr i = addr + current_size; i < after; i++) {
					setMapEntry(heap, addr, os_getCurrentProc());
				}
				os_leaveCriticalSection();
				return addr;
			}
		}
	}
	
	// Then try to use space before and after.
	MemAddr before = addr;
	while (before > heap->use_start && os_getMapEntry(heap, before) == 0) {
		before--;
	}
	if (after - before > size) {
		mem_copy(heap, addr, before, current_size, size);
		os_leaveCriticalSection();
		return before;
	}
	
	// Search everywhere
	MemAddr new_addr = os_malloc(heap, size);
	if (new_addr != 0) {
		mem_copy(heap, addr, new_addr, current_size, size);
	}
	os_leaveCriticalSection();
	return new_addr;
}