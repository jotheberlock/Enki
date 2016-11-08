generic test(val) Uint64

def test(Byte^ val) Uint64
    return 3

def test(Uint64 val) Uint64
    return 4

Uint64 ret = 0
Uint64 val = 1
ret = test(val)
return ret

