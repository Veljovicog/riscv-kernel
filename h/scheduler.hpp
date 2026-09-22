#ifndef _scheduler_hpp_
#define _scheduler_hpp_

class TCB;                       // dovoljna je najava, ne treba ceo header

class Scheduler {
public:
    static TCB* get();           // uzmi prvu spremnu nit (nullptr ako je red prazan)
    static void put(TCB* tcb);   // ubaci nit na KRAJ reda

private:
    static TCB* head;            // pocetak reda
    static TCB* tail;            // kraj reda
};

#endif