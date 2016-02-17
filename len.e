def len(Byte^ ptr)
    Uint64 count = 0
    Byte^ counter = ptr
    while counter^ != 0
        counter = counter + 1
        count = count + 1
    return count
