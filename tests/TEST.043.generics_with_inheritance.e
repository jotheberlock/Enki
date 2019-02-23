struct Foo
    Uint thing

struct Bar(Foo)
    Uint another_thing

generic test(val) Uint

def test(Foo^ val) Uint
    write("Foo ")
    return 1

def test(Bar^ val) Uint
    write("Bar ")
    return 2
    
Foo foo
Foo^ fooptr = @foo
Uint ret
ret = 42
ret = test(fooptr)
Bar bar
Bar^ barptr = @bar
ret = test(barptr)
fooptr = @bar
ret = test(fooptr)
return ret

