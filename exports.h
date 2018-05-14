#ifndef _EXPORTS_
#define _EXPORTS_

#include <vector>
#include "symbols.h"

class ExportRec
{
public:

    std::string name;
    FunctionScope * fun;

};

class Exports
{
public:

    Exports()
    {
        data = 0;
        data_size = 0;
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

    uint64_t size()
    {
        return data_size;
    }

protected:
    
    std::string module_name;
    unsigned char * data;
    uint64_t data_size;
    std::vector<ExportRec> recs;

};

extern Exports * exports;

#endif
