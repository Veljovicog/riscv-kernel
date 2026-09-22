#include "../h/syscall_c.hpp"
#include "../lib/hw.h"

// jedina funkcija koja stvarno radi ecall
static uint64 syscall(uint64 code, uint64 a1v = 0, uint64 a2v = 0,
                                   uint64 a3v = 0, uint64 a4v = 0) {
    register uint64 a0 __asm__("a0") = code;
    register uint64 a1 __asm__("a1") = a1v;
    register uint64 a2 __asm__("a2") = a2v;
    register uint64 a3 __asm__("a3") = a3v;
    register uint64 a4 __asm__("a4") = a4v;
    __asm__ volatile("ecall" : "+r"(a0)
                             : "r"(a1), "r"(a2), "r"(a3), "r"(a4)
                             : "memory");
    return a0;
}

void* mem_alloc(size_t size) {
    size_t blocks = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    return (void*) syscall(0x01, blocks);
}

int mem_free(void* ptr) {
    return (int) syscall(0x02, (uint64) ptr);
}

int thread_create(thread_t* handle, void(*start_routine)(void*), void* arg) {
    // C API PRVO alocira stek, pa tek onda pravi nit -- dva ecall-a
    void* stack = mem_alloc(DEFAULT_STACK_SIZE);
    if (stack == nullptr) return -1;

    void* stackSpace = (char*) stack + DEFAULT_STACK_SIZE;   // vrh (stek raste nanize)

    return (int) syscall(0x11, (uint64) handle, (uint64) start_routine,
                               (uint64) arg,    (uint64) stackSpace);
}

int  thread_exit() {
    return (int) syscall(0x12);
}
void thread_dispatch() {
    syscall(0x13);
}
void putc(char chr) {
    syscall(0x42, (uint64) chr);
}
char getc() {
    return (char) syscall(0x41);
}

//semaforske operacije

int sem_open(sem_t* handle, unsigned init) {
    return (int) syscall(0x21, (uint64) handle, (uint64) init);
}
int sem_close(sem_t id) {
    return (int) syscall(0x22, (uint64) id);
}
int sem_wait(sem_t id) {
    return (int) syscall(0x23, (uint64) id);
}
int sem_signal(sem_t id) {
    return (int) syscall(0x24, (uint64) id);
}
int sem_wait_n(sem_t id, unsigned n) {
    return (int) syscall(0x25, (uint64) id, (uint64) n);
}
int sem_signal_n(sem_t id, unsigned n) {
    return (int) syscall(0x26, (uint64) id, (uint64) n);
}
//za zadnji zadatak
int time_sleep(time_t t) { return (int) syscall(0x31, (uint64) t); }
