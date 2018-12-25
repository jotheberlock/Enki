
fptr (Uint64) Uint64 entrypointtype

# Entrypoint is top-level function, sibling of load_import
entrypointtype entrypoint

def find_export(Byte^ name) Uint64
    Byte^ exports_ptr = __exports
    exports_ptr += 8
    Uint64^ lenptr = exports_ptr
    Uint64 len = lenptr^
    exports_ptr += 8
    exports_ptr += len
    lenptr = exports_ptr
    Uint64 recs = lenptr^
    exports_ptr += 8
    constif DEBUG
        write_num(recs)
        write(" entries in exports\n")
    Uint64 count = 0
    while count < recs
        lenptr = exports_ptr
        Uint64 reloc = lenptr^
        exports_ptr += 8
        lenptr = exports_ptr
        Uint64 slen = lenptr^
        exports_ptr += 8
        Uint64 ret = strcmp(exports_ptr, name)
        if ret == 1
            return reloc
        exports_ptr += slen
        count += 1
    write("Failed to find import [")
    write(name)
    write("] !\n")
    exit(1)

def load_import(Byte^ file) Uint64
    constif DEBUG
        write("About to open ")
        write(file)
        write("\n")
    Uint64 handle = open_file(file)
    if handle > 0x10000000  # Bit of a hack, it's -1 (or -2 for some reason on Windows)
        write("Unable to open ")
        write(file)
        write("\n")
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
    Byte^ ptr = header + 512
    if ptr^ != 101
        display_num("Wrong magic 0 - ", ptr^)
        return 1
    ptr = ptr + 1
    if ptr^ != 110
        display_num("Wrong magic 1 - ", ptr^)
        return 1
    ptr = ptr + 1
    if ptr^ != 107
        display_num("Wrong magic 2 - ", ptr^)
        return 1
    ptr = ptr + 1
    if ptr^ != 105
        display_num("Wrong magic 3 - ", ptr^)
        return 1
    ptr = ptr + 1
    Uint32 iversion
    Uint32^ iversionp
    iversionp = cast(ptr, Uint32^)
    iversion = iversionp^
    display_num("Inanna version ", iversion)
    return 0

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
