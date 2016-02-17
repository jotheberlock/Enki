extern KERNEL32.DLL:WriteFile(Uint64 output, Byte^ buffer, Uint64 chars, Uint64^ written, Uint64 overlapped)
extern KERNEL32.DLL:GetStdHandle(Uint64 handle)

Byte^ ptr = "Hello world\n"
# count = len(ptr) doesn't work for some reason hmm
Uint64 count 
count = len(ptr)
Uint64 written
Uint64 handle = 0
handle = GetStdHandle(-11)
WriteFile(handle, ptr, 12, @written, 0)
return count
