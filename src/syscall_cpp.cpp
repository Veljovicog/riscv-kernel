#include "../h/syscall_cpp.hpp"

// ---- operator new/delete: obmotavaju mem_alloc/mem_free ----
void* operator new (size_t n)  { return mem_alloc(n); }
void  operator delete (void* p){ mem_free(p); }

// ---- Thread ----
Thread::Thread (void (*body)(void*), void* arg) {
    this->body = body;
    this->arg  = arg;
    this->myHandle = nullptr;      // nit se pravi tek u start()
}

Thread::Thread () {                // za izvedene klase (run)
    this->body = nullptr;
    this->arg  = nullptr;
    this->myHandle = nullptr;
}

Thread::~Thread () {
    // niti se ne dealociraju u ovom projektu
}

int Thread::start () {
    if (body != nullptr)
        return thread_create(&myHandle, body, arg);         // obican Thread
    else
        return thread_create(&myHandle, &wrapper, this);    // izvedeni: telo je run()
}

void Thread::wrapper(void* obj) {
    ((Thread*) obj)->run();        // pozovi polimorfni run() tog objekta
}

void Thread::dispatch () { thread_dispatch(); }

int Thread::sleep (time_t t) { return time_sleep(t); }

// ---- Semaphore ----
Semaphore::Semaphore (unsigned init) {
    sem_open(&myHandle, init);
}

Semaphore::~Semaphore () {
    sem_close(myHandle);
}

int Semaphore::wait ()   { return sem_wait(myHandle); }
int Semaphore::signal () { return sem_signal(myHandle); }

// ---- Console ----
char Console::getc ()        { return ::getc(); }
void Console::putc (char c)  { ::putc(c); }

PeriodicThread::PeriodicThread (time_t period) : Thread() {
    this->period = period;
}
//periodic
void PeriodicThread::run () {
    while (true) {
        periodicActivation();       // korisnikov posao (redefinisan u izvedenoj klasi)
        time_sleep(period);         // spavaj 'period' tikova
    }
}

void PeriodicThread::terminate () {
    // za sad prosto -- prava logika zaustavljanja je opciona
}