#ifndef _MEM_
#define _MEM_

#include <stdint.h>

#define MEM_READ 1
#define MEM_WRITE 2
#define MEM_EXEC 4

class MemBlock
{
  public:

    MemBlock()
    {
        ptr=0;
        len=0;
    }

    bool isNull()
    {
        return ptr==0;
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
