#ifndef _REGSET_
#define _REGSET_

#include <assert.h>
#include <string>

#include "platform.h"

#define MAXREG 256

class RegSet
{
  public:

    RegSet();

    void setAll();
    void setEmpty();
    bool isEmpty();

    int count()
    {
        int ret = 0;
        for (int loopc=0; loopc<MAXREG; loopc++)
        {
            if (isSet(loopc))
            {
                ret++;
            }
        }

        return ret;
    }
    
    void set(int i)
    {
        assert(i >= 0 && i < MAXREG);
        regs[i/64] |= (uint64)0x1 << (i % 64);
    }

    void clear(int i)
    {
        assert(i >= 0 && i < MAXREG);
        regs[i/64] = (uint64)regs[i/64] & ~(0x1 << (i % 64));
    }
    
    bool isSet(int i)
    {
        assert(i >= 0 && i < MAXREG);
        return (regs[i/64] & (uint64)0x1 << (i % 64)) ? true : false;
    }

    bool operator[](int i)
    {
        return isSet(i);
    }
    
    RegSet operator|(const RegSet &);
    RegSet operator!();
    RegSet operator&(const RegSet &);
    RegSet operator^(const RegSet &);
    bool operator==(const RegSet &);
    bool operator!=(const RegSet & r)
    {
        return !(*this == r);
    }
    
    std::string toString();
    
  protected:

    uint64 regs[MAXREG/64];
    
};

#endif
