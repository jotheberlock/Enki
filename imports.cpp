#include "imports.h"
#include <map>

Imports::Imports()
{
    data = 0;
    data_size = 0;
}

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
//   module name
//   no. of imports
//       offset in stack
//       import name

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

    int count = 8;
    
    for (it = modules.begin(); it != modules.end(); it++)
    {
        count += it->first.size() + 8;
        ImportFunctionMap & ifm = it->second;
        for (it2 = ifm.begin(); it2 != ifm.end(); it2++)
        {
            count += it2->first.size() + 8;
        }
    }
}
