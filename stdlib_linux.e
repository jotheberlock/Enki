def write(Byte^ ptr) Uint64
    Uint64 count 
    count = len(ptr)
    Uint64 written
    written = __syscall(1, 1, ptr, count)
    return written
