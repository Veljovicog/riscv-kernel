.global enterThreadMode

# Prelazak iz sistemskog u rezim niti (odredjen bitom SPP).
# Vraca se pozivaocu, isto kao obican ret -- samo menja rezim usput.

enterThreadMode:
    li   t0, 0x20      # SPIE = bit 5
    csrs sstatus, t0   # SPIE = 1  -> nova nit krece sa dozvoljenim prekidima
                       #             (posle sret bice SIE = SPIE = 1 -> preotimljiva)
    csrw sepc, ra      # sepc = adresa povratka pozivaocu
    sret               # skoci na sepc, rezim uzmi iz SPP, SIE <- SPIE
