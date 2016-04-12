#include "mem.h"
#include "platform.h"

#include <stdio.h>

#ifdef HAVE_MPROTECT
#include <sys/mman.h>
#elif HAVE_WINDOWS_API
#include <windows.h>
#endif

static int getFlags(int in)
{
    int ret = 0;
#ifdef HAVE_MPROTECT
    if (in & MEM_READ)
        ret |= PROT_READ;
    if (in & MEM_WRITE)
        ret |= PROT_WRITE;
    if (in & MEM_EXEC)
        ret |= PROT_EXEC;
#elif HAVE_WINDOWS_API
	if (in & MEM_WRITE)
		ret = PAGE_READWRITE;
	if (in & MEM_EXEC)
		ret = PAGE_EXECUTE_READ;
#endif
    return ret;
}


MemBlock Mem::getBlock(uint64 len, int perms)
{
    MemBlock ret;
#ifdef HAVE_MPROTECT
    void * ptr = mmap(0, len, getFlags(perms), MAP_PRIVATE | MAP_ANON, -1, 0);
#elif HAVE_WINDOWS_API
	void * ptr = VirtualAlloc(0, len, MEM_RESERVE | MEM_COMMIT, getFlags(perms));
#endif
    if (ptr)
    {
        fprintf(log_file, "Got block!\n");
        ret.ptr = (unsigned char *)ptr;
        ret.len = len;
    }
	else
	{
		fprintf(log_file, "No block for you!\n");
	}
    return ret;
}

void Mem::releaseBlock(MemBlock & m)
{
    if (m.ptr == 0)
    {
        return;
    }
    
#ifdef HAVE_MPROTECT
    munmap(m.ptr, m.len);
#elif HAVE_WINDOWS_API
	VirtualFree(m.ptr, m.len, MEM_RELEASE);
#endif
}

bool Mem::changePerms(MemBlock & m, int perms)
{
    bool ret = false;
#ifdef HAVE_MPROTECT
    if (mprotect(m.ptr, m.len, getFlags(perms)) == 0)
    {
        ret = true;
        fprintf(log_file, "Changed perms!\n");
    }
#elif HAVE_WINDOWS_API
	DWORD oldperms;
	if (VirtualProtect(m.ptr, m.len, getFlags(perms), &oldperms))
	{
		ret = true;
		fprintf(log_file, "Changed perms!\n");
	}
	else
	{
		fprintf(log_file, "No perms for you!\n");
	}
#endif
    return ret;
}
