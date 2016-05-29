def write(Byte^ ptr) Uint64
    Uint64 count 
    count = len(ptr)
    Uint64 written
    written = __syscall(1, 1, ptr, count)
    return written

def read(Byte^ ptr, Uint64 len) Uint64
    Uint64 count = __syscall(0, 0, ptr, len)
    ptr = ptr + count
    ptr ^= 0
    return count
