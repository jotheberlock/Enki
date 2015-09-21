extern USER32.DLL:MessageBoxA(Uint64 hwnd, Byte^ text, Byte^ caption, Uint64 type)
extern KERNEL32.DLL:ExitProcess(Uint64 val)

Byte^ text = "Hello World"
MessageBoxA(0, text, text, 0)
ExitProcess(42)










