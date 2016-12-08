generic test(val,val2) Uint64

def test(Uint64 val, Uint64 val2) Uint64
    write("Uint64,Uint64 ")
    return 2

def test(Byte^ val, Uint64 val2) Uint64
    write("Byte^,Uint64 ")
    return 3

def test(Uint64 val, Byte^ val2) Uint64
    write("Uint64,Byte^ ")
    return 4
    
Uint64 ret = 0
Uint64 val = 1
Byte^ val2 = 0
test(val,val)
test(val,val2)
ret = test(val2,val)
return ret

