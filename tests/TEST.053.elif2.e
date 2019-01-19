def test_elif(Uint val) Uint
    if val == 0
        write("Zero")
    elif val == 1
        write("One")
    elif val == 2
        write("Two")
    else
        write("Three")

test_elif(0)
test_elif(1)
test_elif(2)
test_elif(3)
return 0

        
