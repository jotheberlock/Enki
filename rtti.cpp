#include "rtti.h"
#include <string.h>

void Rtti::finalise()
{
    count = 0;
    std::map<std::string, Type *> tmap = types->get();
    for (std::map<std::string, Type *>::iterator it = tmap.begin();
         it != tmap.end(); it++)
    {
        Type * t = (*it).second;
        indexes[t->classId()] = count;
        count = count + strlen(t->name().c_str()) + 9;
        while (count & 0x7)
        {
            count++;
        }
    }

    data = new unsigned char[count];

    unsigned char * ptr = data;
    for (std::map<std::string, Type *>::iterator it = tmap.begin();
         it != tmap.end(); it++)
    {
        Type * t = (*it).second;
        
        wle64(ptr, t->classId());
        strcpy((char *)ptr, t->name().c_str());
        ptr += strlen(t->name().c_str())+1;
        while ((unsigned long)ptr & 0x7)
        {
            ptr++;
        }
    }
}

uint64 Rtti::lookup(uint64 id)
{
    if (!data)
    {
        printf("Lookup before data!\n");
        assert(false);
    }
    
    std::map<uint64, uint64>::iterator it = indexes.find(id);
    if (it == indexes.end())
    {
        printf("Couldn't find class id %lld!\n", id);
        assert(false);
    }

    return (*it).second;
}
