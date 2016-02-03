extern KERNEL32.DLL:WriteFile(Uint64 output, Byte^ buffer, Uint64 chars, Uint64^ written, Uint64 overlapped)
extern KERNEL32.DLL:GetStdHandle(Uint64 handle)

Uint64 written
Uint64 handle = 0

handle = GetStdHandle(-11)
WriteFile(handle,"Hello world\n", 12, @written, 0)
return written










