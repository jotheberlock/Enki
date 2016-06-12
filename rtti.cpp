#include "rtti.h"

void Rtti::finalise()
{
}

uint64 Rtti::lookup(uint64 id)
{
    std::map<uint64, uint64>::iterator it = indexes.find(id);
    if (it == indexes.end())
    {
        printf("Couldn't find class id %lld!\n", id);
        assert(false);
    }

    return (*it).second;
}
