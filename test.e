def dump_exports(Byte^ ptr) Uint64
    Uint64^ intptr = ptr
    Uint64 lenval = intptr^
    ptr = ptr + 8
    Byte^ nameptr = ptr
    write(nameptr)
    write("\n")
    ptr = ptr + lenval
    intptr = ptr
    Uint64 entries = intptr^
    ptr = ptr + 8
    Uint64 count = entries
    while count > 0
        intptr = ptr
        Uint64 reloc = intptr^
        write_num(reloc)
        write(" ")
        ptr = ptr + 8
        intptr = ptr
        lenval = intptr^
        ptr = ptr + 8
        write(ptr)
        ptr = ptr + lenval
        write("\n")
        count = count - 1

dump_exports(__exports)



