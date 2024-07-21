struct Stream
    pass

struct FileStream(Stream)
    Uint in
    Uint out

struct StdioStream(FileStream)
    pass

generic init_stream(stream, path) Uint
generic close_stream(stream)
generic print(stream, param) Uint
generic readline(stream, param, count) Uint

# O_CREAT|O_RDWR, 0777
def init_stream(FileStream^ stream, Byte^ path) Uint
    Uint fd = __syscall(SYSCALL_OPENAT, AT_FDCWD, path, 66, 0x1ff)
    stream^.in = fd
    stream^.out = fd

def close_stream(FileStream^ stream)
    __syscall(SYSCALL_CLOSE,stream^.in)

def print(FileStream^ stream, Byte^ param) Uint
    Uint count = len(param)
    Uint ret = __syscall(SYSCALL_WRITE, stream^.out, param, count)
    return ret

def readline(FileStream^ stream, Byte^ param, Uint count) Uint
    Uint ret = __syscall(SYSCALL_READ, stream^.in, param, count)
    Byte^ end = param+ret
    end^ = 0
    return ret

# Hardcode stdin/out
def init_stream(StdioStream^ stream, Byte^ path) Uint
    stream^.in = 0
    stream^.out = 1
    return 0

# Doesn't close anything
def close_stream(StdioStream^ stream)
    pass
