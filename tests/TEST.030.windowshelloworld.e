extern KERNEL32:WriteFile(Uint64 output, Byte^ buffer, Uint32 chars, Uint32^ written, Uint64 overlapped)
extern KERNEL32:GetStdHandle(Uint64 handle)

Uint32 written
Uint64 handle = 0

handle = GetStdHandle(-11)
Uint32 no_chars = 12
WriteFile(handle,"Hello world\n", no_chars, @written, 0)
return written
