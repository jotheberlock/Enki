def malloc(Uint len) Uint
    Uint ret
    Uint32 protect = PROT_READWRITE   
    Uint32 flags = MAP_ANONYMOUS_PRIVATE
    ret = __syscall(SYSCALL_MMAP, 0, len, protect, flags, 0, 0)
    return ret
    
def free(Uint^ addr)
    __syscall(SYSCALL_MUNMAP,addr,4096)