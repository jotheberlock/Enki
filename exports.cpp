#include <string.h>
#include "exports.h"
#include "platform.h"
#include "image.h"
#include "configfile.h"

void Exports::addExport(std::string n, FunctionScope * f)
{
    ExportRec rec;
    rec.name = n;
    rec.fun = f;
    recs.push_back(rec);
}

uint64_t round64(uint64_t i)
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
    data_size += 8;   // Module name length
    data_size += round64(module_name.size()+1);

    data_size += 8;   // Number of entries

    for (int loopc = 0; loopc < recs.size(); loopc++)
    {
        data_size += 8;  // Relocation
        data_size += 8;  // String size
        data_size += round64(recs[loopc].name.size()+1);
    }

    delete[] data;
    data = new unsigned char[data_size];
    unsigned char * ptr = data;

    uint64_t len = round64(module_name.size() + 1);
    wle64(ptr, len);
    strcpy((char *)ptr, module_name.c_str());
    ptr += len;

    wle64(ptr, recs.size());
    for (int loopc = 0; loopc < recs.size(); loopc++)
    {
        new FunctionTableRelocation(configuration->image,
                                    recs[loopc].fun,
                                    ptr-data,
                                    IMAGE_EXPORTS);
        wle64(ptr, 0xdeadbeefdeadbeefLL);
        len = round64(recs[loopc].name.size() + 1);
        wle64(ptr, len);
        strcpy((char *)ptr, recs[loopc].name.c_str());
        ptr += len;
    }
}
