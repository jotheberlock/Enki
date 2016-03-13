#include "macho.h"

#if defined(LINUX_HOST) || defined(CYGWIN_HOST)
#include <sys/stat.h>
#endif

MachOImage::MachOImage()
{
    base_addr = 0x400000;
    next_addr =  base_addr + 12288;
    fname = "a.out";
    sf_bit = false;
    arch = 0;
    bases[IMAGE_UNALLOCED_DATA] = 0x800000;
    sizes[IMAGE_UNALLOCED_DATA] = 4096;
}

MachOImage::~MachOImage()
{
    for (int loopc=0; loopc<4; loopc++)
    {
        delete[] sections[loopc];
    }
}

bool MachOImage::configure(std::string param, std::string val)
{
    if (param == "file")
    {
        fname = val;
    }
    else if (param == "bits")
    {
        if (val == "64")
        {
            sf_bit = true;
        }
        else if (val == "32")
        {
            sf_bit = false;
        }
        else
        {
            return false;
        }
    }
    else if (param == "arch")
    {
        arch = strtol(val.c_str(), 0, 10);
    }
    else if (param == "baseaddr")
    {
        base_addr = strtol(val.c_str(), 0, 0);
        next_addr = base_addr + 12288;
    }
    else if (param == "heapaddr")
    {
        bases[IMAGE_UNALLOCED_DATA] = strtol(val.c_str(), 0, 0);
    }
    else
    {
        return Image::configure(param, val);
    }

    return true;
}

void MachOImage::finalise()
{
}

void MachOImage::materialiseSection(int s)
{
    sections[s] = new unsigned char[sizes[s]];
    bases[s] = next_addr;
    next_addr += sizes[s];
    next_addr += 4096;
    while (next_addr % 4096)
    {
        next_addr++;
    }

    if (guard_page)
    {
        next_addr += 4096;   // Create a guard page
    }
}

