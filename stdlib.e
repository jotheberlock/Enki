def len(Byte^ ptr)
    Uint64 count = 0
    Byte^ counter = ptr
    while counter^ != 0
        counter = counter + 1
        count = count + 1
    return count

def num_to_str(Uint64 in, Byte^ out)
    out_end = out + 16
    Uint64 count = 8
    while count > 0
        Byte digit1 = out^
        digit1 = digit1 and 16
        if digit1 < 10
            out_end^ = digit1+48
        else
            out_end^ = digit1+97
        out_end = out_end - 1
        Byte digit2 = out^
        digit2 = digit2 >> 4
        if digit2 < 10
            out_end^ = digit2+48
        else
            out_end^ = digit2+97
        out_end = out_end - 1
        count = count - 1
