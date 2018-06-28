#include <string.h>
#include "exports.h"
#include "platform.h"
#include "image.h"
#include "configfile.h"
#include "asm.h"

void Exports::addExport(std::string n, FunctionScope * f)
{
    recs[n] = f;
}

static uint64_t round64(uint64_t i)
{
    while (i & 0x7)
    {
        i++;
    }

    return i;
}

void Exports::finalise()
{
    data_size = 0;
    data_size += 8;   // Pointer to next export table
    data_size += 8;   // Module name length
    data_size += round64(module_name.size()+1);

    data_size += 8;   // Number of entries

    std::map<std::string, FunctionScope *>::iterator it;
    for (it = recs.begin(); it != recs.end(); it++)
    {
        data_size += 8;  // Relocation
        data_size += 8;  // String size
        data_size += round64(it->first.size()+1);
    }

    delete[] data;
    data = new unsigned char[data_size];
    unsigned char * ptr = data;
    wle64(ptr, 0x0);
    
    uint64_t len = round64(module_name.size() + 1);
    wle64(ptr, len);
    
    strcpy((char *)ptr, module_name.c_str());
    ptr += len;

    wle64(ptr, recs.size());
    for (it = recs.begin(); it != recs.end(); it++)
    {
        FunctionTableRelocation * ftr = new FunctionTableRelocation(configuration->image,
                                                                    it->second,
                                                                    ptr-data,
                                                                    IMAGE_EXPORTS);
        ftr->add64();
                
        wle64(ptr, 0xdeadbeefdeadbeefLL);
        len = round64(it->first.size() + 1);
        wle64(ptr, len);
        strcpy((char *)ptr, it->first.c_str());
        ptr += len;
    }
}
