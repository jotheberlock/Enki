
fptr (Uint64) Uint64 entrypointtype

# Entrypoint is top-level function, sibling of load_import
entrypointtype entrypoint

def find_export(Byte^ name) Uint64
    Byte^ exports_ptr = __exports
    exports_ptr = exports_ptr + 8
    Uint64^ lenptr = exports_ptr
    Uint64 len = lenptr^
    exports_ptr = exports_ptr + 8
    exports_ptr = exports_ptr + len
    lenptr = exports_ptr
    Uint64 recs = lenptr^
    exports_ptr = exports_ptr + 8
    constif DEBUG
        write_num(recs)
        write(" entries in exports\n")
    Uint64 count = 0
    while count < recs
        lenptr = exports_ptr
        Uint64 reloc = lenptr^
        exports_ptr = exports_ptr + 8
        lenptr = exports_ptr
        Uint64 slen = lenptr^
        exports_ptr = exports_ptr + 8
        Uint64 ret = strcmp(exports_ptr, name)
        if ret == 1
            return reloc
        exports_ptr = exports_ptr + slen
        count = count + 1
    write("Failed to find import ")
    write(name)
    write(" !\n")
    exit(1)

def load_import(Byte^ file) Uint64
    constif DEBUG
        write("About to open ")
        write(file)
        write("\n")
    Uint64 handle = open_file(file)
    if handle > 0x10000000  # Bit of a hack, it's -1 (or -2 for some reason on Windows)
        write("Unable to open!\n")
        exit(2)
    constif DEBUG
        display_num("Handle: ", handle)
    Uint64 size = 42
    size = get_file_size(handle)
    constif DEBUG
        display_num("Size: ",size)
    Byte^ header = map_file(handle, 0, size, RW_PERMISSION)
    constif DEBUG
        display_num("Mapped at ", header)
    if header == 0
        exit(3)
    Uint64^ entryaddrp = header + 516
    Uint32^ rec = header + 524
    Uint32 recs = rec^
    Uint64 tmp = recs
    constif DEBUG
        write_num(tmp)
        write(" records\n\n")
    Uint32 count = 0
    rec = rec + 4
    Uint64[10] offsets
    Byte^ textptr
    Byte^ importsptr
    Uint64 textsize

    # Process sections
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
        constif DEBUG
            write("Arch ")
        tmp = arch    
        constif DEBUG
            write_num(tmp)
            write(" type ")
        tmp = type
        constif DEBUG
            write_num(tmp)
            write(" offset ")
        tmp = offset
        constif DEBUG
            write_num(tmp)
            write(" size ")
        tmp = size
        constif DEBUG
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
            constif DEBUG
                display_num("Text ptr ", textptr)
        elif type == 1
            remap(secptr, size, RW_PERMISSION)

    # Now process relocations
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
            constif DEBUG
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
            fromptr^ = toaddr
            val = fromptr^
        rtype = rec^

    # Fix up imports
    Uint64^ imports = header + 8192
    Uint64 modules = imports^
    imports = imports + 8
    constif DEBUG
        display_num("Module count ", modules)
    Uint64 mcount = 0
    Uint64 entryaddroff = entryaddrp^
    constif DEBUG
        display_num("Entry addr ", entryaddroff)
    entrypoint = textptr
    entrypoint = entrypoint + entryaddroff
    remap(textptr, textsize, EXECUTE_PERMISSION)
    Uint64$ ret = 0
    ret = entrypoint()
    Uint64^ frameptr = cast(ret, Uint64^)
    constif DEBUG
        display_num("Frame ptr ", frameptr)

    while mcount < modules
        Uint64 entries_offset = imports^
        Uint64^ entries = modules + entries_offset
        imports = imports + 8
        Uint64 mentries = imports^
        imports = imports + 8
        Uint64 strsize = imports^
        imports = imports + 8
        Byte^ mname = imports
        constif DEBUG
            write("Mentries ")
            write_num(mentries)
            write("\n")
            write("Strsize ")
            write_num(strsize)
            write("\n")
            write("Module ")
            write(mname)
            write("\n")
        mcount = mcount + 1
        imports = imports + entries_offset
        Uint64 fcount = 0
        while fcount < mentries
            Uint64^ fp = imports
            Uint64 fstrsize = imports^
            imports = imports + 8
            Uint64 addr = imports^
            imports = imports + 8
            Uint64 fstrlen = imports^
            imports = imports + 8
            Uint64 export_addr = find_export(imports)
            constif DEBUG
                write("Function ")
                write(imports)
                write(" stack offset ")
                write_num(addr)
                write(" resolves to ")
                write_num(export_addr)
                write("\n")
            Uint64^ to_write = frameptr + addr
            to_write^ = export_addr
            imports = imports + fstrsize
            fcount = fcount + 1
    constif DEBUG
        display_num("Jumping to ", entrypoint)
    !ret
    constif DEBUG
        display_num("Returned second ", ret)
    return ret

constif DEBUG    
    display_num("OS stack ptr ", __osstackptr)
Uint64 argc = get_argc()
constif DEBUG
    display_num("Argc ", argc)
    write("Argv0 ")
    write(get_argv(0))
    write("\n")
Byte^ ptr = "a.enk"
if argc > 1
    constif DEBUG
        write("Argv1 ")
        write(get_argv(1))
        write("\n")
    ptr = get_argv(1)
constif DEBUG
    write("Loading ")
    write(ptr)
    write("\n")

return load_import(ptr)
