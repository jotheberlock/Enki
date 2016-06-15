#ifndef _RTTI_
#define _RTTI

#include <map>
#include "type.h"

// Builds the rtti section in object files
class Rtti
{
  public:

    Rtti()
    {
        data=0;
        count=0;
    }

    ~Rtti()
    {
        delete[] data;
    }
    
    void finalise();
    uint64 lookup(uint64);

    uint64 size() 
    {
        return count;
    }
    
    unsigned char * getData()
    {
        return data;
    }
    
  protected:

    std::map<uint64, uint64> indexes;
    unsigned char * data;
    uint64 count;
    
};

extern Rtti * rtti;

#endif
