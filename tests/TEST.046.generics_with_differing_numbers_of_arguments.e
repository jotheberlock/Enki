generic foo() Uint64

def foo(Uint64 a) Uint64
    write("Hi")

def foo(Uint64 a, Uint64 b) Uint64
    write("Hello")

Uint64 var = 0
foo(var,var)
foo(var)
return 0
