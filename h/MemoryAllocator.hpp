#pragma once
#include "../lib/hw.h"


struct FreeBlock{
    size_t size;
    FreeBlock *next;
};

class MemoryAllocator{
public:
    static void* mem_alloc(size_t size);
    static int mem_free(void *ptr);
    static void init();
private:
    static FreeBlock *freeList;
};

