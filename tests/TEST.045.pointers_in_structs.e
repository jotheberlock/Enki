struct Foo
    Byte^ ptr1
    Byte^ ptr2

Foo foo

Foo^ fooptr
fooptr = @foo
fooptr^.ptr1 = "Hello\n"
fooptr^.ptr2 = "World\n"

Byte^ byteptr = "Argh"
return write(fooptr^.ptr2)


