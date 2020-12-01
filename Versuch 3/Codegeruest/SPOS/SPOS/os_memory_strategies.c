#include "os_memory_strategies.h"
#include "os_memory.h"

MemAddr os_Memory_FirstFit(Heap *heap, size_t size) {
	MemAddr addr;
	size_t current_size;
	for(addr = heap->use_start; addr < heap->use_size - size; addr++) {
		if(os_getMapEntry(heap, addr) == 0) {
			bool found = true;
			for(current_size = 0; current_size < size && found; current_size++) {
				if(os_getMapEntry(heap, addr + current_size) != 0) {
					found = false;
				}
			}
			if(found) {
				return addr;
			}
			addr += current_size;
		}
	}
	return 0;
}


MemAddr os_Memory_NextFit(Heap *heap, size_t size) {
	static MemAddr last_addr = -1;
	MemAddr addr = -1;
	size_t current_size;
	if(last_addr == -1) {
		last_addr = os_Memory_FirstFit(heap, size);
		return last_addr;
	}
	for(addr = last_addr; addr < heap->use_size - size; addr++) {
		if(os_getMapEntry(heap, addr) == 0) {
			bool found = true;
			for(current_size = 0; current_size < size && found; current_size++) {
				if(os_getMapEntry(heap, addr + current_size) != 0) {
					found = false;
				}
			}
			if(found) {
				last_addr = addr;
				return addr;
			}
			addr += current_size;
		}
	}
	for(addr = heap->use_start; addr < last_addr - size; addr++) {
		if(os_getMapEntry(heap, addr) == 0) {
			bool found = true;
			for(current_size = 0; current_size < size && found; current_size++) {
				if(os_getMapEntry(heap, addr + current_size) != 0) {
					found = false;
				}
			}
			if(found) {
				last_addr = addr;
				return addr;
			}
			addr += current_size;
		}
	}
	return 0;
}

MemAddr os_Memory_BestFit(Heap *heap, size_t size) {
	MemAddr addr;
	size_t current_size;
	MemAddr best_addr;
	size_t best_size = -1;
	for(addr = heap->use_start; addr < heap->use_size - size; addr++) {
		current_size = 0;
		while(os_getMapEntry(heap, addr + current_size) == 0) current_size++;
		if(current_size == size) return addr;
		if(best_size == -1 && current_size > size) {
			best_size = current_size;
			best_addr = addr;
		}else if(current_size < best_size && current_size > size) {
			best_size = current_size;
			best_addr = addr;
		}
		addr += current_size;
	}
	if(best_size == -1) return 0;
	return best_addr;
}

MemAddr os_Memory_WorstFit(Heap *heap, size_t size) {
	MemAddr addr;
	size_t current_size;
	MemAddr worst_addr;
	size_t worst_size = -1;
	for(addr = heap->use_start; addr < heap->use_size - size; addr++) {
		current_size = 0;
		while(os_getMapEntry(heap, addr + current_size) == 0) current_size++;
		if(current_size >= heap->use_size / 2) return addr;
		if(worst_size == -1 && current_size > size) {
			worst_size = current_size;
			worst_addr = addr;
		}else if(current_size > worst_size && current_size > size) {
			worst_size = current_size;
			worst_addr = addr;
		}
		addr += current_size;
	}
	if(worst_size == -1) return 0;
	return worst_addr;
}
