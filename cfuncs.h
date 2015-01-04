#ifndef _CFUNCS_
#define _CFUNCS_

#include <string>
#include <vector>
#include <stdint.h>

class Codegen;
class ExternalFunctionType;

class CFuncRec
{
  public:

    std::string name;
    ExternalFunctionType * type;

};

class CFuncs
{
  public:

    CFuncs();
    ~CFuncs();

    void add(uint64_t, std::string);
    bool find(std::string, uint64_t &);
    uint64_t base()
    {
        return (uint64_t)table;
    }

    void addToCodegen(Codegen *);
    
  protected:

    std::vector<CFuncRec> names;
    uint64_t * table;
    int count;
    
};

extern CFuncs * cfuncs;

#endif
