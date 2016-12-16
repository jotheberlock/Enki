struct Foo
    pass

struct Bar(Foo)
    pass

struct Frobnitz(Bar)
    pass

generic test(val) Uint64

def test(Foo^ val) Uint64
    write("Foo ")
    return 1

def test(Frobnitz^ val) Uint64
    write("Frobnitz ")
    return 2
    
Foo foo
Bar bar
Frobnitz frobnitz

Foo^ fooptr = @foo
Uint64 ret
ret = 42
ret = test(fooptr)
fooptr = @bar
ret = test(fooptr)
fooptr = @frobnitz
ret = test(fooptr)
return ret

