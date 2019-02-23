generic test(val,val2) Uint

def test(Uint val, Uint val2) Uint
    write("Uint,Uint ")
    return 2

def test(Byte^ val, Uint val2) Uint
    write("Byte^,Uint ")
    return 3

def test(Uint val, Byte^ val2) Uint
    write("Uint,Byte^ ")
    return 4
    
Uint ret = 0
Uint val = 1
Byte^ val2 = 0
test(val,val)
test(val,val2)
ret = test(val2,val)
return ret

