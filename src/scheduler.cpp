#include "../h/scheduler.hpp"
#include "../h/tcb.hpp"

TCB* Scheduler::head = nullptr;
TCB* Scheduler::tail = nullptr;

TCB* Scheduler::get()
{
    if (head == nullptr) return nullptr;     // red je prazan

    TCB* first = head;
    head = head->next;
    if (head == nullptr) tail = nullptr;     // izvadili smo poslednju
    first->next = nullptr;
    return first;
}

void Scheduler::put(TCB* tcb)
{
    tcb->next = nullptr;

    if (tail == nullptr) {                   // red je bio prazan
        head = tcb;
        tail = tcb;
    } else {
        tail->next = tcb;
        tail = tcb;
    }
}
