#include "Allocator.h"
#include <assert.h>
#include <new>
#include <iostream>
#include <chrono> // Добавлена библиотека chrono для измерения времени

// @see https://github.com/endurodave/Allocator

// On VisualStudio, to disable the debug heap for faster performance when using
// the debugger use this option:
// Debugging > Environment _NO_DEBUG_HEAP=1

class MyClass 
{
	DECLARE_ALLOCATOR
	// remaining class definition
};
IMPLEMENT_ALLOCATOR(MyClass, 0, 0)

// Heap blocks mode unlimited with 100 byte blocks
Allocator allocatorHeapBlocks(100);

// Heap pool mode with 20, 100 byte blocks
Allocator allocatorHeapPool(100, 20);

// Static pool mode with 20, 100 byte blocks
char staticMemoryPool[100 * 20];
Allocator allocatorStaticPool(100, 20, staticMemoryPool);

// Static pool mode with 20 MyClass sized blocks using template
AllocatorPool<MyClass, 20> allocatorStaticPool2;

// Benchmark allocators
static const int MAX_BLOCKS = 10000;
static const int MAX_BLOCK_SIZE = 4096;
void* memoryPtrs[MAX_BLOCKS];
void* memoryPtrs2[MAX_BLOCKS];
AllocatorPool<char[MAX_BLOCK_SIZE], MAX_BLOCKS*2> allocatorStaticPoolBenchmark;
Allocator allocatorHeapBlocksBenchmark(MAX_BLOCK_SIZE);

static void out_of_memory()
{
	// new-handler function called by Allocator when pool is out of memory
	assert(0);
}

typedef void* (*AllocFunc)(int size);
typedef void (*DeallocFunc)(void* ptr);
void Benchmark(const char* name, AllocFunc allocFunc, DeallocFunc deallocFunc);
void* AllocHeap(int size);
void DeallocHeap(void* ptr);
void* AllocStaticPool(int size);
void DeallocStaticPool(void* ptr);
void* AllocHeapBlocks(int size);
void DeallocHeapBlocks(void* ptr);

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(void)
{
	std::set_new_handler(out_of_memory);

	// Allocate MyClass using fixed block allocator
	MyClass* myClass = new MyClass();
	delete myClass;

	// Allocate 100 bytes in fixed block allocator, then deallocate
	void* memory1 = allocatorHeapBlocks.Allocate(100);
	allocatorHeapBlocks.Deallocate(memory1);

	void* memory2 = allocatorHeapBlocks.Allocate(100);
	allocatorHeapBlocks.Deallocate(memory2);

	void* memory3 = allocatorHeapPool.Allocate(100);
	allocatorHeapPool.Deallocate(memory3);

	void* memory4 = allocatorStaticPool.Allocate(100);
	allocatorStaticPool.Deallocate(memory4);

	void* memory5 = allocatorStaticPool2.Allocate(sizeof(MyClass));
	allocatorStaticPool2.Deallocate(memory5);

	Benchmark("Heap (Run 1)", AllocHeap, DeallocHeap);
	Benchmark("Heap (Run 2)", AllocHeap, DeallocHeap);
	Benchmark("Heap (Run 3)", AllocHeap, DeallocHeap);
	Benchmark("Static Pool (Run 1)", AllocStaticPool, DeallocStaticPool);
	Benchmark("Static Pool (Run 2)", AllocStaticPool, DeallocStaticPool);
	Benchmark("Static Pool (Run 3)", AllocStaticPool, DeallocStaticPool);
	Benchmark("Heap Blocks (Run 1)", AllocHeapBlocks, DeallocHeapBlocks);
	Benchmark("Heap Blocks (Run 2)", AllocHeapBlocks, DeallocHeapBlocks);
	Benchmark("Heap Blocks (Run 3)", AllocHeapBlocks, DeallocHeapBlocks);
	return 0;
}

