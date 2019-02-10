def foo(Uint count) Uint
    Byte[10] buffer
    num_to_str(count, @buffer)
    write(@buffer)
    count = count - 1
    if count == 0
        return
    foo(count)
    
foo(5)
return 0

