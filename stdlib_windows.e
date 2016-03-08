extern KERNEL32.DLL:WriteFile(Uint64 output, Byte^ buffer, Uint64 chars, Uint64^ written, Uint64 overlapped)
extern KERNEL32.DLL:GetStdHandle(Uint64 handle)
extern KERNEL32.DLL:ReadFile(Uint64 input, Byte^ buffer, Uint64 chars, Uint64^ read, Uint64 overlapped)

def write(Byte^ ptr) Uint64
    Uint64 count 
    count = len(ptr)
    Uint64 written
    Uint64 handle = 0
    handle = GetStdHandle(-11)
    WriteFile(handle, ptr, 12, @written, 0)
    return count

def read(Byte^ ptr, Uint64 len) Uint64
    Uint64 num_read
    Uint64 handle = 0
    handle = GetStdHandle(-10)
    ReadFile(handle, ptr, len, @num_read, 0)
    return num_read

