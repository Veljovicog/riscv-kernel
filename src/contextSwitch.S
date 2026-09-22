.global contextSwitch

# contextSwitch(Context* stari, Context* novi)
#   a0 = stari, a1 = novi

contextSwitch:
    sd ra, 0(a0)        # stari->ra = ra
    sd sp, 8(a0)        # stari->sp = sp

    ld ra, 0(a1)        # ra = novi->ra
    ld sp, 8(a1)        # sp = novi->sp   <- od sad si na drugom steku

    ret