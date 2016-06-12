#ifndef _RTTI_
#define _RTTI

#include <map>
#include "type.h"

// Builds the rtti section in object files
class Rtti
{
  public:

    void finalise();
    uint64 lookup(uint64);

  protected:

    std::map<uint64, uint64> indexes;
    
};

extern Rtti * rtti;

#endif
