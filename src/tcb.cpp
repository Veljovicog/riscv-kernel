#include "../h/tcb.hpp"
#include "../h/scheduler.hpp"
#include "../h/riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/syscall_c.hpp"

TCB* TCB::running = nullptr;
TCB* TCB::sleepHead = nullptr;
uint64 TCB::timeSliceCounter = 0;

TCB* TCB::createThread(Body body, void* arg, void* stackSpace, bool kernelMode)
{
    // jezgro alocira DIREKTNO -- ne sme ecall unutar jezgra
    TCB* tcb = (TCB*) MemoryAllocator::mem_alloc(sizeof(TCB));
    if (tcb == nullptr) return nullptr;

    tcb->body       = body;
    tcb->arg        = arg;
    tcb->stackSpace = stackSpace;
    tcb->finished   = false;
    tcb->blocked    = false;
    tcb->kernelMode = kernelMode;
    tcb->next       = nullptr;
    tcb->timeSleeping = 0;

    // laziran pocetni kontekst: nit jos nije radila
    tcb->context.ra = (uint64) &threadWrapper;   // "vrati se" u omotac
    // vrh steka mora biti 16-poravnat (RISC-V ABI); stek raste nanize
    tcb->context.sp = ((uint64) stackSpace) & ~0xFUL;

    if (body != nullptr) Scheduler::put(tcb);    // main ne ide u red

    return tcb;
}

// OVDE je threadWrapper -- ulazna tacka svake nove niti
void TCB::threadWrapper()
{
    enterThreadMode();               // 1. predji u rezim koji je dispatch odredio
    running->body(running->arg);     // 2. izvrsi telo niti
    thread_exit();                   // 3. nit gotova, predaj procesor
}

void TCB::dispatch()
{
    timeSliceCounter = 0;            // svaki izbor niti krece sa svezim kvantumom
    TCB* old = running;
    if (!old->isFinished() && !old->isBlocked()) Scheduler::put(old);

    running = Scheduler::get();

    while (running == nullptr) {                  // ako je red prazan...
        Riscv::ms_sstatus(Riscv::SSTATUS_SIE);    //   nakratko upali prekide
        Riscv::mc_sstatus(Riscv::SSTATUS_SIE);    //   pa ih ugasi
        running = Scheduler::get();               //   probaj opet
    }
    // odluka o rezimu -- primenice je sret (u enterThreadMode ili u interrupts.S)
    if (running->kernelMode) Riscv::ms_sstatus(Riscv::SSTATUS_SPP);
    else                     Riscv::mc_sstatus(Riscv::SSTATUS_SPP);

    contextSwitch(&old->context, &running->context);
}

void TCB::exit()
{
    running->setFinished(true);
    dispatch();                      // ovamo se nikad ne vraca
}

void TCB::sleep(uint64 ticks) {
    if (ticks == 0) return;
    running->timeSleeping = ticks;
    running->setBlocked(true);        // ne vracaj je u red spremnih
    running->next = sleepHead;        // ubaci u listu usnulih
    sleepHead = running;
    dispatch();                       // predaj procesor -> nit spava
}

void TCB::tick() {
    TCB* prev = nullptr;
    TCB* cur = sleepHead;
    while (cur != nullptr) {
        cur->timeSleeping--;
        if (cur->timeSleeping == 0) {
            TCB* woken = cur;
            if (prev == nullptr) sleepHead = cur->next;
            else prev->next = cur->next;
            cur = cur->next;
            woken->setBlocked(false);
            Scheduler::put(woken);
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
}