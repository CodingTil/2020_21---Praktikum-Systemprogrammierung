#ifndef OS_MEMORY_STRATEGIES_H_
#define OS_MEMORY_STRATEGIES_H_

MemAddr os_Memory_FirstFit(Heap *heap, size_t size);
MemAddr os_Memory_NextFit(Heap *heap, size_t size);
MemAddr os_Memory_BestFit(Heap *heap, size_t size);
MemAddr os_Memory_WorstFit(Heap *heap, size_t size);


#endif /* OS_MEMORY_STRATEGIES_H_ */