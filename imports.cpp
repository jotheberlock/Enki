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

void Imports::finalise()
{

}
