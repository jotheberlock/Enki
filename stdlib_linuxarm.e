def write(Byte^ ptr) Uint
    Uint count 
    count = len(ptr)
    Uint written
    written = __syscall(4, 1, ptr, count)
    return written

def read(Byte^ ptr, Uint len) Uint
    Uint count = __syscall(3, 0, ptr, len)
    ptr = ptr + count
    ptr ^= 0
    return count
