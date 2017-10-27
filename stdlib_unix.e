def write(Byte^ ptr) Uint
    Uint count 
    count = len(ptr)
    Uint written
    written = __syscall(SYSCALL_WRITE, STDOUT, ptr, count)
    return written

def read(Byte^ ptr, Uint len) Uint
    Uint count = __syscall(SYSCALL_READ, STDIN, ptr, len)
    ptr = ptr + count
    ptr ^= 0
    return count
