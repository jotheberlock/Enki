extern KERNEL32:WriteFile(Uint64 output, Byte^ buffer, Uint32 chars, Uint32^ written, Uint64 overlapped)
extern KERNEL32:GetStdHandle(Uint64 handle)
extern KERNEL32:ReadFile(Uint64 input, Byte^ buffer, Uint32 chars, Uint32^ read, Uint64 overlapped)
extern KERNEL32:CreateFileA(Byte^ filename, Uint32 access, Uint32 sharemode, Byte^ secure, Uint64 create_disp, Uint64 flags, Byte^ Template) Uint64
extern KERNEL32:GetFileSizeEx(Uint64 file, Uint64^ size) Uint32
extern KERNEL32:CreateFileMappingA(Uint32 file, Uint32 sec_attr, Uint32 protect, Uint32 max_high,  Uint32 max_low, Byte^ name) Uint32
extern KERNEL32:MapViewOfFileEx(Uint32 filemap, Uint32 access, Uint32 off_high, Uint32 off_low, Uint64 len, Byte^ base) Byte^
extern KERNEL32:GetLastError() Uint32
extern KERNEL32:VirtualProtect(Byte^ address, Uint64 size, Uint32 new, Uint32^ old) Uint32
extern KERNEL32:CloseHandle(Uint64 file) Uint32
extern KERNEL32:GetCommandLineA() Byte^
extern KERNEL32:ExitProcess(Uint64 ret)

def exit(Uint64 ret)
    ExitProcess(ret)    

def write(Byte^ ptr) Uint64
    Uint32 count 
    count = len(ptr)
    Uint32 written32
    Uint64 handle = 0
    handle = GetStdHandle(-11)
    WriteFile(handle, ptr, count, @written32, 0)
    Uint64 written = written32
    return written
    
def write_num(Uint64 num) Uint64
    Byte[20] bytes
    num_to_str(num, @bytes)
    Uint32 written32
    Uint64 handle = 0
    handle = GetStdHandle(-11)
    Uint32 chars = 16
    WriteFile(handle, @bytes, chars, @written32, 0)
    return 0

def read(Byte^ ptr, Uint64 len) Uint64
    Uint32 len32 = len
    Uint32 num_read32
    Uint64 handle = 0
    handle = GetStdHandle(-10)
    ReadFile(handle, ptr, len32, @num_read32, 0)
    Uint64 num_read = num_read32
    ptr = ptr + num_read
    ptr^ = 0
    return num_read

def open_file(Byte^ filename) Uint64
    Uint64 handle
    Uint32 access = 0xe0000000
    Uint32 sharemode = 3
    handle = CreateFileA(filename, access, sharemode, 0, 3, 0x80, 0)
    return handle

def close_file(Uint64 fd)
    CloseHandle(fd)

def get_file_size(Uint64 fd) Uint64
    Uint64 size
    GetFileSizeEx(fd, @size)
    return size

def map_file(Uint64 fd, Uint64 offset, Uint64 size, Uint64 permissions) Byte^
    Uint32 fmap = 0
    Uint32 zero = 0
    Uint32 protect 
    Uint32 map_access
    if permissions == READ_PERMISSION
        protect = PAGE_READ
        map_access = MMAP_READ
    elif permissions == RW_PERMISSION
        protect = PAGE_RW
        map_access = MMAP_RW
    elif permissions == EXECUTE_PERMISSION
        protect = PAGE_CODE
        map_access = MMAP_CODE
    else
       write("Unknown permissions\n")
       return 0
    Uint32 sfd = fd
    fmap = CreateFileMappingA(sfd, zero, protect, zero, zero, 0)
    if fmap == 0
        write("Failed to map\n")
        return 0
    Uint32 err = GetLastError()
    Byte^ ptr = 0
    Uint64 ffmap = fmap
    Uint32 loffs = offset
    ptr = MapViewOfFileEx(fmap, map_access, zero, loffs, size, 0)
    return ptr

def remap(Byte^ ptr, Uint64 size, Uint64 permissions) Uint64
    Uint32 new
    Uint32 old
    Uint64 ret
    if permissions == READ_PERMISSION
        new = PAGE_READ
    elif permissions == RW_PERMISSION
        new = PAGE_RW
    elif permissions == EXECUTE_PERMISSION
        new = PAGE_CODE
    else
       write("Unknown permissions\n")
       return 0
    ret = VirtualProtect(ptr, 4096, new, @old)
    if ret == 0
        write("Remap failed!\n")
        exit(1)
    return ret


def get_argc() Uint64
    Byte^ command_line = GetCommandLineA()
    Uint count = 1
    Byte^ ptr = command_line
    while ptr^ != 0
        if ptr^ == 32
            count = count + 1
        ptr = ptr + 1
    return count

def get_argv(Uint64 index) Byte^
    Byte^ ret = malloc(4096)
    ret^ = 0
    Byte^ command_line = GetCommandLineA()
    Byte^ ptr = command_line
    Uint count = 0
    while count < index
        if ptr^ == 32
             count = count + 1
        if ptr^ == 0
             return ret
        ptr = ptr + 1
    Byte^ dest = ret
    while ptr^ != 32
        dest^ = ptr^
        dest = dest + 1
        ptr = ptr + 1
        if ptr^ == 0
            dest^ = 0
            return ret
    dest^ = 0
    return ret

