#ifndef _console_hpp_
#define _console_hpp_

#include "../lib/hw.h"

class ksemaphore;

// Sopstvena konzola (zadatak 4):
//  - IZLAZ: kruzni bafer + interna kernel nit koja proziva UART TX i salje
//  - ULAZ:  kruzni bafer koji puni prekidna rutina (IRQ 10), getc blokira na semaforu
class KConsole {
public:
    static void init();          // pozvati u main-u (kernelMode, pre ukljucivanja SIE)

    static void doPutc(char c);  // iz putc syscalla (izvrsava se sa SIE=0)
    static char doGetc();        // iz getc syscalla (izvrsava se sa SIE=0)

    static void handleRx();      // iz spoljasnjeg (PLIC) prekida za konzolu, SIE=0

    static bool outEmpty();      // true ako je izlazni bafer prazan (za flush na kraju)

private:
    static const int BUF = 256;

    static char outBuf[BUF];
    static int  outHead, outTail;     // outHead pise TX nit, outTail pise putc
    static char inBuf[BUF];
    static int  inHead, inTail;       // inHead pise getc, inTail pise RX prekid

    static ksemaphore* outItems;      // koliko znakova ceka slanje
    static ksemaphore* outSpace;      // koliko slobodnih mesta u izlaznom baferu
    static ksemaphore* inItems;       // koliko znakova pristiglo sa tastature

    static void txThreadBody(void*);  // telo interne kernel niti koja salje
};

#endif
