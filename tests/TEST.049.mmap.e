Uint64 handle
handle = open_file("helloworld")
Uint64 size
size = get_file_size(handle)
Byte^ ptr = 0

ptr = map_file(handle,0,size,0)

if ptr == 0
    write("Null pointer!\n")
    return 0

write(ptr)
close_file(handle)
return size
