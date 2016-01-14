extern KERNEL32.DLL:WriteConsoleA(Uint64 handle, Byte^ buffer, Uint64 towrite, Uint64^ wrote, Uint64 reserved) Uint64

Uint64 written
WriteConsoleA(7, "Hello world\n", 12, @written, 0)
return written
