Uint64 max = 100
Uint64 count = 0
Byte[20] number

while count < max
    if (count % 15) == 0
        write("Fizzbuzz\n")
    elif (count % 3) == 0
        write("Fizz\n")
    elif (count % 5) == 0
        write("Buzz\n")
    else
        num_to_str(count, @number)
        write(@number)
        write("\n")
    count = count + 1

