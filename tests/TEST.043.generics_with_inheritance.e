struct Foo
    Uint64 thing

struct Bar(Foo)
    Uint64 another_thing

generic test(val) Uint64

def test(Foo^ val) Uint64
    write("Foo ")
    return 1

def test(Bar^ val) Uint64
    write("Bar ")
    return 2
    
Foo foo
Foo^ fooptr = @foo
Uint64 ret
ret = 42
ret = test(fooptr)
Bar bar
Bar^ barptr = @bar
ret = test(barptr)
fooptr = @bar
ret = test(fooptr)
return ret

