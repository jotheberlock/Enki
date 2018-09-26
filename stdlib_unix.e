def write(Byte^ ptr) Uint
    Uint count 
    count = len(ptr)
    Uint written
    written = __syscall(SYSCALL_WRITE, STDOUT, ptr, count)
    return written

def write_num(Uint64 num) Uint64
    Byte[20] bytes
    num_to_str(num, @bytes)
    __syscall(SYSCALL_WRITE, STDOUT, @bytes, 16)
    return 0

def read(Byte^ ptr, Uint len) Uint
    Uint count = __syscall(SYSCALL_READ, STDIN, ptr, len)
    ptr += count
    ptr ^= 0
    return count

def open_file(Byte^ filename) Uint64
    Uint64 handle
    handle = __syscall(SYSCALL_OPEN, filename, 2, 0)
    return handle

def close_file(Uint64 fd)
    __syscall(SYSCALL_CLOSE, fd)

def get_file_size(Uint64 fd) Uint64
    Uint64 current
    Uint64 size
    current = __syscall(SYSCALL_LSEEK, fd, 0, 1)
    size = __syscall(SYSCALL_LSEEK, fd, 0, 2)
    __syscall(SYSCALL_LSEEK, fd, 0, 0)
    return size

def map_file(Uint64 fd, Uint64 offset, Uint64 size, Uint64 permissions) Byte^
    Uint32 protect
    
    if permissions == READ_PERMISSION
        protect = PROT_READ
    elif permissions == RW_PERMISSION
        protect = PROT_READWRITE
    elif permissions == EXECUTE_PERMISSION
        protect = PROT_EXEC
    else
       write("Unknown permissions\n")
       return 0
    Uint32 flags = MAP_SHARED
    Byte^ ret
    ret = __syscall(SYSCALL_MMAP, 0, size, protect, flags, fd, offset)
    return ret
    
def remap(Byte^ ptr, Uint64 size, Uint64 permissions) Uint64
    Uint32 protect

    if permissions == READ_PERMISSION
        protect = PROT_READ
    elif permissions == RW_PERMISSION
        protect = PROT_READWRITE
    elif permissions == EXECUTE_PERMISSION
        protect = PROT_EXEC
    else
       write("Unknown permissions\n")
       return 0
    Uint64 ret = __syscall(SYSCALL_MPROTECT, ptr, size, protect)
    if ret != 0
        display_num("Failure to remap!", ret)
    return 0

def get_argc() Uint64
    Uint64^ argcp = __osstackptr
    Uint64 argc = argcp^
    return argc

def get_argv(Uint64 index) Byte^
    Byte^^ argp = __osstackptr+8
    Uint64 idx = index * 8
    argp += idx
    return argp^

def exit(Uint64 ret)
    __syscall(SYSCALL_EXIT, ret)
