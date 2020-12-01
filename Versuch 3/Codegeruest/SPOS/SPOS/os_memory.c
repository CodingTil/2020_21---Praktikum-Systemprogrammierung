/*
 * os_memory.c
 *
 * Created: 01.12.2020 17:36:55
 *  Author: User
 */ 
#include "os_memory.h"

MemAddr os_malloc(Heap* heap, uint16_t size) {
	//TODO
}

void os_free(Heap* heap, MemAddr addr) {
	//TODO
}

size_t os_getMapSize(Heap const* heap) { return heap->map_size; }
size_t os_getUseSize(Heap const* heap) { return heap->use_size; }
MemAddr os_getMapStart(Heap const* heap) { return heap->map_start; }
MemAddr os_getUseStart(Heap const* heap) { return heap->use_start; }

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr) {
	
}

AllocStrategy os_getAllocationStrategy(Heap const* heap) { return heap->heap_strategy; }
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat) { heap->heap_strategy = allocStrat; }
