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
    Uint32 protect = PAGE_READ
    Uint32 sfd = fd
    fmap = CreateFileMappingA(sfd, zero, protect, zero, zero, 0)
    if fmap == 0
        write("Failed to map\n")
        return 0
    Uint32 err = GetLastError()
    Byte^ ptr = 0
    Uint32 map_access = MMAP_READ
    Uint64 ffmap = fmap
    Uint32 loffs = offset
    ptr = MapViewOfFileEx(fmap, map_access, zero, loffs, size, 0)
    return ptr