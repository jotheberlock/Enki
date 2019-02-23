generic foo() Uint

def foo(Uint a) Uint
    write("Hi")

def foo(Uint a, Uint b) Uint
    write("Hello")

Uint var = 0
foo(var,var)
foo(var)
return 0
