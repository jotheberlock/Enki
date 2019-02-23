Uint handle
handle = open_file("helloworld")
Uint size
size = get_file_size(handle)
Byte^ ptr = 0

ptr = map_file(handle,0,size,READ_PERMISSION)

Byte^ test = ptr
test = test + 1
if test == 0
    write("Null pointer!\n")
    return 0

write(ptr)
close_file(handle)
return size
