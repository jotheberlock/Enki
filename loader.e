
fptr (Uint) Uint entrypointtype

# Entrypoint is top-level function, sibling of load_import
entrypointtype entrypoint

def find_export(Byte^ name) Uint
    Byte^ exports_ptr = __exports
    exports_ptr += 8
    Uint^ lenptr = exports_ptr
    Uint len = lenptr^
    exports_ptr += 8
    exports_ptr += len
    lenptr = exports_ptr
    Uint recs = lenptr^
    exports_ptr += 8
    constif DEBUG
        write_num(recs)
        write(" entries in exports\n")
    Uint count = 0
    while count < recs
        lenptr = exports_ptr
        Uint reloc = lenptr^
        exports_ptr += 8
        lenptr = exports_ptr
        Uint slen = lenptr^
        exports_ptr += 8
        Uint ret = strcmp(exports_ptr, name)
        if ret == 1
            return reloc
        exports_ptr += slen
        count += 1
    write("Failed to find import [")
    write(name)
    write("] !\n")
    exit(1)

struct raw InannaHeader
    Uint8[4] magic
    Uint32 version
    Uint32 archs_count
    Uint32 strings_offset
    Uint32 imports_offset

struct raw InannaArchHeader
    Uint32 arch
    Uint32 offset
    Uint32 sec_count
    Uint32 reloc_count
    Uint64 start_address

struct raw InannaSection
    Uint32 type
    Uint32 offset
    Uint32 length
    Uint32 name
    Uint64 vmem

struct raw InannaReloc
    Uint32 type
    Uint32 secfrom
    Uint32 secto
    Uint32 rshift
    Uint64 mask
    Uint32 lshift
    Uint32 bits
    Uint64 offset
    Uint64 offrom
    Uint64 offto

def load_arch(InannaArchHeader^ iah, Byte^ base, Uint soffset, Uint ioffset) Uint
    constif DEBUG
        display_num("\nArch ", iah^.arch)
        display_num("Offset ", iah^.offset)
        display_num("Sec count ", iah^.sec_count)
        display_num("Reloc count ", iah^.reloc_count)
        display_num("Start addr ", iah^.start_address)
        display_num("String offset ", soffset)
        display_num("Import offset ", ioffset)
    Uint count = iah^.sec_count
    Byte^ start = base+iah^.offset
    InannaSection^ is = cast(start, InannaSection^)
    if iah^.sec_count > 10
        write("Too many sections in object file!\n")
        return 1
    Uint64[10] offsets
    Uint64[10] sizes
    Uint64 scount = 0
    while scount < 10
        offsets[scount] = 0
        sizes[scount] = 0
        scount += 1
    while count > 0
        constif DEBUG
            display_num("\nSection type ", is^.type)
            display_num("Offset ", is^.offset)
            display_num("Length ", is^.length)
            display_num("Soffset ", soffset)
            Byte^ nameptr = base + soffset
            nameptr += is^.name
            write("Name ")
            write(nameptr)
            write("\n")
            display_num("Vmem ", is^.vmem)
        Byte^ offptr = base
        offptr += is^.offset
        offsets[is^.type] = offptr
        sizes[is^.type] = is^.length
        count -= 1
        is += 1
    InannaReloc^ ir = cast(is, InannaReloc^)
    count = iah^.reloc_count
    while count > 0
        constif DEBUG
            display_num("\nReloc type ", ir^.type)
        Uint itype = ir^.type
        # hmm why does elif not work
        if itype == 1
           Byte^ fromptrb = offsets[ir^.secfrom]
           fromptrb += ir^.offrom
           Uint64^ fromptr = cast(fromptrb, Uint64^)
           Uint64 toaddr = offsets[ir^.secto]
           toaddr += ir^.offto
           constif DEBUG
               write("Setting 64 bit ")
               write_num(fromptr)
               write(" to ")
               write_num(toaddr)
               write(" offset ")
               write_num(ir^.offto)
               write("\n")
           fromptr^ = toaddr
        if itype == 2
           Byte^ fromptrb = offsets[ir^.secfrom]
           fromptrb += ir^.offrom
           Uint32^ fromptr = cast(fromptrb, Uint32^)
           Uint32 toaddr = offsets[ir^.secto]
           toaddr += ir^.offto
           constif DEBUG
               write("Setting 32 bit ")
               write_num(fromptr)
               write(" to ")
               write_num(toaddr)
               write(" offset ")
               write_num(ir^.offto)
               write("\n")
           fromptr^ = toaddr
        count -= 1
        ir += 1

    Byte^ textsec = offsets[0]
    remap(textsec, sizes[0], EXECUTE_PERMISSION)
    Uint$ ret = 0
    Byte^ entrypointp = textsec
    entrypointp += iah^.start_address
    entrypoint = cast(entrypointp, entrypointtype)
    constif DEBUG
        display_num("About to do first call to ", entrypointp)
    ret = entrypoint()    
    Byte^ frameptr = cast(ret, Byte^)
    constif DEBUG
        display_num("Frame ptr is ", frameptr)
        
    Byte^ importbp = base + ioffset
    Uint64^ importp = cast(importbp, Uint64^)
    Uint64 modules = importp^
    importp += 1
    constif DEBUG
        display_num("Module count ", modules)
    Uint64 mcount = 0
    while mcount < modules
        Uint64 entries_offset = importp^
        Uint64^ entries = modules + entries_offset
        importp += 1
        Uint64 mentries = importp^
        importp += 1
        Uint64 strsize = importp^
        importp += 1
        Byte^ mname = importp
        constif DEBUG
            write("Module entries ")
            write_num(mentries)
            write("\n")
            write("Strsize ")
            write_num(strsize)
            write("\n")
            write("Module ")
            write(mname)
            write("\n")
        mcount += 1
        Byte^ twiddler
        twiddler = importp
        twiddler += entries_offset
        importp = twiddler
        Uint64 fcount = 0
        while fcount < mentries
            Uint64^ fp = importp
            Uint64 fstrsize = importp^
            importp += 1
            Uint64 addr = importp^
            importp += 1
            Uint64 fstrlen = importp^
            importp += 1
            Uint64 export_addr = find_export(importp)
            constif DEBUG
                write("Function ")
                write(importp)
                write(" stack offset ")
                write_num(addr)
                write(" resolves to ")
                write_num(export_addr)
                write("\n")
            Byte^ to_write = frameptr
            to_write += addr
            Uint64^ writer = to_write
            writer^ = export_addr
            twiddler = importp
            twiddler += fstrsize
            importp = twiddler
            fcount += 1
  
    constif DEBUG
        display_num("Jumping to ", entrypoint)
    !ret
    constif DEBUG
        display_num("Returned second ", ret)
    return ret
    
