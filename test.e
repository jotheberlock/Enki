Byte[18] outstr

outstr[16] = 0
num_to_str(0xdeadbeef, @outstr[0])
write(outstr, 16)
