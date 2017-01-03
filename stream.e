struct Stream
    pass

struct FileStream(Stream)
    Uint64 in
    Uint64 out

struct StdioStream(FileStream)
    pass

generic init_stream(stream, path) Uint64
generic close_stream(stream)
generic print(stream, param) Uint64
generic readline(stream, param, count) Uint64

# O_CREAT|O_RDWR, 0777
def init_stream(FileStream^ stream, Byte^ path) Uint64
    Uint64 fd = __syscall(2, path, 66, 0x1ff)
    stream^.in = fd
    stream^.out = fd

def close_stream(FileStream^ stream)
    __syscall(3,stream^.in)

def print(FileStream^ stream, Byte^ param) Uint64
    Uint64 count = len(param)
    Uint64 ret = __syscall(1, stream^.out, param, count)
    return ret

def readline(FileStream^ stream, Byte^ param, Uint64 count) Uint64
    Uint64 ret = __syscall(0, stream^.in, param, count)
    Byte^ end = param+ret
    end^ = 0
    return ret

# Hardcode stdin/out
def init_stream(StdioStream^ stream, Byte^ path) Uint64
    stream^.in = 0
    stream^.out = 1
    return 0

# Doesn't close anything
def close_stream(StdioStream^ stream)
    pass
