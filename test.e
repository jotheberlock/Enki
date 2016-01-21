extern KERNEL32.DLL:WriteConsoleA(Uint64 output, Byte^ buffer, Uint64 chars, Uint64^ written, Uint64 reserved)
extern KERNEL32.DLL:GetStdHandle(Uint64 handle)
extern KERNEL32.DLL:AllocConsole()

Uint64 written
Uint64 handle = 0

AllocConsole()
handle = GetStdHandle(-11)
WriteConsoleA(handle,"Hello world\n", 12, @written, 0)
return handle









