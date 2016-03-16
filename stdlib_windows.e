extern KERNEL32.DLL:WriteFile(Uint64 output, Byte^ buffer, Uint32 chars, Uint32^ written, Uint64 overlapped)
extern KERNEL32.DLL:GetStdHandle(Uint64 handle)
extern KERNEL32.DLL:ReadFile(Uint64 input, Byte^ buffer, Uint32 chars, Uint32^ read, Uint64 overlapped)

def write(Byte^ ptr) Uint64
    Uint32 count 
    count = len(ptr)
    Uint32 written32
    Uint64 handle = 0
    handle = GetStdHandle(-11)
    WriteFile(handle, ptr, count, @written32, 0)
    Uint64 written = written32
    return written

def read(Byte^ ptr, Uint64 len) Uint64
    Uint32 len32 = len
    Uint32 num_read32
    Uint64 handle = 0
    handle = GetStdHandle(-10)
    ReadFile(handle, ptr, len32, @num_read32, 0)
    Uint64 num_read = num_read32
    ptr = ptr + num_read
    ptr^ = 0
    return num_read

