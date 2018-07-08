def load_import(Byte^ file) Uint64
    write("About to open\n")
    Uint64 handle = open_file(file)
    write("Handle: ")
    write_num(handle)
    write("\n")
    Uint64 size = 42
    size = get_file_size(handle)
    write("Size: ")
    write_num(size)
    write("\n")
    Byte^ header = map_file(handle, 0, size, READ_PERMISSION)
    Uint32^ rec = header + 516
    Uint32 recs = rec^
    Uint64 tmp = recs
    write_num(tmp)
    write(" records\n\n")
    Uint32 count = 0
    rec = rec + 4
    while count < recs
        Uint32 arch = rec^
        rec = rec + 4
        Uint32 type = rec^
        rec = rec + 4
        Uint32 offset = rec^
        rec = rec + 4
        Uint32 size = rec^
        rec = rec + 4
        Uint64^ rec64 = rec
        Uint64 vmem = rec64^
        rec = rec + 8
        write("Arch ")
        tmp = arch
        write_num(tmp)
        write(" type ")
        tmp = type
        write_num(tmp)
        write(" offset ")
        tmp = offset
        write_num(tmp)
        write(" size ")
        tmp = size
        write_num(tmp)
        write(" vmem ")
        write_num(vmem)
        write("\n")
        count = count + 1
        Byte^ secptr = header
        secptr = secptr + offset
        Uint64 size64 = size
        if type == 0
            remap(secptr, size64, EXECUTE_PERMISSION)
        elif type == 1
            remap(secptr, size64, RW_PERMISSION)

write("Loading a.enk\n")
Byte^ ptr = "a.enk"
load_import(ptr)
