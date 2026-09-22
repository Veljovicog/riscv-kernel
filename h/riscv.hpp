#pragma once
#include "../lib/hw.h"

class Riscv {
public:
    static uint64 r_scause();               // zasto smo upali u prekid
    static uint64 r_sepc();                  // gde je nit stala
    static void   w_sepc(uint64 sepc);
    static uint64 r_stvec();                 // adresa prekidne rutine
    static void   w_stvec(uint64 stvec);
    static uint64 r_sstatus();
    static void   w_sstatus(uint64 sstatus);

    enum BitMaskSstatus {
        SSTATUS_SIE  = (1 << 1),
        SSTATUS_SPIE = (1 << 5),
        SSTATUS_SPP  = (1 << 8),
    };

    static void ms_sstatus(uint64 mask);   // mask set   -> postavi bite na 1
    static void mc_sstatus(uint64 mask);   // mask clear -> obrisi bite na 0
    static void clearTimerFlag();     // spusti zastavicu tajmera (bit 1 u sip)
    static void enableExternalInterrupts();
};

inline uint64 Riscv::r_scause() {
    uint64 v;
    __asm__ volatile("csrr %0, scause":"=r"(v));
    return v;
}
inline uint64 Riscv::r_sepc() {
    uint64 v;
    __asm__ volatile("csrr %0, sepc"  :"=r"(v));
    return v;
}
inline void   Riscv::w_sepc(uint64 x) {
    __asm__ volatile("csrw sepc, %0" : :"r"(x));
}
inline uint64 Riscv::r_stvec() {
    uint64 v;
    __asm__ volatile("csrr %0, stvec" :"=r"(v));
    return v;
}
inline void   Riscv::w_stvec(uint64 x) {
    __asm__ volatile("csrw stvec, %0" : :"r"(x));
}
inline uint64 Riscv::r_sstatus() {
    uint64 v;
    __asm__ volatile("csrr %0, sstatus":"=r"(v));
    return v;
}
inline void   Riscv::w_sstatus(uint64 x) {
    __asm__ volatile("csrw sstatus, %0" : :"r"(x));
}

inline void Riscv::ms_sstatus(uint64 m) {
    __asm__ volatile("csrs sstatus, %0" : : "r"(m));
}
inline void Riscv::mc_sstatus(uint64 m) {
    __asm__ volatile("csrc sstatus, %0" : : "r"(m));
}

inline void Riscv::clearTimerFlag() { __asm__ volatile("csrc sip, %0" : : "r"(2)); }

inline void Riscv::enableExternalInterrupts() {
    __asm__ volatile("csrs sie, %0" : : "r"(1 << 9));   // SEIE bit 9
}