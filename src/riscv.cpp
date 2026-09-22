#include "../h/riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/tcb.hpp"
#include "../h/syscall_c.hpp"
#include "../lib/console.h"
#include "../h/semaphore.hpp"
#include "../h/console.hpp"
#include "../lib/hw.h"

// ispisi 64-bitni broj heksadecimalno (nemamo printf)
static void printHex(uint64 v) {
    __putc('0'); __putc('x');
    for (int i = 15; i >= 0; i--) {
        uint64 d = (v >> (i * 4)) & 0xF;
        __putc(d < 10 ? ('0' + d) : ('a' + d - 10));
    }
    __putc('\n');
}

extern "C" void handleSupervisorTrap(uint64* frame)
{
    uint64 scause = Riscv::r_scause();

    if (scause == 8 || scause == 9) {          // ecall
        // sepc se na izlazu vraca SA STEKA (slot 0), ne iz CSR-a.
        // Zato inkrement mora da ide u frame[0], inace bismo se vratili
        // na istu ecall instrukciju -> beskonacna petlja.
        frame[0] += 4;                         // preskoci ecall instrukciju

        uint64 code = frame[10];               // a0

        if (code == 0x01) {                    // mem_alloc(blokovi)
            frame[10] = (uint64) MemoryAllocator::mem_alloc(frame[11] * MEM_BLOCK_SIZE);
        }
        else if (code == 0x02) {               // mem_free(ptr)
            frame[10] = (uint64) MemoryAllocator::mem_free((void*) frame[11]);
        }
        else if (code == 0x11) {               // thread_create
            thread_t* handle = (thread_t*) frame[11];
            TCB::Body body   = (TCB::Body)  frame[12];
            void* arg        = (void*)      frame[13];
            void* stackSpace = (void*)      frame[14];

            TCB* tcb = TCB::createThread(body, arg, stackSpace);
            if (handle != nullptr) *handle = (thread_t) tcb;
            frame[10] = (tcb == nullptr) ? (uint64) -1 : 0;
        }
        else if (code == 0x12) {               // thread_exit
            TCB::exit();
            frame[10] = 0;
        }
        else if (code == 0x13) {
            // thread_dispatch
            TCB::dispatch();
            frame[10] = 0;
        }
        else if (code == 0x42) {                       // putc
            KConsole::doPutc((char) frame[11]);        // u izlazni bafer -> TX nit salje
        }
        else if (code == 0x41) {                        // getc
            frame[10] = (uint64) KConsole::doGetc();   // blokira dok RX prekid ne ubaci znak
        }

        else if (code == 0x21) {               // sem_open
            sem_t* handle = (sem_t*) frame[11];
            unsigned init = (unsigned) frame[12];
            ksemaphore* s = (ksemaphore*) MemoryAllocator::mem_alloc(sizeof(ksemaphore));
            if (s == nullptr) { frame[10] = (uint64) -1; }
            else {
                s = new (s) ksemaphore(init);   // placement new na vec alociranom
                if (handle != nullptr) *handle = (sem_t) s;
                frame[10] = 0;
            }
        }
        else if (code == 0x22) { frame[10] = (uint64) ksemaphore::close((ksemaphore*) frame[11]); }
        else if (code == 0x23) { frame[10] = (uint64) ((ksemaphore*) frame[11])->wait(); }
        else if (code == 0x24) { frame[10] = (uint64) ((ksemaphore*) frame[11])->signal(); }
        else if (code == 0x25) { frame[10] = (uint64) ((ksemaphore*) frame[11])->wait_n((unsigned) frame[12]); }
        else if (code == 0x26) { frame[10] = (uint64) ((ksemaphore*) frame[11])->signal_n((unsigned) frame[12]); }
        else if (code == 0x31) {
            TCB::sleep(frame[11]);
            frame[10] = 0;
        }
        else {
            // nepoznat syscall kod -> vrati gresku umesto da ostavimo a0 netaknut
            frame[10] = (uint64) -1;
        }

    }
    else if (scause == 0x8000000000000001UL) {   // tajmer (supervisor software interrupt)
        Riscv::clearTimerFlag();
        TCB::tick();                              // prvo probudi uspavane niti
        if (TCB::running != nullptr) {            // (u idle-spinu je running==nullptr)
            TCB::timeSliceCounter++;
            if (TCB::timeSliceCounter >= DEFAULT_TIME_SLICE) {
                TCB::dispatch();                  // istekao kvantum -> preotmi (dispatch resetuje brojac)
            }
        }
        // NEMA w_sstatus/w_sepc -- interrupts.S cuva/vraca stanje sa steka
    }
    else if (scause == 0x8000000000000009UL) {   // spoljasnji prekid (PLIC)
        int irq = plic_claim();
        if (irq == (int) CONSOLE_IRQ) {
            KConsole::handleRx();                 // pokupi znakove sa tastature
        }
        if (irq != 0) plic_complete(irq);
        // NEMA w_sstatus/w_sepc -- interrupts.S sredjuje (kao i kod tajmera)
    }
    else {
        __putc('!'); __putc('T'); __putc('R'); __putc('A'); __putc('P'); __putc(' ');
        printHex(scause);                    // zasto smo upali
        __putc('p'); __putc('c'); __putc('=');
        printHex(Riscv::r_sepc());           // gde je puklo
        while (true);
    }
}