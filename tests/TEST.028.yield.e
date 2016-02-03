def test()
    yield 2
    yield 4
    return 6

Uint64 ret = 0
Uint64$ val = test()
!val
!val
!val
ret = val
return ret