def load_import(Byte^ file) Uint
    constif DEBUG
        write("About to open ")
        write(file)
        write("\n")
    Uint handle = open_file(file)
    if handle > 0x10000000  # Bit of a hack, it's -1 (or -2 for some reason on Windows)
        write("Unable to open ")
        write(file)
        write("\n")
        exit(2)
    constif DEBUG
        display_num("Handle: ", handle)
    Uint size = 42
    size = get_file_size(handle)
    constif DEBUG
        display_num("Size: ",size)
    Byte^ header = map_file(handle, 0, size, RW_PERMISSION)
    constif DEBUG
        display_num("Mapped at ", header)
    if header == 0
        exit(3)
    Byte^ ptr = header + 512
    InannaHeader^ ih = cast(ptr, InannaHeader^)
    if ih^.magic[0] != 101
        display_num("Wrong magic 0 ",ih^.magic[0])
        return 1
    if ih^.magic[1] != 110
        display_num("Wrong magic 1 ",ih^.magic[1])
        return 1
    if ih^.magic[2] != 107
        display_num("Wrong magic 2 ",ih^.magic[2])
        return 1
    if ih^.magic[3] != 105
        display_num("Wrong magic 3 ",ih^.magic[3])
        return 1
    constif DEBUG
        display_num("Inanna version ", ih^.version)
        display_num("Archs ", ih^.archs_count)
        display_num("Strings offset ", ih^.strings_offset)
        display_num("Imports offset ", ih^.imports_offset)
    Uint count = 0
    ptr += 24
    InannaArchHeader^ iah = cast(ptr, InannaArchHeader^)
    while count < ih^.archs_count
        if iah^.arch == MACHINE_ARCH
            # FIXME: weirdness if we don't extract before call
            Uint so = ih^.strings_offset
            Uint io = ih^.imports_offset
            return load_arch(iah, header, so, io)
        count += 1
        iah += 1
    write("No suitable architecture found!\n")
    return 1

constif DEBUG    
    display_num("OS stack ptr ", __osstackptr)
Uint argc = get_argc()
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
