struct Foo
    pass

struct Bar(Foo)
    pass

struct Frobnitz(Bar)
    pass

generic test(val) Uint

def test(Foo^ val) Uint
    write("Foo ")
    return 1

def test(Frobnitz^ val) Uint
    write("Frobnitz ")
    return 2
    
Foo foo
Bar bar
Frobnitz frobnitz

Foo^ fooptr = @foo
Uint ret
ret = 42
ret = test(fooptr)
fooptr = @bar
ret = test(fooptr)
fooptr = @frobnitz
ret = test(fooptr)
return ret

