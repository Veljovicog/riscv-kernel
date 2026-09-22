#ifndef _tcb_hpp_
#define _tcb_hpp_

#include "../lib/hw.h"

class TCB {
public:
    using Body = void (*)(void*);

    struct Context {
        uint64 ra;          // ofset 0  -- mora se poklapati sa contextSwitch.S
        uint64 sp;          // ofset 8
    };

    static TCB* createThread(Body body, void* arg, void* stackSpace, bool kernelMode = false);
    static void dispatch();
    static void exit();

    bool isFinished() const  { return finished; }
    void setFinished(bool f) { finished = f; }
    bool isBlocked() const   { return blocked; }
    void setBlocked(bool b)  { blocked = b; }

    static TCB* running;            // nit koja se TRENUTNO izvrsava
    static uint64 timeSliceCounter; // koliko tikova traje TRENUTNI kvantum (jedna staticka)

    uint64 timeSleeping;          // > 0: spavam jos toliko tikova
    static TCB* sleepHead;        // lista uspavanih niti
    static void sleep(uint64 ticks);
    static void tick();        // tajmer poziva svaki tik: budi usnule niti

private:
    Body    body;                   // funkcija koju nit izvrsava
    void*   arg;                    // argument te funkcije
    void*   stackSpace;             // vrh njenog steka
    Context context;                // gde je nit stala
    bool    finished;               // zavrsila se
    bool    blocked;                // ceka na semaforu
    bool    kernelMode;             // true = telo radi u sistemskom rezimu
    TCB*    next;                   // ulancavanje u red spremnih

    static void threadWrapper();    // ulazna tacka svake nove niti

    friend class Scheduler;
    friend class ksemaphore;


};

extern "C" void contextSwitch(TCB::Context* oldC, TCB::Context* newC);
extern "C" void enterThreadMode();

#endif