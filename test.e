extern USER32.DLL:MessageBoxA(Uint64 hwnd, Byte^ text, Byte^ caption, Uint64 type)
extern KERNEL32.DLL:ExitProcess(Uint64 val)

def return_a_thing()
    MessageBoxA(0, "Hi", "Hello", 0)
    2

Uint64 $ thing
thing = return_a_thing()












