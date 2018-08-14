#include "imports.h"
#include <map>
#include <string.h>

void Imports::add(std::string module, std::string fun, Type * ty)
{
    ImportFunctionMap & ifm = modules[module];
    ImportRec rec;
    rec.type = ty;
    ifm[fun] = rec;
} 

ImportRec * Imports::lookup(std::string module, std::string name)
{
    ImportModuleMap::iterator it = modules.find(module);
    if (it == modules.end())
    {
        return 0;
    }

    ImportFunctionMap & ifm = modules[module];
    ImportFunctionMap::iterator it2 = ifm.find(name);
    if (it2 == ifm.end())
    {
        return 0;
    }
    
    return &ifm[name];
}

// imports structure:
// no. of modules
//   offset to imports
//   no. of imports
//   module name
//   padding to 64 bits
//       offset to next import
//       offset in stack
//       import name
//       padding to 64 bits

static uint64 round64(uint64 i)
{
    while (i & 0x7)
    {
        i++;
    }

    return i;
}

void Imports::finalise()
{
        // Clean out all the functions that never actually got
        // imported

    ImportModuleMap::iterator it;
    ImportFunctionMap::iterator it2;
    ImportFunctionMap::iterator next_it2;
    
    for (it = modules.begin(); it != modules.end(); it++)
    {
        printf("Module %s\n", it->first.c_str());
        ImportFunctionMap & ifm = it->second;
        for (it2 = ifm.begin(); it2 != ifm.end(); it2 = next_it2)
        {
            next_it2 = it2;
            ++next_it2;
            
            ImportRec & ir = it2->second;
            if (ir.value == 0)
            {
                ifm.erase(it2);
            }
        }
    }

    data_size = 8;
    
    for (it = modules.begin(); it != modules.end(); it++)
    {
        int mcount = round64(it->first.size()) + 24;
        
        data_size += mcount;
        ImportFunctionMap & ifm = it->second;
        for (it2 = ifm.begin(); it2 != ifm.end(); it2++)
        {
            int icount = round64(it2->first.size()) + 24;
            data_size += icount;
        }
    }

    data = new unsigned char[data_size];
    unsigned char * ptr = data;

    for (it = modules.begin(); it != modules.end(); it++)
    {
        ImportFunctionMap & ifm = it->second;
        wle64(ptr, round64(it->first.size()) + 24);
        wle64(ptr, ifm.size());
        wle64(ptr, it->first.size());
        strcpy((char *)ptr, it->first.c_str());
        ptr += round64(it->first.size());
        
        for (it2 = ifm.begin(); it2 != ifm.end(); it2++)
        {
            wle64(ptr, round64(it2->first.size()) + 24);
            printf(">> %s %lld\n",
                   it2->first.c_str(),
                   it2->second.value->stackOffset());
            wle64(ptr, it2->second.value->stackOffset());
            wle64(ptr, it2->first.size());
            strcpy((char *)ptr, it2->first.c_str());
            ptr += round64(it2->first.size());
        }
    }
}