//------------------------------------------------------------------------------
// AllocHeap
//------------------------------------------------------------------------------
void* AllocHeap(int size)
{
	return new CHAR[size];
}

//------------------------------------------------------------------------------
// DeallocHeap
//------------------------------------------------------------------------------
void DeallocHeap(void* ptr)
{
	delete [] ptr;
}

//------------------------------------------------------------------------------
// AllocStaticPool
//------------------------------------------------------------------------------
void* AllocStaticPool(int size)
{
	return allocatorStaticPoolBenchmark.Allocate(size);
}

//------------------------------------------------------------------------------
// DeallocStaticPool
//------------------------------------------------------------------------------
void DeallocStaticPool(void* ptr)
{
	allocatorStaticPoolBenchmark.Deallocate(ptr);
}

//------------------------------------------------------------------------------
// AllocHeapBlocks
//------------------------------------------------------------------------------
void* AllocHeapBlocks(int size)
{
	return allocatorHeapBlocksBenchmark.Allocate(size);
}

//------------------------------------------------------------------------------
// DeallocHeapBlocks
//------------------------------------------------------------------------------
void DeallocHeapBlocks(void* ptr)
{
	allocatorHeapBlocksBenchmark.Deallocate(ptr);
}

//------------------------------------------------------------------------------
// Benchmark
//------------------------------------------------------------------------------
void Benchmark(const char* name, AllocFunc allocFunc, DeallocFunc deallocFunc)
{
    using namespace std::chrono;
    
    auto totalDuration = microseconds(0);
    
    // Allocate MAX_BLOCKS blocks MAX_BLOCK_SIZE / 2 sized blocks
    auto startTime = high_resolution_clock::now();
    for (int i = 0; i < MAX_BLOCKS; i++)
        memoryPtrs[i] = allocFunc(MAX_BLOCK_SIZE / 2);
    auto endTime = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(endTime - startTime);
    std::cout << name << " allocate time: " << duration.count() << std::endl;
    totalDuration += duration;

    // Deallocate MAX_BLOCKS blocks (every other one)
    startTime = high_resolution_clock::now();
    for (int i = 0; i < MAX_BLOCKS; i += 2)
        deallocFunc(memoryPtrs[i]);
    endTime = high_resolution_clock::now();
    duration = duration_cast<microseconds>(endTime - startTime);
    std::cout << name << " deallocate time: " << duration.count() << std::endl;
    totalDuration += duration;

    // Allocate MAX_BLOCKS blocks MAX_BLOCK_SIZE sized blocks
    startTime = high_resolution_clock::now();
    for (int i = 0; i < MAX_BLOCKS; i++)
        memoryPtrs2[i] = allocFunc(MAX_BLOCK_SIZE);
    endTime = high_resolution_clock::now();
    duration = duration_cast<microseconds>(endTime - startTime);
    std::cout << name << " allocate time: " << duration.count() << std::endl;
    totalDuration += duration;

    // Deallocate MAX_BLOCKS blocks (every other one)
    startTime = high_resolution_clock::now();
    for (int i = 1; i < MAX_BLOCKS; i += 2)
        deallocFunc(memoryPtrs[i]);
    endTime = high_resolution_clock::now();
    duration = duration_cast<microseconds>(endTime - startTime);
    std::cout << name << " deallocate time: " << duration.count() << std::endl;
    totalDuration += duration;

    // Deallocate MAX_BLOCKS blocks 
    startTime = high_resolution_clock::now();
    for (int i = MAX_BLOCKS - 1; i >= 0; i--)
        deallocFunc(memoryPtrs2[i]);
    endTime = high_resolution_clock::now();
    duration = duration_cast<microseconds>(endTime - startTime);
    std::cout << name << " deallocate time: " << duration.count() << std::endl;
    totalDuration += duration;

    std::cout << name << " TOTAL TIME: " << totalDuration.count() << std::endl;
}

