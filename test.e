struct Foo
   Uint64 a

struct Bar(Foo)
   Uint64 b

generic foo(param)

def foo(Uint64 param)
   write("Uint64")

def foo(Byte^ param)
   write("Byte^")

def foo(Foo^ param)
   write("Foo^")

def foo(Bar^ param)
   write("Bar^")

foo(42)
