#include "../h/MemoryAllocator.hpp"
#include "../h/syscall_c.hpp"
#include "../h/riscv.hpp"
#include "../h/tcb.hpp"
#include "../h/console.hpp"
#include "../lib/hw.h"

extern "C" void interruptHandler();
void userMain();

// userMain se izvrsava kao KORISNICKA nit (test 7 proverava korisnicki rezim)
static void userMainWrapper(void*) { userMain(); }

int main() {
    MemoryAllocator::init();
    Riscv::w_stvec((uint64) &interruptHandler);      // prekidna rutina

    // 'main' postaje sistemska (idle) nit -- ona koja se izvrsava upravo sada
    TCB::running = TCB::createThread(nullptr, nullptr, nullptr, /*kernelMode=*/true);

    KConsole::init();                                // sopstvena konzola: baferi, semafori, TX nit

    // userMain kao zasebna KORISNICKA nit sa sopstvenim stekom
    void* umStack = MemoryAllocator::mem_alloc(DEFAULT_STACK_SIZE);
    TCB* um = TCB::createThread(userMainWrapper, nullptr,
                                (char*) umStack + DEFAULT_STACK_SIZE, /*kernelMode=*/false);

    Riscv::ms_sstatus(Riscv::SSTATUS_SIE);           // od sad su prekidi (tajmer/konzola) dozvoljeni

    // idle petlja: ustupaj procesor dok userMain nit ne zavrsi
    while (!um->isFinished()) thread_dispatch();

    // isprazni izlazni bafer -- pusti TX nit da posalje sve zaostale znakove
    while (!KConsole::outEmpty()) thread_dispatch();

    Riscv::mc_sstatus(Riscv::SSTATUS_SIE);           // ugasi prekide pre gasenja
    *(volatile uint32*) 0x100000 = 0x5555;           // ugasi QEMU
    return 0;
}
