#include "os_memory_strategies.h"
#include "os_memory.h"

MemAddr os_Memory_FirstFit(Heap *heap, size_t size) {
	MemAddr addr;
	size_t current_size = 0;
	for (addr = heap->use_start; addr < heap->use_start + heap->use_size; addr++) {
		if (os_getMapEntry(heap, addr) == 0) {
			current_size++;
			if (current_size >= size) {
				return addr - size + 1;
			}
		} else {
			current_size = 0;
		}
	}
	return 0;
}

MemAddr os_Memory_NextFit(Heap *heap, size_t size) {
	static MemAddr last_addr = 0;
	static bool first_time = true;
	if (first_time) {
		first_time = false;
		last_addr = os_Memory_FirstFit(heap, size);
		return last_addr;
	}
	MemAddr address;
	size_t current_size = 0;
	for (address = last_addr; address < heap->use_size; address++) {
		if (os_getMapEntry(heap, address) == 0) {
			current_size ++;
			if (current_size >= size) {
				return address - size + 1;
			}
		} else {
			current_size = 0;
		}
	}
	for (address = heap->use_start; address < last_addr; address++) {
		if (os_getMapEntry(heap, address) == 0) {
			current_size++;
			if (current_size >= size) {
				return address - size + 1;
			}
		} else {
			current_size = 0;
		}
	}
	return 0;
}

MemAddr os_Memory_BestFit(Heap *heap, size_t size) {
	MemAddr address;
	size_t current_size = 0;
	MemAddr best_addr = 0;
	size_t best_size = 0;
	for (address = heap->use_start; address < heap->use_size; address++) {
		if (os_getMapEntry(heap, address) == 0) {
			current_size++;
		} else {
			if (current_size > size && current_size > best_size) {
				best_addr = address - size - 1;
				best_size = current_size;
			}
			current_size = 0;
		}
	}
	if (current_size > best_size) {
		best_addr = address - size - 1;
	}
	return best_addr;
}

MemAddr os_Memory_WorstFit(Heap *heap, size_t size) {
	MemAddr address;
	size_t current_size = 0;
	MemAddr best_addr = 0;
	size_t best_size = 0;
	for (address = heap->use_start; address < heap->use_size; address++) {
		if (os_getMapEntry(heap, address) == 0) {
			current_size ++;
		} else {
			if (current_size > size && current_size < best_size) {
				best_addr = address - size - 1;
				best_size = current_size;
			}
			current_size = 0;
		}
	}
	if (current_size > size && current_size < best_size) {
		best_addr = address - size - 1;
	}
	return best_addr;
}