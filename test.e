extern USER32.DLL:MessageBoxA(Uint64 hwnd, Byte^ text, Byte^ caption, Uint64 type)
extern KERNEL32.DLL:ExitProcess(Uint64 val)

def Foo()
  Uint64 val = 4
  return val

MessageBoxA(0, "Hi", "Hello", 0)

MessageBoxA(0, "Hi", "Hello", 0)

Uint64 ret = Foo()
return ret













