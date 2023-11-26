#ifndef _MEM_
#define _MEM_

/*
	An abstraction over Windows and Unix methods for
	setting memory permissions. Goes back to when the
	compiler JITted (so memory could be set writeable
	for generation, then read-only and executable to run it).
	Not currently important.
*/

#include "platform.h"

#define MEM_READ 1
#define MEM_WRITE 2
#define MEM_EXEC 4

class MemBlock
{
public:

	MemBlock()
	{
		ptr = 0;
		len = 0;
	}

	bool isNull()
	{
		return ptr == 0;
	}

	unsigned char * ptr;
	uint64_t len;

};


class Mem
{
public:

	MemBlock getBlock(uint64_t len, int perms);
	void releaseBlock(MemBlock & m);
	bool changePerms(MemBlock & m, int perms);

};


#endif
