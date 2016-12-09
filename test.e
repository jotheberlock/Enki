struct Foo
    Uint64 thing

generic test(val) Uint64

def test(Foo^ val) Uint64
    write("Foo ")
    return 1

Foo foo
Foo^ fooptr = @foo
Uint64 ret
ret = 42
ret = test(fooptr)
return ret

