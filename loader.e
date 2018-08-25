
fptr (Uint64) Uint64 entrypointtype

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
    Byte^ header = map_file(handle, 0, size, RW_PERMISSION)
    write("Mapped at ")
    write_num(header)
    write("\n")
    Uint64^ entryaddrp = header + 516
    Uint32^ rec = header + 524
    Uint32 recs = rec^
    Uint64 tmp = recs
    write_num(tmp)
    write(" records\n\n")
    Uint32 count = 0
    rec = rec + 4
    entrypointtype entrypoint
    Uint64[10] offsets
    Byte^ textptr
    Uint64 textsize
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
        offsets[type] = secptr
        if type == 0
            textptr = secptr
            textsize = size
            write("Text ptr ")
            write_num(textptr)
            write("\n")
        elif type == 1
            remap(secptr, size, RW_PERMISSION)
    Uint64^ relocs = rec
    Uint64 rtype = rec^
    while rtype != 5
        if rtype == 1
            rec = rec + 8
            Uint64 fromsec = rec^
            rec = rec + 8
            Uint64 fromoff = rec^
            rec = rec + 8
            Uint64 tosec = rec^
            rec = rec + 8
            Uint64 tooff = rec^
            rec = rec + 8
            Uint64^ fromptr = offsets[fromsec]
            fromptr = fromptr + fromoff
            Uint64 toaddr = offsets[tosec]
            toaddr = toaddr + tooff
            write("Setting ")
            write_num(fromptr)
            write(" to ")
            write_num(toaddr)
            write(" offset ")
            write_num(tooff)
            write("\n")
            write("Current value ")
            Uint64 val = 0
            val = fromptr^
            write_num(val)
            write(" from ")
            write_num(fromptr)
            write("\n")
            fromptr^ = toaddr
            val = fromptr^
            write("Is now ")
            write_num(val)
            write("\n")
        rtype = rec^
        
    remap(textptr, textsize, EXECUTE_PERMISSION)
    Uint64 entryaddroff = entryaddrp^
    write("Entry addr ")
    write_num(entryaddroff)
    write("\n")
    entrypoint = textptr
    entrypoint = entrypoint + entryaddroff
    write("Jumping to ")
    write_num(entrypoint)
    write("\n")
    Uint64 ret = 0
    ret = entrypoint()
    write("\nReturned ")
    write_num(ret)
    write("\n")
    
write("OS stack ptr ")
write_num(__osstackptr)
write("\n")
Uint64^ argcp = __osstackptr
Uint64 argc = argcp^
write("Argc ")
write_num(argc)
write("\n")
Byte^^ argp = __osstackptr+8
Byte^ argv0 = argp^
write("Argv0 ")
write(argv0)
write("\n")
Byte^ ptr = "a.enk"
if argc > 1
    argp = argp + 8
    ptr = argp^
    write("Argv1 ")
    write(ptr)
    write("\n")
write("Loading ")
write(ptr)
write("\n")

load_import(ptr)
