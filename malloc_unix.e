# On Linux at least brk() with an invalid value returns current break
# It's 2017 so 0 is valid pretty much everywhere
Uint base = __syscall(SYSCALL_BRK, 0)

def malloc(Uint len) Uint64
    Uint ret = base
    base = base + len
    base = __syscall(SYSCALL_BRK, base)
    return ret
    
