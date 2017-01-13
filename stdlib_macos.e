def write(Byte^ ptr) Uint64
    Uint64 count 
    count = len(ptr)
    Uint64 written
    written = __syscall(0x2000004, 1, ptr, count)
    return written

def read(Byte^ ptr, Uint64 len) Uint64
    Uint64 count = __syscall(0x2000003, 0, ptr, len)
    ptr = ptr + count
    ptr ^= 0
    return count
