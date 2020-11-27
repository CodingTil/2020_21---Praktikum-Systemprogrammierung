#include "os_memory_strategies.h"
#include "os_memheap_drivers.h"

MemAddr os_Memory_FirstFit(Heap *heap, size_t size) {
	MemAddr addr;
	for(addr = heap->map_start; addr < heap->use_start; addr++) {
		if(heap->driver->read(addr) == 0) {
			bool found = true;
			size_t i;
			for(i = 0; i < size & found; i++) {
				if(heap->driver->read(addr + i) != 0) {
			}
			if(found) {
				return addr;
			}
			addr += i;
		}
	}
	if(addr >= heap->use_start) return 0;
	return addr;
}
