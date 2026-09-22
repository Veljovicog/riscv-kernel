#include "../h/console.hpp"
#include "../h/semaphore.hpp"      // ksemaphore + placement new
#include "../h/tcb.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/syscall_c.hpp"      // sem_wait/sem_signal (C API, za TX nit)
#include "../lib/hw.h"

// staticka polja
char KConsole::outBuf[KConsole::BUF];
int  KConsole::outHead = 0;
int  KConsole::outTail = 0;
char KConsole::inBuf[KConsole::BUF];
int  KConsole::inHead = 0;
int  KConsole::inTail = 0;

ksemaphore* KConsole::outItems = nullptr;
ksemaphore* KConsole::outSpace = nullptr;
ksemaphore* KConsole::inItems  = nullptr;

// napravi kernel semafor DIREKTNO (u fazi inicijalizacije, bez ecall-a)
static ksemaphore* makeSem(unsigned init) {
    void* p = MemoryAllocator::mem_alloc(sizeof(ksemaphore));
    if (p == nullptr) return nullptr;
    return new (p) ksemaphore(init);
}

void KConsole::init() {
    outHead = outTail = 0;
    inHead  = inTail  = 0;

    outItems = makeSem(0);          // na pocetku nema sta da se salje
    outSpace = makeSem(BUF);        // ceo izlazni bafer je slobodan
    inItems  = makeSem(0);          // nema pristiglih znakova

    // Samo RX prekid (bit 0). NE ukljucujemo TX prekid (bit 1) -- inace bi
    // prazan THR neprekidno okidao prekid ("TX storm"). TX resavamo prozivanjem.
    // IER (Interrupt Enable Register) je na adresi baze UART-a + 1.
    *(volatile char*) (CONSOLE_TX_DATA + 1) = 0x01;

    // interna kernel nit koja prazni izlazni bafer
    void* stack = MemoryAllocator::mem_alloc(DEFAULT_STACK_SIZE);
    TCB::createThread(txThreadBody, nullptr,
                      (char*) stack + DEFAULT_STACK_SIZE, /*kernelMode=*/true);
}

// ---- IZLAZ ----

// putc syscall: stavi znak u izlazni bafer i probudi TX nit.
// Radi se sa SIE=0 (unutar syscalla), pa je pristup outSpace/outItems bezbedan.
void KConsole::doPutc(char c) {
    outSpace->wait();                       // blokiraj ako je bafer pun
    outBuf[outTail] = c;
    outTail = (outTail + 1) % BUF;
    outItems->signal();                     // ima novih znakova za slanje
}

// Telo interne TX niti. Radi kao NORMALNA nit (SIE=1), pa semafore ne sme da
// zove direktno (to bi diralo raspoređivač sa upaljenim prekidima) -- zato
// koristi C API (sem_wait/sem_signal preko ecall-a, koji radi sa SIE=0).
// Bafer indekse (outHead) sme da dira: outHead pise ISKLJUCIVO ova nit,
// outTail pise iskljucivo putc -> nema trke.
void KConsole::txThreadBody(void*) {
    while (true) {
        sem_wait((sem_t) outItems);         // cekaj da ima znaka (bezbedno blokira)

        char c = outBuf[outHead];
        outHead = (outHead + 1) % BUF;

        // sacekaj da UART bude spreman za slanje (THRE), pa posalji
        while ((*(volatile char*) CONSOLE_STATUS & CONSOLE_TX_STATUS_BIT) == 0);
        *(volatile char*) CONSOLE_TX_DATA = c;

        sem_signal((sem_t) outSpace);       // oslobodi jedno mesto u baferu
    }
}

bool KConsole::outEmpty() {
    return outHead == outTail;
}

// ---- ULAZ ----

// getc syscall: uzmi jedan znak iz ulaznog bafera; blokiraj ako ga nema.
char KConsole::doGetc() {
    inItems->wait();                        // cekaj dok prekid ne ubaci znak
    char c = inBuf[inHead];
    inHead = (inHead + 1) % BUF;
    return c;
}

// Prekidna rutina za konzolu (IRQ 10). Radi sa SIE=0. Pokupi SVE pristigle
// znakove iz UART-a, ubaci ih u ulazni bafer i probudi getc koji ceka.
void KConsole::handleRx() {
    while ((*(volatile char*) CONSOLE_STATUS & CONSOLE_RX_STATUS_BIT) != 0) {
        char c = *(volatile char*) CONSOLE_RX_DATA;

        int nextTail = (inTail + 1) % BUF;
        if (nextTail != inHead) {           // ima mesta u ulaznom baferu
            inBuf[inTail] = c;
            inTail = nextTail;
            inItems->signal();              // probudi eventualnog cekaoca u getc
        }
        // ako je ulazni bafer pun -> znak se odbacuje
    }
}
