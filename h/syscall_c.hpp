#pragma once
#include "../lib/hw.h"

void* mem_alloc(size_t size);
int   mem_free(void* ptr);

class _thread;
typedef _thread* thread_t;

int  thread_create(thread_t* handle, void(*start_routine)(void*), void* arg);
int  thread_exit();
void thread_dispatch();

void putc(char chr);
char getc();
class _sem;
typedef _sem* sem_t;

int sem_open(sem_t* handle, unsigned init);
int sem_close(sem_t handle);
int sem_wait(sem_t id);
int sem_signal(sem_t id);
int sem_wait_n(sem_t id, unsigned n);
int sem_signal_n(sem_t id, unsigned n);
int time_sleep(time_t t);