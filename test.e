extern KERNEL32:CreateFileA(Byte^ filename, Uint32 access, Uint32 sharemode, Byte^ secure, Uint32 create_disp, Uint32 flags, Byte^ Template) Uint32
extern KERNEL32:GetFileSizeEx(Uint32 file, Uint64^ size) Uint32

Uint32 handle = 0
Byte[20] buffer

Uint32 access = 0x80000000
Uint32 sharemode = 0
Uint32 create_disp = 3
Uint32 flags = 0
handle = CreateFileA("test.txt", access, sharemode, 0, create_disp, flags, 0)
Uint64 size
GetFileSizeEx(handle, @size)
num_to_str(size, @buffer)
write(@buffer)