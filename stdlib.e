def len(Byte^ ptr)
    Uint64 count = 0
    Byte^ counter = ptr
    while counter^ != 0
        counter = counter + 1
        count = count + 1
    return count

def num_to_str(Uint64 in, Byte^ out)
    Byte^ out_end = out + 16
    out_end^ = 0
    out_end = out_end - 1
    Uint64 count = 8
    while count > 0
        Byte the_digit = in & 255
        Byte digit1 = the_digit
        digit1 = digit1 & 15
        if digit1 < 10
            out_end^ = digit1+48
        else
            out_end^ = digit1+87
        out_end = out_end - 1
        Byte digit2 = the_digit
        digit2 = digit2 >> 4
        if digit2 < 10
            out_end^ = digit2+48
        else
            out_end^ = digit2+87
        out_end = out_end - 1
        in = in / 256
        count = count - 1

def to_upper(Byte^ str)
    Byte ch = str^
    while ch != 0
        if ch > 97
            if ch < 123
                ch = ch - 32
                str^ = ch
        str = str + 1
        ch = str^
