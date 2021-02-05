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

void setMapEntry(Heap *heap, MemAddr addr, MemValue value) {
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
			heap->last_used[cp] = address ;
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
	if(oldChunk > newChunk) { // Move backwards
		MemAddr start = newChunk;
		
		setMapEntry(heap, newChunk, os_getCurrentProc());
		heap->driver->write(newChunk, heap->driver->read(oldChunk));
		setMapEntry(heap, oldChunk, 0);
		oldChunk++;
		newChunk++;
		
		while(oldChunk < heap->use_start + heap->use_size && os_getMapEntry(heap, oldChunk) == 0xF) {
			setMapEntry(heap, newChunk, 0xF);
			heap->driver->write(newChunk, heap->driver->read(oldChunk));
			setMapEntry(heap, oldChunk, 0);
			oldChunk++;
			newChunk++;
		}
	
		for (; newChunk < start + newSize; newChunk++) {
			// if we do not init values with 0 can't a process get all the heap and read it out?
			setMapEntry(heap, newChunk, 0xF);
		}
		
	}else if(oldChunk < newChunk) { // Move forwards
		MemAddr end_old = oldChunk + oldSize - 1;
		MemAddr end_new = newChunk + newSize;
		for (uint16_t i = 0; i < newChunk - oldChunk; i++) {
			setMapEntry(heap, end_new, 0xF);
			end_new--;
		}
		
		while (os_getMapEntry(heap, end_old) == 0) {
			setMapEntry(heap, end_new, 0xF);
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
	
	size_t oldSize = os_getChunkSize(heap, addr);
	
	if(size <= oldSize) {
		for(MemAddr i = addr + size; i < addr + oldSize; i++) {
			setMapEntry(heap, i, 0);
		}
		return addr;
	}
	
	// Find after
	MemAddr after = addr + oldSize;
	for (; after < heap->use_start + heap->use_size; after++) {
		if (os_getMapEntry(heap, after) != 0) {
			break;
		} else {
			if (after >= addr + size - 1) {
				for (MemAddr i = addr + oldSize; i <= after; i++) {
					setMapEntry(heap, i, 0xF);
				}
				os_leaveCriticalSection();
				return addr;
			}
		}
	}
	
	// Find before
	MemAddr before = addr - 1;
	while (before >= heap->use_start && os_getMapEntry(heap, before) == 0) {
		before--;
	}
	before++;
	if (after - before >= size) {
		moveChunk(heap, addr, oldSize, before, size);
		os_leaveCriticalSection();
		return before;
	}

	
	// Finding any free chunk
	MemAddr newChunk = os_malloc(heap, size);
	if(newChunk != 0) {
		moveChunk(heap, addr, oldSize, newChunk, size);
		os_leaveCriticalSection();
		return newChunk;
	}
	
	os_leaveCriticalSection();
	return 0;
}

/*
due to the size of a nibble there are 16 possible states in the map
first eight states are covered by Process ID's from 0 to 7
since we need two different states for an open shared memory and a writing state, this implementation provides states for five different reading processes
open shared memory: 0b1000
writing mode:		0b1001
reading:			0b1010
reading 2:			0b1011
reading3:			0b1100
reading4:			0b1101
reading5:			0b1110

0xF is already used for showing connected memory chunks
*/



void os_sh_free(Heap *heap, MemAddr *addr) {
	MemAddr ad = os_getFirstByteOfChunk(heap, (MemAddr) &addr);
	if (os_getMapEntry(heap, ad) == 0b1000) {
		os_free(heap, (MemAddr) &addr);
		} else {
		return;
	}
}



MemAddr os_sh_malloc(Heap *heap, size_t size) {
	os_enterCriticalSection();
	MemAddr mal = os_malloc(heap, size);
	setMapEntry(heap, mal, 0b1000);
	os_leaveCriticalSection();
	return mal;
}

bool reading_possible(Heap const *heap, MemAddr addr) {
	ProcessID id = getOwnerOfChunk(heap,addr);
	if ((id & 0b1001) == 0b1001) {
		//writing
		return false;
	}
	if ((id | 0b0111) == 0b0111) {
		//id is between 0 and 7
		return false;
	}
	if ((id & 0b1110) == 0b1110) {
		//shared memory is already read by 5 processes
		return false;
	}
	if (id == 0xF) {
		return false;
	}
	return true;
}

MemAddr os_sh_readOpen(Heap const *heap, MemAddr const *ptr) {
	while (reading_possible(heap,(MemAddr) &ptr) == false) {
		os_yield();
	}
	os_enterCriticalSection();
	MemAddr addr = os_getFirstByteOfChunk(heap, (MemAddr) &(ptr));
	setMapEntry(heap, addr, (os_getMapEntry(heap,addr) +1));
	os_leaveCriticalSection();
	MemAddr forreturn = (MemAddr) &ptr;
	return forreturn;
}


bool writing_possible(Heap const *heap, MemAddr addr) {
	ProcessID id = getOwnerOfChunk(heap, addr);
	if ((id & 0b1000) == 0b1000) {
		return true;
	}
	return false;
}

MemAddr os_sh_writeOpen(Heap const *heap, MemAddr const *ptr) {
	while (writing_possible(heap,(MemAddr) &ptr) == false) {
		os_yield();
	}
	os_enterCriticalSection();
	MemAddr addr = os_getFirstByteOfChunk(heap, (MemAddr) &ptr);
	setMapEntry(heap, addr, 0b1001);
	os_leaveCriticalSection();
	MemAddr forreturn = (MemAddr) &addr;
	return forreturn;
}

void os_sh_write(Heap const *heap, MemAddr const *ptr, uint16_t offset, MemValue const *dataSrc, uint16_t length) {
	ProcessID id = getOwnerOfChunk(heap, (MemAddr) &ptr);
	MemAddr check = os_sh_writeOpen(heap, ptr);
	if ((id == 0b1001)) {
		for (uint16_t i = (0 + offset); i < offset + length; i++) {
			MemValue wert = heap->driver->read(dataSrc + i);
			heap->driver->write(check + i,wert);
		}
		os_sh_close(heap, (MemAddr) &ptr);
	}
}

void os_sh_read(Heap const *heap, MemAddr const *ptr, uint16_t offset, MemValue *dataDest, uint16_t length) {
	ProcessID id = getOwnerOfChunk(heap,(MemAddr) &ptr);
	MemAddr check = os_sh_readOpen(heap, ptr);
	if ((id | 0b0111) == 0b1111 && id != 0xF && id != 0b1001) {
		for (uint16_t i = (0 + offset); i < offset + length; i++) {
			MemValue wert = heap->driver->read(check + i);
			heap->driver->write(dataDest + i, wert);
		}
		os_enterCriticalSection();
		MemAddr end = os_getFirstByteOfChunk(heap, (MemAddr) &ptr);
		setMapEntry(heap, end, os_getMapEntry(heap, end) - 1);
		os_leaveCriticalSection();
	}
}


void os_sh_close(Heap const* heap, MemAddr addr) {
	os_enterCriticalSection();
	MemAddr chunk = os_getFirstByteOfChunk(heap,addr);
	setMapEntry(heap, chunk, 0b1000);
	os_leaveCriticalSection();
}