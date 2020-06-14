def len(Byte^ ptr) Uint
    Uint count = 0
    Byte^ counter = ptr
    while counter^ != 0
        counter += 1
        count += 1
    return count

def num_to_str(Uint64 in, Byte^ out) Uint
    Byte^ out_end = out + 16
    out_end^ = 0
    out_end -= 1
    Uint count = 8
    while count > 0
        Byte the_digit = in & 255
        Byte digit1 = the_digit
        digit1 = digit1 & 15
        if digit1 < 10
            out_end^ = digit1+48
        else
            out_end^ = digit1+87
        out_end -= 1
        Byte digit2 = the_digit
        digit2 = digit2 >> 4
        if digit2 < 10
            out_end^ = digit2+48
        else
            out_end^ = digit2+87
        out_end -= 1
        in = in >> 8
        count -= 1
    return 0

def str_to_num(Byte^ in) Uint
    Byte^ ptr = in
    Uint ret = 0
    while ptr^ > 0
        ret = ret << 4
        Byte the_digit
        the_digit = ptr^
        Uint to_add = the_digit
        if the_digit < 58
            to_add = to_add - 48
            ret = ret + to_add
        elif the_digit < 71
            to_add = to_add - 55
            ret = ret + to_add
        else
            to_add = to_add - 87
            ret = ret + to_add
        ptr = ptr + 1
    return ret

def to_upper(Byte^ str) Uint
    Byte ch = str^
    while ch != 0
        if ch > 97
            if ch < 123
                ch = ch - 32
                str^ = ch
        str += 1
        ch = str^
    return 0

def strcmp(Byte^ str1, Byte^ str2) Uint
    Uint true = 1
    while true == 1
          Uint char1 = str1^
          Uint char2 = str2^
          if char1 != char2
             return 0
          if char1 == 0
             return 1
          str1 += 1
          str2 += 1
    return 0

def display_num(Byte^ text, Uint64 val) Uint
    write(text)
    write_num(val)
    write("\n")
    return 0

