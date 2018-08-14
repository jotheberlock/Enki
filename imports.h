#ifndef __IMPORTS__
#define __IMPORTS__

#include <map>
#include "type.h"
#include "codegen.h"

class ImportRec
{
  public:

    ImportRec()
    {
        type = 0;
        value = 0;
    }
    
    Type * type;
    Value * value;
    
};

typedef std::map<std::string, ImportRec> ImportFunctionMap;
typedef std::map<std::string, ImportFunctionMap> ImportModuleMap;

class Imports
{
  public:

    Imports()
    {
        data = 0;
        data_size = 0;
    }
    
    ~Imports()
    {
        delete[] data;
    }
    
    void add(std::string, std::string, Type *);
    ImportRec * lookup(std::string, std::string);

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

    ImportModuleMap modules;
    unsigned char * data;
    uint64 data_size;
    
};

extern Imports * imports;

#endif
