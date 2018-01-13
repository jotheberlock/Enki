extern KERNEL32:VirtualAlloc(Uint64^ addr, Uint64 size, Uint32 alloc_type, Uint32 protect)
extern KERNEL32:VirtualFree(Uint64^ addr, Uint64 size, Uint32 free_type)

def malloc(Uint len) Uint64
    Uint ret
    Uint32 alloc_type = 0x1000 
    Uint32 protect = 0x40      
    ret = VirtualAlloc(0, len, alloc_type, protect)
    return ret
    
def free(Uint64^ addr)
    Uint32 free_type = 0x4000 
    VirtualFree(addr,0,free_type)