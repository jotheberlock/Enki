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
    }

    void addExport(std::string, FunctionScope *);

    unsigned char * getData()
    {
        return data;
    }

protected:

    unsigned char * data;
    std::vector<ExportRec> recs;

};

extern Exports * exports;

#endif
