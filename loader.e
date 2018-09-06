
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
    return 0

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
    Uint64[10] offsets
    Byte^ textptr
    Byte^ importsptr
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
    Uint64^ imports = header + 4096
    Uint64 modules = imports^
    imports = imports + 8
    write("Module count ")
    write_num(modules)
    write("\n")
    Uint64 mcount = 0
    while mcount < modules
        Uint64 entries_offset = imports^
        Uint64^ entries = modules + entries_offset
        imports = imports + 8
        Uint64 mentries = imports^
        imports = imports + 8
        Uint64 strsize = imports^
        imports = imports + 8
        Byte^ mname = imports
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
            write("Function ")
            write(imports)
            write(" stack offset ")
            write_num(addr)
            write(" resolves to ")
            write_num(export_addr)
            write("\n")
            imports = imports + fstrsize
            fcount = fcount + 1
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
    return ret
    
write("OS stack ptr ")
write_num(__osstackptr)
write("\n")
Uint64 argc = get_argc()
write("Argc ")
write_num(argc)
write("\n")
write("Argv0 ")
write(get_argv(0))
write("\n")
Byte^ ptr = "a.enk"
if argc > 1
    write("Argv1 ")
    write(get_argv(1))
    write("\n")
    ptr = get_argv(1)
write("Loading ")
write(ptr)
write("\n")

return load_import(ptr)
