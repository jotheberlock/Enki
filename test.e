extern KERNEL32.DLL:ExitProcess(Uint64 val)
extern USER32.DLL:MessageBoxA(Uint64 hwnd, Byte^ text, Byte^ caption, Uint64 type)

fptr extern (Uint64 hwnd, Byte^ text, Byte^ caption, Uint64 type) Uint64 Testptr
Testptr foo

foo = MessageBoxA
foo(0, "Hi", "Hello", 0)
ExitProcess(42)








