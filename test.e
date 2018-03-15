extern KERNEL32:CreateFileA(Byte^ filename, Uint32 access, Uint32 sharemode, Byte^ secure, Uint32 create_disp, Uint32 flags, Byte^ Template) Uint32
extern KERNEL32:GetFileSizeEx(Uint32 file, Uint64^ size) Uint32
extern KERNEL32:CreateFileMappingA(Uint32 file, Uint32 sec_attr, Uint32 protect, Uint32 max_high,  Uint32 max_low, Byte^ name) Uint32
extern KERNEL32:MapViewOfFile(Uint32 filemap, Uint32 access, Uint32 off_high, Uint32 off_low, Uint64 len) Byte^

Uint32 handle = 0
Byte[20] buffer

Uint32 access = 0xc0000000
Uint32 sharemode = 3
Uint32 create_disp = 3
Uint32 flags = 0x80
handle = CreateFileA("test.txt", access, sharemode, 0, create_disp, flags, 0)
Uint64 size
GetFileSizeEx(handle, @size)
Uint32 fmap = 0
Uint32 zero = 0
Uint32 protect = 0x40
fmap = CreateFileMappingA(handle, zero, protect, zero, zero, 0)
Byte^ ptr = 0
Uint32 map_access = 6
ptr = MapViewOfFile(fmap, map_access, zero, zero, 0)
Uint64 out = GetStdHandle(-11)
Uint32 written32
Uint32 len32 = size
WriteFile(out, ptr, len32, @written32, 0)
return len32
