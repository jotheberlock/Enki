#ifndef _EXPORTS_
#define _EXPORTS_

#include <map>
#include "symbols.h"

class Exports
{
public:

    Exports()
    {
        data = 0;
        data_size = 0;
    }

    ~Exports()
    {
        delete[] data;
    }
    
    void setName(std::string s)
    {
        module_name = s;
    }

    void addExport(std::string, FunctionScope *);
    void finalise();

    unsigned char * getData()
    {
        return data;
    }

    uint64 size()
    {
        return data_size;
    }

protected:
    
    std::string module_name;
    unsigned char * data;
    uint64 data_size;
    std::map<std::string, FunctionScope *> recs;

};

extern Exports * exports;

#endif
