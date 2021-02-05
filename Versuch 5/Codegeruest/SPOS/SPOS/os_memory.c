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

void setMapEntry(Heap const *heap, MemAddr addr, MemValue value) {
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


#define SH_ALLOC 0b1000
#define SH_WRITING 0b1110
/*
#define SH_READING_1 0b0001
#define SH_READING_2 0b0010
#define SH_READING_3 0b0011
#define SH_READING_4 0b0100
#define SH_READING_5 0b0101
*/
#define SH_MAX_READING 5

bool sh_is_shared_memory(Heap const *heap, MemAddr addr) {
	return ((getOwnerOfChunk(heap, addr) & SH_ALLOC) == SH_ALLOC) && (getOwnerOfChunk(heap, addr) != 0xF);
}

void sh_set_writing(Heap const *heap, MemAddr addr) {
	MemAddr chunk = os_getFirstByteOfChunk(heap, addr);
	setMapEntry(heap, chunk, SH_WRITING);
}

bool sh_is_writing(Heap const *heap, MemAddr addr) {
	return getOwnerOfChunk(heap, addr) == SH_WRITING;
}

bool sh_is_reading(Heap const *heap, MemAddr addr) {
	if(sh_is_writing(heap, addr)) return false;
	if(getOwnerOfChunk(heap, addr) == SH_ALLOC) return false;
	return true;
}

uint8_t sh_get_reading(Heap const *heap, MemAddr addr) {
	if(!sh_is_reading(heap, addr)) return 0;
	return getOwnerOfChunk(heap, addr) & (~SH_ALLOC);
}

void sh_add_reading(Heap const *heap, MemAddr addr) {
	MemAddr chunk = os_getFirstByteOfChunk(heap, addr);
	setMapEntry(heap, chunk, os_getMapEntry(heap, chunk) + 1);
}

void sh_remove_reading(Heap const *heap, MemAddr addr) {
	MemAddr chunk = os_getFirstByteOfChunk(heap, addr);
	setMapEntry(heap, chunk, os_getMapEntry(heap, chunk) - 1);
}

bool sh_is_open(Heap const *heap, MemAddr addr) {
	return sh_is_reading(heap, addr) || sh_is_writing(heap, addr);
}

void os_free(Heap* heap, MemAddr address) {
	if(sh_is_shared_memory(heap, address)) {
		os_error("Memory is shared memory.");
	}else {
		os_freeOwnerRestricted(heap, address, os_getCurrentProc());
	}
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



void os_sh_close (Heap const *heap, MemAddr addr) {
	os_enterCriticalSection();
	MemAddr chunk = os_getFirstByteOfChunk(heap, addr);
	while (sh_is_open(heap, chunk)) {
		os_yield();
	}
	setMapEntry(heap, chunk, SH_ALLOC);
	os_leaveCriticalSection();
}

MemAddr os_sh_malloc(Heap* heap, size_t size) {
	os_enterCriticalSection();
	MemAddr address = os_malloc(heap, size);
	if(address != 0) {
		setMapEntry(heap, address, SH_ALLOC);
	}
	os_leaveCriticalSection();
	return address;
}

void os_sh_free(Heap* heap, MemAddr *addr) {
	os_enterCriticalSection();
	if(sh_is_shared_memory(heap, *addr)) {
		while(sh_is_open(heap, *addr)) {
			os_yield();
		}
		os_freeOwnerRestricted(heap, *addr, SH_ALLOC);
	}else {
		os_error("Memory is not shared memory.");
	}
	os_leaveCriticalSection();
}

void os_sh_write(Heap const *heap, MemAddr const *ptr, uint16_t offset, MemValue const *dataSrc, uint16_t length) {
	//Only works with this first?!?!?!?
	MemAddr sh_addr = os_sh_writeOpen(heap, ptr);
		
	if(sh_addr == 0) return;

	uint16_t sh_chunk_size = os_getChunkSize(heap, sh_addr);
	if(!(sh_chunk_size >= length + offset)) {
		os_error("Chunk sizes too small.");
		return;
	}
	
	int i = 0;
	
	do {
		heap->driver->write(sh_addr + offset + i, intHeap->driver->read((MemAddr) dataSrc + i));
		i++;
	}while(i < length);
	
	os_sh_close(heap, sh_addr);
}

void os_sh_read(Heap const *heap, MemAddr const *ptr, uint16_t offset, MemValue *dataDest, uint16_t length) {
	MemAddr sh_addr = os_sh_readOpen(heap, ptr);
	if(sh_addr == 0) return;
	
	uint16_t sh_chunk_size = os_getChunkSize(heap, sh_addr);
	//uint16_t dest_chunk_size = os_getChunkSize(intHeap, (MemAddr) dataDest);
	
	if(!(sh_chunk_size >= length + offset) /*|| dest_chunk_size < length - 1*/) {
		os_error("Chunk sizes too small.");
		return;
	}
	
	int i = 0;
	
	do {
		intHeap->driver->write((MemAddr) dataDest + i, heap->driver->read(sh_addr + offset + i));
		i++;
	}while(i < length);
		
	sh_remove_reading(heap, sh_addr);
}

MemAddr os_sh_readOpen (Heap const *heap, MemAddr const *ptr) {
	os_enterCriticalSection();
	MemAddr addr = os_getFirstByteOfChunk(heap, *ptr);
	if(!(addr >= heap->use_start && addr < heap->use_start + heap->use_size)) {
		os_error("Out of bounds."); //
		os_leaveCriticalSection();
		return 0;
	}
	if(!sh_is_shared_memory(heap, addr)) {
		os_error("Memory not shared memory.");
		os_leaveCriticalSection();
		return 0;
	}
	
	while (sh_is_writing(heap, addr) || sh_get_reading(heap, addr) >= SH_MAX_READING) {
		os_yield();
		addr = os_getFirstByteOfChunk(heap, *ptr);
	}
	sh_add_reading(heap, addr);
	os_leaveCriticalSection();
	return addr;
}

MemAddr os_sh_writeOpen (Heap const *heap, MemAddr const *ptr) {
	os_enterCriticalSection();
	MemAddr addr = os_getFirstByteOfChunk(heap, *ptr);
	if(!(addr >= heap->use_start && addr < heap->use_start + heap->use_size)) {
		//os_error("Out of bounds.");
		os_leaveCriticalSection();
		return 0;
	}
	if(!sh_is_shared_memory(heap, addr)) {
		os_error("Memory not shared memory.");
		os_leaveCriticalSection();
		return 0;
	}
	
	while(sh_is_writing(heap, addr) || sh_is_reading(heap, addr)) {
		os_yield();
		addr = os_getFirstByteOfChunk(heap, *ptr);
	}
	sh_set_writing(heap, addr);
	os_leaveCriticalSection();
	return addr;
}
