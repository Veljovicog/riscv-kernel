#ifndef _semaphore_hpp_
#define _semaphore_hpp_

#include "../lib/hw.h"
inline void* operator new(size_t, void* p) {
    return p;
}

class TCB;                       // dovoljna najava (ne treba ceo tcb.hpp)

class ksemaphore {
public:
    ksemaphore(unsigned init);

    int wait();                  // trazi 1 jedinicu
    int signal();                // vraca 1 jedinicu
    int wait_n(unsigned n);      // trazi n jedinica  (novo u 2026)
    int signal_n(unsigned n);    // vraca n jedinica  (novo u 2026)

    static int close(ksemaphore* sem);   // dealokacija: deblokira sve sa greskom

private:
    int   val;                   // brojac: >=0 slobodne jedinice, <0 koliko ih ceka
    bool  closed;                // da li je semafor dealociran
    TCB*  blockedHead;           // pocetak reda blokiranih niti
    TCB*  blockedTail;           // kraj reda

    void putBlocked(TCB* tcb);   // ubaci nit u red ovog semafora
    TCB* getBlocked();           // izvadi prvu blokiranu nit
};

#endif
