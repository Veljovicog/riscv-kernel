# RISC-V Kernel

A small preemptive multitasking kernel for 64-bit RISC-V, running bare metal under QEMU. No operating system underneath it, no standard library, no bootloader — QEMU jumps straight into this image.

Built for the Operating Systems course at the School of Electrical Engineering, University of Belgrade. The course supplies the project skeleton, the linker script, the hardware library (`lib/`) and the test suite (`test/`); **everything under `src/` and `h/` is mine.**

## What it does

**Allocates memory.** A best-fit allocator over the heap region, with a free list kept sorted by address. Each allocation is prefixed with a one-word header holding its size in blocks, so `mem_free` knows how much to give back without being told. A freed block is merged with the neighbour before it and the neighbour after it when they turn out to be adjacent, which keeps the heap from degrading into unusable slivers. `operator new` and `operator delete` are routed through it, so ordinary C++ allocation works in a freestanding environment.

**Runs threads.** Each thread has a `TCB` with its own stack and a two-word context — return address and stack pointer. That is all the context that needs saving, because `contextSwitch` is an ordinary function call: the compiler has already spilled whatever callee-saved registers the caller cared about. A newly created thread has `threadWrapper` planted as its return address, so the very first switch into it "returns" into the thread body.

**Preempts.** The timer interrupt increments a counter against the time slice; when the slice runs out, the scheduler switches. Threads can also yield with `thread_dispatch`. The ready queue is a plain FIFO.

**Synchronises.** Counting semaphores whose value is allowed to go negative — the magnitude of a negative value *is* the number of waiters. `wait` blocks a thread onto the semaphore's own FIFO queue; `signal` moves one back to the ready queue. `wait_n` and `signal_n` do the same n units at a time. Closing a semaphore releases every thread parked on it, and their `wait` returns −1 rather than success.

**Sleeps.** `time_sleep` parks a thread on a list of sleepers, which the timer walks on each tick, waking any whose counter has reached zero.

**Has its own console.** The one part where the course's library is deliberately not used. Output goes into a circular buffer guarded by two semaphores (items and free space); an internal kernel thread drains it, polling the UART's transmit-ready bit, so a thread calling `putc` never busy-waits on hardware. Input is filled by the UART receive interrupt through the PLIC and drained by `getc`, which blocks on a semaphore until a character arrives.

Two details in there are worth more than they look:

- Only the **receive** interrupt is enabled on the UART. Enabling the transmit interrupt as well would mean an empty transmit register raises an interrupt continuously — an interrupt storm that starves everything else. Transmission is done by polling instead, on a thread that can afford to wait.
- The transmit thread runs with interrupts enabled, like any other thread, so it must **not** touch kernel semaphores directly — it goes through the C API and therefore through `ecall`, which runs with interrupts off. `putc`, which is already inside a syscall, calls them directly. Same semaphores, two different routes, chosen by which side of the trap the caller is on.

**Separates user and supervisor mode.** User threads run in user mode and reach the kernel only through `ecall`. Which mode a thread resumes in is decided in `dispatch` by setting or clearing `SPP`, and applied by the `sret` that ends the trap — or, for a thread that has never run, by `enterThreadMode`, which is a `ret` that changes privilege level on the way out.

**Reports what it cannot handle.** An unexpected trap prints the cause and the faulting PC in hex over the UART and halts, using a direct-to-UART writer rather than the console — because when the trap handler is the thing that failed, the console is not something to rely on.

## The part that actually took the time

`interrupts.S` saves `sepc` and `sstatus` **on the stack**, not just in their CSRs, along with all 31 general-purpose registers, and restores them from there before `sret`.

It has to work that way. A trap can switch threads before it returns — that is the whole point of preemption — and `sepc`/`sstatus` belong to the thread that was interrupted, not to the CPU. If they were left in the CSRs, a context switch inside the handler would return the *new* thread to the *old* thread's program counter.

It also means that skipping the `ecall` instruction is done by adding 4 to the saved `sepc` in the frame, not to the CSR. Writing the CSR instead is the classic version of this bug: the CSR gets overwritten on the way out, the thread returns to the same `ecall`, and the program hangs in an invisible loop. The same reasoning applies to syscall return values — they go into the saved `a0` slot in the frame, which is what the restore sequence will load, rather than into the register that is about to be overwritten.

## Syscalls

The trap handler dispatches on a code passed in `a0`:

| Area | Calls |
|---|---|
| Memory | `mem_alloc`, `mem_free` |
| Threads | `thread_create`, `thread_exit`, `thread_dispatch`, `time_sleep` |
| Semaphores | `sem_open`, `sem_close`, `sem_wait`, `sem_signal`, `sem_wait_n`, `sem_signal_n` |
| Console | `getc`, `putc` |

Two APIs sit on top: a C one (`syscall_c.hpp`) and a C++ one (`syscall_cpp.hpp`) with `Thread`, `Semaphore`, `Console` and `PeriodicThread`, so a user program can be written in either style. `Thread` supports both forms the assignment asks for — a function pointer passed to the constructor, and a subclass overriding `run()`.

## Building and running

Needs a RISC-V cross-compiler (`riscv64-unknown-elf-` or `riscv64-linux-gnu-`) and `qemu-system-riscv64`.

```bash
make            # build the kernel image
make qemu       # run it under QEMU
```

QEMU is started as `-machine virt -bios none -kernel kernel -m 128M -smp 1 -nographic`: no firmware, no bootloader — the kernel image *is* what the machine starts executing.

Built with `-nostdlib -ffreestanding -fno-common -fno-rtti -march=rv64ima -mabi=lp64 -mcmodel=medany`, and `-Wall -Werror`. Everything compiles without a single warning.

On startup the kernel asks for a test number and runs the corresponding test from `test/`:

| Test | What it exercises |
|---|---|
| 1, 2 | Threads and synchronous context switching, C and C++ API |
| 3, 4 | Producer/consumer over semaphores, C and C++ API |
| 5 | `time_sleep` |
| 6 | Producer/consumer with asynchronous (preemptive) switching |
| 7 | User mode — user code attempts a privileged instruction |

Test 7 is expected **not** to terminate normally: the privileged instruction traps, and the kernel prints the cause and the faulting address. That is the test passing, not failing.

## Structure

```
h/      MemoryAllocator, Riscv, Scheduler, TCB, ksemaphore,
        KConsole, syscall_c, syscall_cpp
src/    the implementations, plus
          interrupts.S       — trap entry: saves the full frame, calls
                               handleSupervisorTrap, restores, sret
          contextSwitch.S    — the context switch itself
          enterThreadMode.S  — first entry into a thread, at its own
                               privilege level
lib/    the course's hardware library (provided, binary)
test/   the course's test suite
```
