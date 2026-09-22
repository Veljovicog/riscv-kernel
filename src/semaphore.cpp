#include "../h/semaphore.hpp"
#include "../h/tcb.hpp"
#include "../h/scheduler.hpp"

ksemaphore::ksemaphore(unsigned init) {
    val = (int) init;           // pocetni broj slobodnih jedinica
    closed = false;
    blockedHead = nullptr;
    blockedTail = nullptr;
}

// FIFO red blokiranih -- isto kao Scheduler, samo lokalno za ovaj semafor
void ksemaphore::putBlocked(TCB* tcb) {
    tcb->next = nullptr;
    if (blockedTail == nullptr) { blockedHead = tcb; blockedTail = tcb; }
    else { blockedTail->next = tcb; blockedTail = tcb; }
}

TCB* ksemaphore::getBlocked() {
    if (blockedHead == nullptr) return nullptr;
    TCB* first = blockedHead;
    blockedHead = blockedHead->next;
    if (blockedHead == nullptr) blockedTail = nullptr;
    first->next = nullptr;
    return first;
}

int ksemaphore::wait() {
    val--;                              // uzimam jednu

    if (val < 0) {                      // nije bilo slobodnih -> blokiraj se
        TCB* self = TCB::running;
        self->setBlocked(true);         // 1. oznaci: nisam spremna
        putBlocked(self);               // 2. stani u red OVOG semafora
        TCB::dispatch();                // 3. predaj procesor -> nit ovde NESTANE

        // ...nastavlja se tek kad je signal (ili close) probudi...
        if (closed) return -1;          // probudio me close -> greska
    }
    return 0;                           // uspeh
}

int ksemaphore::signal() {
    val++;                              // vracam jednu

    if (val <= 0) {                     // bilo je <0 -> neko ceka -> probudi ga
        TCB* t = getBlocked();
        if (t != nullptr) {
            t->setBlocked(false);       // vise nije blokirana
            Scheduler::put(t);          // vrati je u red SPREMNIH
        }
    }
    return 0;
}

int ksemaphore::wait_n(unsigned n) {
    val -= (int) n;                     // trazim n jedinica

    if (val < 0) {                      // nema dovoljno -> blokiraj se
        TCB* self = TCB::running;
        self->setBlocked(true);
        putBlocked(self);
        TCB::dispatch();

        if (closed) return -1;
    }
    return 0;
}

int ksemaphore::signal_n(unsigned n) {
    val += (int) n;                     // vracam n jedinica

    // oslobodjeno je n mesta -> probudi do n niti koje mogu da prodju
    while (val >= 0 && blockedHead != nullptr) {
        TCB* t = getBlocked();
        t->setBlocked(false);
        Scheduler::put(t);
    }
    return 0;
}

int ksemaphore::close(ksemaphore* sem) {
    if (sem == nullptr) return -1;

    sem->closed = true;                 // oznaci dealociran

    // deblokiraj SVE zatecene niti -- njihov wait ce vratiti -1
    TCB* t;
    while ((t = sem->getBlocked()) != nullptr) {
        t->setBlocked(false);
        Scheduler::put(t);
    }
    return 0;
}