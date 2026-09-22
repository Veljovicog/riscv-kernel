#include "../h/MemoryAllocator.hpp"
#include "../lib/hw.h"

FreeBlock* MemoryAllocator::freeList = nullptr;

void MemoryAllocator::init() {
    freeList = (FreeBlock*)HEAP_START_ADDR;
    freeList->next = nullptr;
    freeList->size = ((size_t)HEAP_END_ADDR - (size_t)HEAP_START_ADDR) / MEM_BLOCK_SIZE;
}
void* MemoryAllocator::mem_alloc(size_t size) {
    //konvertovanje size (u bajtovima) u blokove, da vidimo koliko nam blokova memorije treba (freeList je izrazena u blokovima)
    size_t neededBlocks = (size + sizeof(size_t)) / MEM_BLOCK_SIZE;
    if((size+sizeof(size_t)) % MEM_BLOCK_SIZE != 0) neededBlocks++; //ako ima ostatak povecaj ga za 1 (zauzima ceo blok za to malo sto je ostalo)

    FreeBlock *current = freeList;
    FreeBlock *best = nullptr;
    FreeBlock *previous = nullptr;
    FreeBlock *bestPrev = nullptr;

    //best-fit trazenje slobodnog prostora

    while(current!=nullptr){

        if(current->size >= neededBlocks){
            if(best == nullptr || current->size < best->size){
                best = current;
                bestPrev=previous;
            }
        }
        previous= current;
        current = current->next;

    }
    //nema zadovoljavajuceg slobodnog prostora
    if(best== nullptr) return nullptr;

    //preulancavanje ako je slobodan prostor bio tacno koliko treba
    if(best->size == neededBlocks){
        if(bestPrev== nullptr)
            freeList = best -> next;
        else
            bestPrev -> next = best -> next;
        size_t* size_header = (size_t*)best; //kada smo dodelili ceo blok, nepotreban je header koji ostaje za best, pa u njega mozemo upisati broj blokova koji ce trebati free_mem
        *size_header = neededBlocks;
        return (void*)(size_header+1);
    }
    //azuriramo velicinu slobodnog bloka i vracamo pokazivac na zauzet deo memorije korisniku
    else{
        best->size -= neededBlocks;
        void *allocStart = (char*)best + best->size * MEM_BLOCK_SIZE; //kastujemo u char da bismo pomerali bajt po bajt a ne block_size
        size_t* size_header = (size_t*)allocStart; //
        *size_header = neededBlocks;
        void* userSpace = (void*)(size_header+1);
        return userSpace;
    }

}
int MemoryAllocator::mem_free(void *ptr) {
    if (ptr == nullptr) return -1;

    size_t *header = (size_t*)ptr - 1;
    size_t neededBlocks = *header;

    FreeBlock* returned = (FreeBlock*)header;
    FreeBlock *current = freeList;
    FreeBlock *previous = nullptr;

    //trazimo dva slobodna bloka izmedju kojih treba da se nalazi vraceni

    while (current!=nullptr && current<returned) {
        previous = current;
        current = current->next;
    }
    //ako je vraceni spojen uz prethodni slobodan blok
    if (previous != nullptr && (char*)previous + previous->size * MEM_BLOCK_SIZE == (char*)returned) {
        previous->size += neededBlocks;
        //ako je spojen i uz naredni
        if (current!=nullptr && (char*)previous + previous->size * MEM_BLOCK_SIZE == (char*)current) {
            previous->size += current->size;
            previous->next = current->next;
        }

    }

    else {


        returned->size = neededBlocks;
        returned->next = current;
        if (previous == nullptr)
            freeList = returned;
        else
            previous->next = returned;

        //proveri susednost sa sledecim blokom posle vracenog
        if (current!=nullptr && (char*)returned + returned->size * MEM_BLOCK_SIZE == (char*)current) {
            returned->size += current->size;
            returned->next = current->next;
        }

    }
    return 0;
}
