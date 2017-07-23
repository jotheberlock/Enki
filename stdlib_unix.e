# On Linux at least brk() with an invalid value returns current break
# It's 2017 so 0 is valid pretty much everywhere
Uint base = __syscall(SYSCALL_BRK, 0)

def malloc(Uint len) Uint64
    Uint ret = base
    base = base + len
    base = __syscall(SYSCALL_BRK, base)
    return ret
    
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
